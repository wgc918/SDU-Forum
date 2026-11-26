#include "middleware/auth_middleware.h"

#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <cctype>
#include <cstdint>
#include <optional>
#include <utility>

#include "auth/token_signer.h"
#include "common/api_response.h"
#include "common/ban_enums.h"
#include "common/errors.h"
#include "config/app_config.h"
#include "log.h"
#include "repositories/account_repository.h"
#include "repositories/token_repository.h"
#include "services/admin/ban_service.h"
#include "utils/utils.h"

namespace middleware
{

AuthMiddleware::AuthMiddleware(const infra::JwtConfig&        config,
                               repository::TokenRepository&   token_repo,
                               repository::AccountRepository& account_repo,
                               admin::service::BanService& ban_service, bool is_access)
    : m_refresh_signer(config.refresh_secret, config.issuer),
      m_access_signer(config.access_secret, config.issuer),
      m_token_repo(token_repo),
      m_account_repo(account_repo),
      m_ban_service(ban_service),
      m_is_access(is_access)
{
}

void AuthMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                            drogon::MiddlewareNextCallback&& next_cb,
                            drogon::MiddlewareCallback&&     mcb)
{
    utils::BearerError bearer_error = utils::BearerError::None;
    auto               token = utils::parse_bearer(req->getHeader("authorization"), &bearer_error);
    if (!token)
    {
        LOGW("%s: %s", utils::bearer_error_message(bearer_error), req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   utils::bearer_error_message(bearer_error)));
        return;
    }

    std::optional<infra::TokenPayload> payload;
    if (m_is_access)
    {
        if (m_token_repo.find_access_token(
                utils::hmac_sha256_base64url(m_token_repo.hash_secret(), *token)))
        {
            LOGW("old access token: %s", req->path().c_str());
            mcb(common::error_response(common::ErrorCode::Unauthorized, "the access token is old"));
            return;
        }
        payload = m_access_signer.verify(*token);
    }
    else
    {
        if (m_token_repo.find_refresh_token(
                utils::hmac_sha256_base64url(m_token_repo.hash_secret(), *token)))
        {
            LOGW("old refresh token: %s", req->path().c_str());
            mcb(common::error_response(common::ErrorCode::Unauthorized,
                                       "the refresh token is old"));
            return;
        }
        payload = m_refresh_signer.verify(*token);
    }

    if (!payload)
    {  // 后续优化可以考虑新增error_code详细区分验证失败的原因，便于前端根据超时原因更新access_token
        LOGW("invalid or expired access token: %s", req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "invalid or expired access token"));
        return;
    }

    auto account = m_account_repo.find_account_by_public_id(payload->sub);
    if (!account)
    {
        LOGW("token subject not found: %s", req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized, "account not found"));
        return;
    }

    auto entry = m_ban_service.is_banned(account->id, ban::BanType::All);
    if (entry.has_value())
    {
        LOGW("account is banned all ,user_id=%s", account->user_id.c_str());
        mcb(common::error_response(common::ErrorCode::AccountBanned, "account is banned all"));
        return;
    }

    req->attributes()->insert(kPublicIdAttribute, payload->sub);
    req->attributes()->insert(kUserRoleAttribute, payload->role);
    req->attributes()->insert(kUserIdAttribute, account->user_id);
    req->attributes()->insert(KAccountIdAttribute, account->id);
    next_cb(std::move(mcb));
}

AccessAuthMiddleware::AccessAuthMiddleware(const infra::JwtConfig&        config,
                                           repository::TokenRepository&   token_repo,
                                           repository::AccountRepository& account_repo,
                                           admin::service::BanService&    ban_service)
    : AuthMiddleware(config, token_repo, account_repo, ban_service, true)
{
}

void AccessAuthMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                                  drogon::MiddlewareNextCallback&& next_cb,
                                  drogon::MiddlewareCallback&&     mcb)
{
    AuthMiddleware::invoke(req, std::move(next_cb), std::move(mcb));
}

RefreshAuthMiddleware::RefreshAuthMiddleware(const infra::JwtConfig&        config,
                                             repository::TokenRepository&   token_repo,
                                             repository::AccountRepository& account_repo,
                                             admin::service::BanService&    ban_service)
    : AuthMiddleware(config, token_repo, account_repo, ban_service, false)
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
    return req->attributes()->get<std::string>(kPublicIdAttribute);
}

std::string current_user_id(const drogon::HttpRequestPtr& req)
{
    return req->attributes()->get<std::string>(kUserIdAttribute);
}

uint64_t current_account_id(const drogon::HttpRequestPtr& req)
{
    return req->attributes()->get<uint64_t>(KAccountIdAttribute);
}
}  // namespace middleware
