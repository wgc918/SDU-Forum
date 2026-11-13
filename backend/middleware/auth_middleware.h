#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"

namespace middleware
{

/// 鉴权通过后注入请求属性的 key，可通过 current_public_id() 读取
inline constexpr char kUserIdAttribute[] = "public_id";

/**
 * @brief JWT 鉴权中间件（Drogon 版本）
 *
 * 验证 Authorization: Bearer <access_token>，校验签名、issuer 与有效期后，
 * 把 token 中的 user_id 注入 req->attributes()。
 */
class AuthMiddleware
{
public:
    AuthMiddleware(const infra::JwtConfig& config, bool is_access);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb);

private:
    infra::TokenSigner m_refresh_signer;
    infra::TokenSigner m_access_signer;
    bool               m_is_access;  // ture 校验access_token ,否则校验refresh_token
};

class AccessAuthMiddleware : public drogon::HttpMiddleware<AccessAuthMiddleware, false>,
                             public AuthMiddleware
{
public:
    AccessAuthMiddleware(const infra::JwtConfig& config);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

class RefreshAuthMiddleware : public drogon::HttpMiddleware<RefreshAuthMiddleware, false>,
                              public AuthMiddleware
{
public:
    RefreshAuthMiddleware(const infra::JwtConfig& config);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

/// 读取 AuthMiddleware 注入的 user_id；未鉴权时返回空字符串
std::string current_public_id(const drogon::HttpRequestPtr& req);

}  // namespace middleware
