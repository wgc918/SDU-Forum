#include "services/auth_service.h"

#include <json/json.h>

#include <utility>

#include "common/errors.h"
#include "config/app_config.h"
#include "log.h"
#include "repositories/repository.h"
#include "security/password_hasher.h"

namespace service
{

AuthService::AuthService(repository::AccountRepository&   account,
                         repository::BlacklistRepository& blacklist,
                         const infra::JwtConfig&          jwt_config)
    : m_account(account),
      m_blacklist(blacklist),
      m_access_signer(jwt_config.access_secret, jwt_config.issuer),
      m_refresh_signer(jwt_config.refresh_secret, jwt_config.issuer),
      m_access_ttl_seconds(static_cast<int64_t>(jwt_config.access_token_minutes) * 60),
      m_refresh_ttl_seconds(static_cast<int64_t>(jwt_config.refresh_token_hours) * 3600)
{
}

LoginResult AuthService::login(const std::string& user_id, const std::string& password)
{
    if (user_id.empty() || password.empty())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password must not be empty");

    if (auto ban = m_blacklist.find_active_by_user_id(user_id))
    {
        Json::Value details;
        details["reason"]   = ban->reason;
        details["end_time"] = ban->end_time;
        throw common::AppException(common::ErrorCode::AccountBanned, "account is banned", details);
    }

    auto account = m_account.find_account_by_user_id(user_id);

    // 账号不存在与密码错误返回同一结果，避免泄露账号是否存在
    bool credentials_valid =
        account.has_value() &&
        infra::verify_password(password, infra::PasswordHash{account->password_hash, account->salt,
                                                             account->iterations});
    if (!credentials_valid)
    {
        LOGW("login failed: user_id=%s", user_id.c_str());
        throw common::AppException(common::ErrorCode::InvalidCredentials,
                                   "user id or password is incorrect");
    }

    auto access  = m_access_signer.sign(user_id, m_access_ttl_seconds);
    auto refresh = m_refresh_signer.sign(user_id, m_refresh_ttl_seconds);

    LOGI("login succeeded: user_id=%s", user_id.c_str());

    LoginResult result;
    result.user_id           = user_id;
    result.access_token      = std::move(access.value);
    result.access_token_exp  = access.expires_at;
    result.refresh_token     = std::move(refresh.value);
    result.refresh_token_exp = refresh.expires_at;
    return result;
}

}  // namespace service
