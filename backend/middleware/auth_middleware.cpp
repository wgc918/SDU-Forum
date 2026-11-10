#include "middleware/auth_middleware.h"

#include <cctype>
#include <utility>

#include "common/api_response.h"
#include "common/errors.h"
#include "log.h"

namespace middleware
{
namespace
{

constexpr std::size_t kBearerPrefixLength = 7;  // "Bearer "

bool has_bearer_prefix(const std::string& value)
{
    static const char kExpected[] = "bearer ";

    if (value.size() < kBearerPrefixLength)
        return false;

    for (std::size_t i = 0; i < kBearerPrefixLength; ++i)
    {
        if (std::tolower(static_cast<unsigned char>(value[i])) != kExpected[i])
            return false;
    }
    return true;
}

std::string trim(const std::string& value)
{
    std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

}  // namespace

AuthMiddleware::AuthMiddleware(std::string access_jwt_secret, std::string jwt_issuer)
    : m_signer(std::move(access_jwt_secret), std::move(jwt_issuer))
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

    if (!has_bearer_prefix(authorization))
    {
        LOGW("invalid authorization scheme: %s", req->path().c_str());
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "authorization header must use the Bearer scheme"));
        return;
    }

    const std::string token = trim(authorization.substr(kBearerPrefixLength));
    if (token.empty())
    {
        mcb(common::error_response(common::ErrorCode::Unauthorized,
                                   "bearer token must not be empty"));
        return;
    }

    auto payload = m_signer.verify(token);
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

std::string current_public_id(const drogon::HttpRequestPtr& req)
{
    return req->attributes()->get<std::string>(kUserIdAttribute);
}

}  // namespace middleware
