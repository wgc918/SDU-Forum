#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "repositories/token_repository.h"

namespace middleware
{

/// 鉴权通过后注入请求属性的 key，可通过 current_*() 读取
inline constexpr char kAdminIdAttribute[]   = "admin_id";
inline constexpr char kAdminRoleAttribute[] = "role";

/**
 * @brief JWT 鉴权中间件（Drogon 版本）
 *
 * 验证 Authorization: Bearer <access_token>，校验签名、issuer 与有效期后，
 * 把 token 中的 sub 注入 req->attributes()。
 */
class AdminAuthMiddleware
{
public:
    AdminAuthMiddleware(const infra::JwtConfig& config, repository::TokenRepository& token_repo,
                        bool is_access);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb);

private:
    infra::TokenSigner          m_refresh_signer;
    infra::TokenSigner          m_access_signer;
    repository::TokenRepository m_token_repo;
    bool                        m_is_access;  // ture 校验access_token ,否则校验refresh_token
};

class AdminAccessAuthMiddleware : public drogon::HttpMiddleware<AdminAccessAuthMiddleware, false>,
                                  public AdminAuthMiddleware
{
public:
    AdminAccessAuthMiddleware(const infra::JwtConfig&      config,
                              repository::TokenRepository& token_repo);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

class AdminRefreshAuthMiddleware : public drogon::HttpMiddleware<AdminRefreshAuthMiddleware, false>,
                                   public AdminAuthMiddleware
{
public:
    AdminRefreshAuthMiddleware(const infra::JwtConfig&      config,
                               repository::TokenRepository& token_repo);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

/// 读取 AuthMiddleware 注入的 admin_id；未鉴权时返回空字符串
std::string current_admin_id(const drogon::HttpRequestPtr& req);

}  // namespace middleware
