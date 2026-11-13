#include "middleware/auth_middleware.h"

#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <cctype>
#include <optional>
#include <utility>

#include "auth/token_signer.h"
#include "common/api_response.h"
#include "common/errors.h"
#include "config/app_config.h"
#include "log.h"
#include "utils/utils.h"

namespace middleware
{

AuthMiddleware::AuthMiddleware(const infra::JwtConfig& config, bool is_access)
    : m_refresh_signer(config.refresh_secret, config.issuer),
      m_access_signer(config.access_secret, config.issuer),
      m_is_access(is_access)
{
}

void AuthMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                            drogon::MiddlewareNextCallback&& next_cb,
                            drogon::MiddlewareCallback&&     mcb)
{
    const std::string authorization = req->getHeader("authorization");

    if (authorization.empty())
    {
        LOGW("missing authorization header: %s", req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "authorization header is required"));
        return;
    }

    if (!utils::has_bearer_prefix(authorization))
    {
        LOGW("invalid authorization scheme: %s", req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "authorization header must use the Bearer scheme"));
        return;
    }

    const std::string token = utils::trim(authorization.substr(utils::kBearerPrefixLength));
    if (token.empty())
    {
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "bearer token must not be empty"));
        return;
    }

    std::optional<infra::TokenPayload> payload;
    if (m_is_access)
        payload = m_access_signer.verify(token);
    else
        payload = m_refresh_signer.verify(token);

    if (!payload)
    {  // 后续优化可以考虑新增error_code详细区分验证失败的原因，便于前端根据超时原因更新access_token
        LOGW("invalid or expired access token: %s", req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "invalid or expired access token"));
        return;
    }

    req->attributes()->insert(kUserIdAttribute, payload->public_id);
    next_cb(std::move(mcb));
}

AccessAuthMiddleware::AccessAuthMiddleware(const infra::JwtConfig& config)
    : AuthMiddleware(config, true)
{
}

void AccessAuthMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                                  drogon::MiddlewareNextCallback&& next_cb,
                                  drogon::MiddlewareCallback&&     mcb)
{
    AuthMiddleware::invoke(req, std::move(next_cb), std::move(mcb));
}

RefreshAuthMiddleware::RefreshAuthMiddleware(const infra::JwtConfig& config)
    : AuthMiddleware(config, false)
{
}

void RefreshAuthMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                                   drogon::MiddlewareNextCallback&& next_cb,
                                   drogon::MiddlewareCallback&&     mcb)
{
    AuthMiddleware::invoke(req, std::move(next_cb), std::move(mcb));
}

std::string current_public_id(const drogon::HttpRequestPtr& req)
{
    return req->attributes()->get<std::string>(kUserIdAttribute);
}

}  // namespace middleware
