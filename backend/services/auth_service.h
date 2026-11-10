#pragma once

#include <cstdint>
#include <string>

#include "auth/token_signer.h"

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
                const infra::JwtConfig& jwt_config);

    /// @throws common::AppException
    LoginResult login(const std::string& user_id, const std::string& password);

    /// @throws common::AppException
    bool signUp(const std::string& user_id, const std::string& passward);

    /// @throws common::AppException
    bool logout();

    /// @throws common::AppException
    /// @param refresh_token 是否更新 refresh_token
    LoginResult refresh_token(bool refresh_token);

    bool logoff();

private:
    repository::AccountRepository&   m_account_repo;
    repository::BlacklistRepository& m_blacklist_repo;
    infra::TokenSigner               m_access_signer;
    infra::TokenSigner               m_refresh_signer;
    int64_t                          m_access_ttl_seconds;
    int64_t                          m_refresh_ttl_seconds;
};

}  // namespace service
