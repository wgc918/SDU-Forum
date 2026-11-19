#pragma once

#include <cstdint>
#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "repositories/token_repository.h"

namespace repository
{
class AdminAccountRepository;
class TokenRepository;
}  // namespace repository

namespace infra
{
struct JwtConfig;
}

namespace admin::service
{

struct AdminLoginResult
{
    std::string admin_id;
    std::string access_token;
    int64_t     access_token_exp = 0;
    std::string refresh_token;
    int64_t     refresh_token_exp = 0;
};

/// 登录业务编排：封禁校验 -> 密码校验 -> 双 Token 签发
class AdminAuthService
{
public:
    AdminAuthService(repository::AdminAccountRepository& account_repo,
                     repository::TokenRepository& token_repo, const infra::JwtConfig& jwt_config);

    /// @throws common::AppException
    AdminLoginResult login(const std::string& admin_id, const std::string& password);

    /// @throws common::AppException
    void logout(const std::string& refresh_token, const std::string access_token);

    /// @throws common::AppException
    /// @param refreshing 是否更新refresh_token
    AdminLoginResult refresh_token(const std::string& refresh_token,
                                   const std::string& access_token, bool refreshing);

private:
    repository::AdminAccountRepository& m_admin_account_repo;
    repository::TokenRepository&        m_token_repo;
    infra::TokenSigner                  m_access_signer;
    infra::TokenSigner                  m_refresh_signer;
    int64_t                             m_access_ttl_seconds;
    int64_t                             m_refresh_ttl_seconds;
};

}  // namespace admin::service
