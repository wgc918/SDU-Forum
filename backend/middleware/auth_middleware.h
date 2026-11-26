#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <cstdint>
#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "repositories/account_repository.h"
#include "repositories/token_repository.h"
#include "services/admin/ban_service.h"

namespace middleware
{

/// 鉴权通过后注入请求属性的 key，可通过 current_public_id() 读取
inline constexpr char kPublicIdAttribute[]  = "public_id";
inline constexpr char kUserRoleAttribute[]  = "role";
inline constexpr char kUserIdAttribute[]    = "user_id";
inline constexpr char KAccountIdAttribute[] = "account_id";

/**
 * @brief JWT 鉴权中间件（Drogon 版本）
 *
 * 验证 Authorization: Bearer <access_token>，校验签名、issuer 与有效期后，
 * 把 token 中的 user_id 注入 req->attributes()。
 */
class AuthMiddleware
{
public:
    AuthMiddleware(const infra::JwtConfig& config, repository::TokenRepository& token_repo,
                   repository::AccountRepository& account_repo,
                   admin::service::BanService& ban_service, bool is_access);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb);

private:
    infra::TokenSigner             m_refresh_signer;
    infra::TokenSigner             m_access_signer;
    repository::TokenRepository&   m_token_repo;
    repository::AccountRepository& m_account_repo;
    admin::service::BanService&    m_ban_service;
    bool                           m_is_access;  // ture 校验access_token ,否则校验refresh_token
};

class AccessAuthMiddleware : public drogon::HttpMiddleware<AccessAuthMiddleware, false>,
                             public AuthMiddleware
{
public:
    AccessAuthMiddleware(const infra::JwtConfig& config, repository::TokenRepository& token_repo,
                         repository::AccountRepository& account_repo,
                         admin::service::BanService&    ban_service);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

class RefreshAuthMiddleware : public drogon::HttpMiddleware<RefreshAuthMiddleware, false>,
                              public AuthMiddleware
{
public:
    RefreshAuthMiddleware(const infra::JwtConfig& config, repository::TokenRepository& token_repo,
                          repository::AccountRepository& account_repo,
                          admin::service::BanService&    ban_service);

    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

/// 读取 AuthMiddleware 注入的 user_id；未鉴权时返回空字符串
std::string current_public_id(const drogon::HttpRequestPtr& req);

std::string current_user_id(const drogon::HttpRequestPtr& req);

uint64_t current_account_id(const drogon::HttpRequestPtr& req);

}  // namespace middleware
