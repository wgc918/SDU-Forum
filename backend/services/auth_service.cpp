#include "services/auth_service.h"

#include <json/json.h>
#include <openssl/rand.h>

#include <utility>

#include "common/errors.h"
#include "config/app_config.h"
#include "log.h"
#include "repositories/account_repository.h"
#include "repositories/repository.h"
#include "security/password_hasher.h"

namespace service
{

AuthService::AuthService(repository::AccountRepository&   account,
                         repository::BlacklistRepository& blacklist,
                         const infra::JwtConfig&          jwt_config)
    : m_account_repo(account),
      m_blacklist_repo(blacklist),
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

    // if (auto ban = m_blacklist_repo.find_active_by_user_id(user_id))
    // {
    //     Json::Value details;
    //     details["reason"]   = ban->reason;
    //     details["end_time"] = ban->end_time;
    //     throw common::AppException(common::ErrorCode::AccountBanned, "account is banned",
    //     details);
    // }

    auto account = m_account_repo.find_account_by_user_id(user_id);

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

    auto access  = m_access_signer.sign(account->public_id, m_access_ttl_seconds);
    auto refresh = m_refresh_signer.sign(account->public_id, m_refresh_ttl_seconds);

    LOGI("login succeeded: user_id=%s", user_id.c_str());

    LoginResult result;
    result.public_id         = account->public_id;
    result.access_token      = std::move(access.value);
    result.access_token_exp  = access.expires_at;
    result.refresh_token     = std::move(refresh.value);
    result.refresh_token_exp = refresh.expires_at;
    return result;
}

// 生成 n 位 Base62 随机串（密码学安全）
static std::string random_base62(size_t n)
{
    static constexpr char kCharset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static constexpr size_t kSize = sizeof(kCharset) - 1;  // 62

    std::string out;
    out.reserve(n);

    // 为消除模偏差：拒绝采样。62 不整除 256，直接 % 62 会有偏。
    // 取 256 - (256 % 62) = 248 作为接受上限。
    constexpr unsigned kLimit = 256 - (256 % kSize);  // 248
    unsigned char      buf[64];

    while (out.size() < n)
    {
        size_t need  = n - out.size();
        size_t chunk = need < sizeof(buf) ? need : sizeof(buf);
        // 多取一些，抵消拒绝采样丢弃的字节
        size_t want = chunk + chunk / 4 + 8;
        if (want > sizeof(buf))
            want = sizeof(buf);
        if (RAND_bytes(buf, static_cast<int>(want)) != 1)
            return "";
        for (size_t i = 0; i < want && out.size() < n; ++i)
        {
            if (buf[i] < kLimit)
                out += kCharset[buf[i] % kSize];
        }
    }
    return out;
}

// 生成对外公开 ID：prefix + '_' + base62
static std::string make_public_id(const std::string& prefix = "usr", size_t rand_len = 20)
{
    if (rand_len < 12 || rand_len > 32)
        throw std::invalid_argument("rand_len must be 12..32");
    return prefix + "_" + random_base62(rand_len);
}

bool AuthService::signUp(const std::string& user_id, const std::string& passward)
{
    if (user_id.empty() || passward.empty())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password must not be empty");
    infra::PasswordHash       hash_passward = infra::hash_password(passward);
    repository::AccountRecord account;
    account.public_id     = make_public_id();
    account.salt          = hash_passward.salt;
    account.iterations    = hash_passward.iterations;
    account.password_hash = hash_passward.hash;

    auto old = m_account_repo.find_account_by_user_id(user_id);
    if (old.has_value())
    {
        LOGW("sign up failed: user_id=%s", user_id.c_str());
        throw common::AppException(common::ErrorCode::InvalidParam, "user_id is existed");
    }

    bool success = m_account_repo.add_account_by_user_id(user_id, account);
    if (success)
        LOGI("sign up succeed: user_id=%s", user_id.c_str());
    else
    {
        LOGW("sign up failed: user_id=%s ,update rows=0", user_id.c_str());
    }
    return success;
}
}  // namespace service
