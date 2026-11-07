#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <string>

#include "auth/token_signer.h"

namespace middleware
{

/// 鉴权通过后注入请求属性的 key，可通过 current_user_id() 读取
inline constexpr char kUserIdAttribute[] = "user_id";

/**
 * @brief JWT 鉴权中间件（Drogon 版本）
 *
 * 验证 Authorization: Bearer <access_token>，校验签名、issuer 与有效期后，
 * 把 token 中的 user_id 注入 req->attributes()。
 */
class AuthMiddleware : public drogon::HttpMiddleware<AuthMiddleware, false>
{
public:
    AuthMiddleware(std::string access_jwt_secret, std::string jwt_issuer);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;

private:
    infra::TokenSigner m_signer;
};

/// 读取 AuthMiddleware 注入的 user_id；未鉴权时返回空字符串
std::string current_user_id(const drogon::HttpRequestPtr& req);

}  // namespace middleware
