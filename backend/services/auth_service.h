#pragma once

#include <cstdint>
#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "repositories/token_repository.h"

namespace repository
{
class BlacklistRepository;
class AccountRepository;
}  // namespace repository

namespace infra
{
struct JwtConfig;
}

namespace service
{

struct LoginResult
{
    std::string public_id;
    std::string access_token;
    int64_t     access_token_exp = 0;
    std::string refresh_token;
    int64_t     refresh_token_exp = 0;
};

/// 登录业务编排：封禁校验 -> 密码校验 -> 双 Token 签发
class AuthService
{
public:
    AuthService(repository::AccountRepository& account, repository::BlacklistRepository& blacklist,
                repository::TokenRepository& token, const infra::JwtConfig& jwt_config,
                const infra::AuthConfig& auth_config);

    /// @throws common::AppException
    LoginResult login(const std::string& user_id, const std::string& password);

    /// @throws common::AppException
    bool signUp(const std::string& user_id, const std::string& passward);

    /// @throws common::AppException
    bool logout(const std::string& refresh_token, const std::string access_token);

    /// @throws common::AppException
    /// @param refreshing 是否更新refresh_token
    LoginResult refresh_token(const std::string& refresh_token, const std::string& access_token,
                              bool refreshing);

    bool logoff(const std::string& public_id, const std::string& refresh_token,
                const std::string access_token);

private:
    repository::AccountRepository&   m_account_repo;
    repository::BlacklistRepository& m_blacklist_repo;
    repository::TokenRepository&     m_token_repo;
    infra::TokenSigner               m_access_signer;
    infra::TokenSigner               m_refresh_signer;
    int64_t                          m_access_ttl_seconds;
    int64_t                          m_refresh_ttl_seconds;
    const std::string                m_hash_secret;
};

}  // namespace service
