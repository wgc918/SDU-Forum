/**
 * @file test_auth_service.cpp
 * @brief 基于 doctest 的 AuthService 测试套件
 *
 * 测试覆盖范围:
 * - login 凭据校验、双 Token 签发、账号不存在/密码错误/哈希损坏、数据库异常
 * - signUp 公开 ID 生成、密码哈希落库、重复账号、插入失败、数据库异常
 * - logout 旧 Token 拒绝、签名校验、双 Token 吊销、吊销失败
 * - refresh_token 刷新/不刷新分支、旧 Token 吊销、缓存写失败降级、数据库异常
 * - logoff 旧 Token 拒绝、账号删除与双 Token 吊销、删除/写入失败
 * - 复用 test_doubles.h 中的 Stub / Spy / Fake / Unavailable 替身构造真实仓库
 */

#include <cstdint>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

#include "auth/token_signer.h"
#include "common/errors.h"
#include "config/app_config.h"
#include "conn-pool/ConnectionPool.h"
#include "doctest.h"
#include "repositories/account_repository.h"
#include "repositories/blacklist_repository.h"
#include "repositories/token_repository.h"
#include "security/password_hasher.h"
#include "services/auth_service.h"
#include "support/test_doubles.h"
#include "utils/utils.h"

namespace
{

constexpr const char* kAccountTable      = "account_it";
constexpr const char* kBlacklistTable    = "blacklist_it";
constexpr const char* kRefreshTable      = "refresh_token_it";
constexpr const char* kHashSecret        = "unit-test-secret";
constexpr const char* kIssuer            = "sdu-forum-test";
constexpr const char* kAccessSecret      = "unit-test-access-secret";
constexpr const char* kRefreshSecret     = "unit-test-refresh-secret";
constexpr int         kAccessTtlSeconds  = 60;
constexpr int         kRefreshTtlSeconds = 3600;
constexpr int         kTestIterations    = 1000;

/// 构造 TokenRepository 使用的存储配置
repository::TokenStoreConfig make_token_config()
{
    repository::TokenStoreConfig config;
    config.refresh_token_table = kRefreshTable;
    config.hash_secret         = kHashSecret;
    config.refresh_ttl_s       = kRefreshTtlSeconds;
    config.access_ttl_s        = kAccessTtlSeconds;
    return config;
}

/// 构造 AuthService 使用的 JWT 配置
infra::JwtConfig make_jwt_config()
{
    infra::JwtConfig config;
    config.issuer               = kIssuer;
    config.access_secret        = kAccessSecret;
    config.refresh_secret       = kRefreshSecret;
    config.access_token_minutes = kAccessTtlSeconds / 60;
    config.refresh_token_hours  = kRefreshTtlSeconds / 3600;
    return config;
}

/// BlacklistRepository 需要连接池引用；被测代码从不触发其查询，使用共享单例仅为满足构造
dcp::ConnectionPool& shared_db_pool()
{
    return dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL);
}

/// 计算 token 在仓库中使用的存储键
std::string hash_of(const std::string& token)
{
    return utils::hmac_sha256_base64url(kHashSecret, token);
}

infra::SignedToken sign_access(const std::string& sub)
{
    return infra::TokenSigner(kAccessSecret, kIssuer)
        .sign(sub, infra::kUserRole, kAccessTtlSeconds);
}

infra::SignedToken sign_refresh(const std::string& sub)
{
    return infra::TokenSigner(kRefreshSecret, kIssuer)
        .sign(sub, infra::kUserRole, kRefreshTtlSeconds);
}

std::optional<infra::TokenPayload> verify_access(const std::string& token)
{
    return infra::TokenSigner(kAccessSecret, kIssuer).verify(token);
}

std::optional<infra::TokenPayload> verify_refresh(const std::string& token)
{
    return infra::TokenSigner(kRefreshSecret, kIssuer).verify(token);
}

int64_t now_seconds()
{
    return static_cast<int64_t>(std::time(nullptr));
}

/// 为结果集替身填充一条账号记录
void fill_account_row(test_support::StubRowValues& row, const std::string& public_id,
                      const std::string& password)
{
    const infra::PasswordHash stored = infra::hash_password(password, kTestIterations);
    row.uint64_columns["id"]            = 1;
    row.string_columns["public_id"]     = public_id;
    row.string_columns["password_hash"] = stored.hash;
    row.string_columns["salt"]          = stored.salt;
    row.int_columns["iterations"]       = stored.iterations;
}

/// 让 Stub/Spy 数据库替身返回指定账号
template <typename DbProvider>
void provide_valid_account(DbProvider& db, const std::string& public_id, const std::string& password)
{
    db.connection().state().has_row = true;
    fill_account_row(*db.connection().state().row, public_id, password);
}

/**
 * @brief 组装被测 AuthService 及其全部依赖
 * @tparam DbProvider 数据库提供者替身类型
 * @tparam CacheProvider 缓存提供者替身类型
 */
template <typename DbProvider, typename CacheProvider>
struct AuthEnv
{
    DbProvider                      db;
    CacheProvider                   cache;
    repository::AccountRepository   account;
    repository::BlacklistRepository blacklist;
    repository::TokenRepository     token;
    service::AuthService            auth;

    AuthEnv()
        : account(db, kAccountTable),
          blacklist(shared_db_pool(), kBlacklistTable),
          token(db, cache, make_token_config()),
          auth(account, blacklist, token, make_jwt_config())
    {
    }
};

/// 判断可调用对象是否抛出指定错误码的 AppException
template <typename Callable>
bool throws_with_code(Callable&& callable, common::ErrorCode expected)
{
    try
    {
        callable();
    }
    catch (const common::AppException& exception)
    {
        return exception.code() == expected;
    }
    catch (...)
    {
        return false;
    }
    return false;
}

}  // namespace

TEST_SUITE("AuthService")
{
    // ============================================
    // 测试套件: login
    // ============================================

    TEST_CASE("login returns public id and both tokens on valid credentials")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        provide_valid_account(env.db, "usr_alice", "secret");

        auto result = env.auth.login("alice", "secret");

        CHECK(result.public_id == "usr_alice");
        CHECK_FALSE(result.access_token.empty());
        CHECK_FALSE(result.refresh_token.empty());
    }

    TEST_CASE("login signs access token with user role and configured ttl")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        provide_valid_account(env.db, "usr_alice", "secret");

        auto result = env.auth.login("alice", "secret");
        auto payload = verify_access(result.access_token);

        REQUIRE(payload.has_value());
        CHECK(payload->sub == "usr_alice");
        CHECK(payload->role == infra::kUserRole);
        CHECK(result.access_token_exp == payload->expires_at);
        CHECK(payload->expires_at - now_seconds() <= kAccessTtlSeconds + 2);
        CHECK(payload->expires_at - now_seconds() >= kAccessTtlSeconds - 2);
    }

    TEST_CASE("login signs refresh token with user role and configured ttl")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        provide_valid_account(env.db, "usr_alice", "secret");

        auto result = env.auth.login("alice", "secret");
        auto payload = verify_refresh(result.refresh_token);

        REQUIRE(payload.has_value());
        CHECK(payload->sub == "usr_alice");
        CHECK(payload->role == infra::kUserRole);
        CHECK(result.refresh_token_exp == payload->expires_at);
        CHECK(payload->expires_at - now_seconds() <= kRefreshTtlSeconds + 2);
        CHECK(payload->expires_at - now_seconds() >= kRefreshTtlSeconds - 2);
    }

    TEST_CASE("login throws InvalidCredentials when password does not match")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        provide_valid_account(env.db, "usr_alice", "secret");

        CHECK(throws_with_code([&env] { env.auth.login("alice", "wrong"); },
                               common::ErrorCode::InvalidCredentials));
    }

    TEST_CASE("login throws InvalidCredentials when account does not exist")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row = false;

        CHECK(throws_with_code([&env] { env.auth.login("nobody", "secret"); },
                               common::ErrorCode::InvalidCredentials));
    }

    TEST_CASE("login throws InvalidCredentials when stored hash is malformed")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        provide_valid_account(env.db, "usr_alice", "secret");
        env.db.connection().state().row->int_columns["iterations"] = 0;

        CHECK(throws_with_code([&env] { env.auth.login("alice", "secret"); },
                               common::ErrorCode::InvalidCredentials));
    }

    TEST_CASE("login throws DatabaseError when db unavailable")
    {
        AuthEnv<test_support::UnavailableDbProvider, test_support::FakeCacheProvider> env;

        CHECK(throws_with_code([&env] { env.auth.login("alice", "secret"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("login throws DatabaseError on sql failure")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().throw_on_query = true;

        CHECK(throws_with_code([&env] { env.auth.login("alice", "secret"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("login queries account table without writing token stores")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::FakeCacheProvider> env;
        provide_valid_account(env.db, "usr_alice", "secret");

        env.auth.login("alice", "secret");

        const auto& state = env.db.connection().state();
        CHECK(state.prepared_sql ==
              "SELECT id, public_id, password_hash, salt, iterations FROM account_it "
              "WHERE user_id = ?");
        REQUIRE(state.string_params.size() == 1);
        CHECK(state.string_params[0].first == 1);
        CHECK(state.string_params[0].second == "alice");
        CHECK(state.query_calls == 1);
        CHECK(state.update_calls == 0);
        CHECK(state.events == std::vector<std::string>{"prepare", "bind_string", "query"});
        CHECK(env.cache.fake().state_ptr()->store.empty());
    }

    // ============================================
    // 测试套件: signUp
    // ============================================

    TEST_CASE("signUp returns generated usr public id")
    {
        AuthEnv<test_support::StubDbProvider, test_support::DummyCacheProvider> env;
        env.db.connection().state().has_row    = false;
        env.db.connection().state().update_rows = 1;

        std::string public_id = env.auth.signUp("alice", "secret");

        CHECK(public_id.rfind("usr_", 0) == 0);
        CHECK(public_id.size() == 24);
    }

    TEST_CASE("signUp stores credentials that verify against the plaintext password")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::DummyCacheProvider> env;
        env.db.connection().state().has_row     = false;
        env.db.connection().state().update_rows = 1;

        std::string public_id = env.auth.signUp("alice", "secret");

        const auto& state = env.db.connection().state();
        REQUIRE(state.string_params.size() == 5);
        CHECK(state.string_params[0].second == "alice");  // 查重绑定
        CHECK(state.string_params[1].second == public_id);
        CHECK(state.string_params[2].second == "alice");
        REQUIRE(state.int_params.size() == 1);
        REQUIRE(state.datetime_params.empty());

        infra::PasswordHash stored{state.string_params[3].second, state.string_params[4].second,
                                   state.int_params[0].second};
        CHECK(infra::verify_password("secret", stored));
        CHECK_FALSE(infra::verify_password("wrong", stored));

        CHECK(state.prepared_sql ==
              "INSERT INTO account_it (public_id,user_id,password_hash,salt,iterations) "
              "VALUES(?,?,?,?,?)");
        CHECK(state.events ==
              std::vector<std::string>{"prepare", "bind_string", "query", "prepare", "bind_string",
                                       "bind_string", "bind_string", "bind_string", "bind_int",
                                       "update"});
    }

    TEST_CASE("signUp throws InvalidParam when user id already exists")
    {
        AuthEnv<test_support::StubDbProvider, test_support::DummyCacheProvider> env;
        env.db.connection().state().has_row = true;

        CHECK(throws_with_code([&env] { env.auth.signUp("alice", "secret"); },
                               common::ErrorCode::InvalidParam));
    }

    TEST_CASE("signUp throws DatabaseError when no account row inserted")
    {
        AuthEnv<test_support::StubDbProvider, test_support::DummyCacheProvider> env;
        env.db.connection().state().has_row     = false;
        env.db.connection().state().update_rows = 0;

        CHECK(throws_with_code([&env] { env.auth.signUp("alice", "secret"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("signUp throws DatabaseError when db unavailable")
    {
        AuthEnv<test_support::UnavailableDbProvider, test_support::DummyCacheProvider> env;

        CHECK(throws_with_code([&env] { env.auth.signUp("alice", "secret"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("signUp throws DatabaseError on sql failure")
    {
        AuthEnv<test_support::StubDbProvider, test_support::DummyCacheProvider> env;
        env.db.connection().state().has_row        = false;
        env.db.connection().state().throw_on_update = true;

        CHECK(throws_with_code([&env] { env.auth.signUp("alice", "secret"); },
                               common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: logout
    // ============================================

    TEST_CASE("logout throws Unauthorized when refresh token is already revoked")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        const auto refresh = sign_refresh("usr_alice");
        env.cache.fake().put(hash_of(refresh.value), hash_of(refresh.value), kRefreshTtlSeconds);

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logout(refresh.value, "access-token"); },
            common::ErrorCode::Unauthorized));
    }

    TEST_CASE("logout throws Unauthorized when refresh token is invalid")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row = false;

        CHECK(throws_with_code([&env] { env.auth.logout("not-a-token", "access-token"); },
                               common::ErrorCode::Unauthorized));
    }

    TEST_CASE("logout persists refresh token and caches access token hash")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row     = false;
        env.db.connection().state().update_rows = 1;
        const auto refresh                      = sign_refresh("usr_alice");
        const auto access                       = sign_access("usr_alice");

        env.auth.logout(refresh.value, access.value);

        const std::string access_hash = hash_of(access.value);
        CHECK(env.cache.fake().contains(access_hash));
        CHECK(env.cache.fake().ttl_of(access_hash) == kAccessTtlSeconds);

        const auto& state = env.db.connection().state();
        REQUIRE(state.string_params.size() == 2);
        CHECK(state.string_params[0].second == hash_of(refresh.value));
        CHECK(state.string_params[1].second == hash_of(refresh.value));
        REQUIRE(state.datetime_params.size() == 1);
        CHECK(state.datetime_params[0].first == 2);
        CHECK(state.datetime_params[0].second == utils::timestamp_to_string(refresh.expires_at));
        CHECK(state.update_calls == 1);
        CHECK(state.events ==
              std::vector<std::string>{"prepare", "bind_string", "query", "prepare", "bind_string",
                                       "bind_datetime", "update"});
    }

    TEST_CASE("logout throws DatabaseError when refresh token cannot be persisted")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row     = false;
        env.db.connection().state().update_rows = 0;
        const auto refresh                      = sign_refresh("usr_alice");

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logout(refresh.value, "access-token"); },
            common::ErrorCode::DatabaseError));
        CHECK(env.cache.fake().state_ptr()->store.empty());
    }

    TEST_CASE("logout throws DatabaseError when access token cannot be cached")
    {
        AuthEnv<test_support::StubDbProvider, test_support::StubCacheProvider> env;
        env.db.connection().state().has_row       = false;
        env.db.connection().state().update_rows   = 1;
        env.cache.connection().state().set_result = false;
        const auto refresh                        = sign_refresh("usr_alice");

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logout(refresh.value, "access-token"); },
            common::ErrorCode::DatabaseError));
    }

    TEST_CASE("logout throws DatabaseError when db unavailable")
    {
        AuthEnv<test_support::UnavailableDbProvider, test_support::FakeCacheProvider> env;
        const auto refresh = sign_refresh("usr_alice");

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logout(refresh.value, "access-token"); },
            common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: refresh_token
    // ============================================

    TEST_CASE("refresh_token throws Unauthorized when refresh token is invalid")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::FakeCacheProvider> env;

        CHECK(throws_with_code(
            [&env] { env.auth.refresh_token("not-a-token", "old-access", true); },
            common::ErrorCode::Unauthorized));
        CHECK(env.db.connection().state().prepare_calls == 0);
        CHECK(env.cache.fake().state_ptr()->store.empty());
    }

    TEST_CASE("refresh_token throws Unauthorized when signed with another key")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        const auto wrong_key_token = sign_access("usr_alice");

        CHECK(throws_with_code(
            [&env, &wrong_key_token] {
                env.auth.refresh_token(wrong_key_token.value, "old-access", true);
            },
            common::ErrorCode::Unauthorized));
    }

    TEST_CASE("refresh_token issues new access and refresh tokens when refreshing")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().update_rows = 1;
        const auto old_refresh                  = sign_refresh("usr_alice");
        const auto old_access                   = sign_access("usr_alice");

        auto result = env.auth.refresh_token(old_refresh.value, old_access.value, true);

        CHECK(result.public_id == "usr_alice");
        REQUIRE(verify_refresh(result.refresh_token).has_value());
        CHECK(verify_refresh(result.refresh_token)->sub == "usr_alice");
        REQUIRE(verify_access(result.access_token).has_value());
        CHECK(verify_access(result.access_token)->sub == "usr_alice");
        CHECK(env.cache.fake().contains(hash_of(old_access.value)));
    }

    TEST_CASE("refresh_token revokes old refresh token with new expiry")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().update_rows = 1;
        const auto old_refresh                  = sign_refresh("usr_alice");
        const auto old_access                   = sign_access("usr_alice");

        auto result = env.auth.refresh_token(old_refresh.value, old_access.value, true);

        const auto new_payload = verify_refresh(result.refresh_token);
        REQUIRE(new_payload.has_value());

        const auto& state = env.db.connection().state();
        CHECK(state.prepared_sql ==
              "INSERT INTO refresh_token_it (token_hash,expires_at) VALUES (?,?)");
        REQUIRE(state.string_params.size() == 1);
        CHECK(state.string_params[0].first == 1);
        CHECK(state.string_params[0].second == hash_of(old_refresh.value));
        REQUIRE(state.datetime_params.size() == 1);
        CHECK(state.datetime_params[0].second ==
              utils::timestamp_to_string(new_payload->expires_at));
        CHECK(state.update_calls == 1);
    }

    TEST_CASE("refresh_token refreshes access token only when not refreshing")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::FakeCacheProvider> env;
        const auto old_refresh = sign_refresh("usr_alice");
        const auto old_access  = sign_access("usr_alice");

        auto result = env.auth.refresh_token(old_refresh.value, old_access.value, false);

        CHECK(result.public_id.empty());
        CHECK(result.refresh_token.empty());
        CHECK_FALSE(result.access_token.empty());
        CHECK(env.db.connection().state().prepare_calls == 0);
        CHECK(env.db.connection().state().update_calls == 0);
        CHECK(env.cache.fake().contains(hash_of(old_access.value)));
    }

    TEST_CASE("refresh_token still returns access token when cache write fails")
    {
        AuthEnv<test_support::StubDbProvider, test_support::StubCacheProvider> env;
        env.cache.connection().state().set_result = false;
        const auto old_refresh                    = sign_refresh("usr_alice");
        const auto old_access                     = sign_access("usr_alice");

        auto result = env.auth.refresh_token(old_refresh.value, old_access.value, false);

        CHECK_FALSE(result.access_token.empty());
        CHECK(verify_access(result.access_token)->sub == "usr_alice");
    }

    TEST_CASE("refresh_token throws DatabaseError when old refresh token cannot be revoked")
    {
        AuthEnv<test_support::UnavailableDbProvider, test_support::FakeCacheProvider> env;
        const auto old_refresh = sign_refresh("usr_alice");
        const auto old_access  = sign_access("usr_alice");

        CHECK(throws_with_code(
            [&env, &old_refresh, &old_access] {
                env.auth.refresh_token(old_refresh.value, old_access.value, true);
            },
            common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: logoff
    // ============================================

    TEST_CASE("logoff throws Unauthorized when refresh token is already revoked")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        const auto refresh = sign_refresh("usr_alice");
        env.cache.fake().put(hash_of(refresh.value), hash_of(refresh.value), kRefreshTtlSeconds);

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logoff("usr_alice", refresh.value, "access-token"); },
            common::ErrorCode::Unauthorized));
    }

    TEST_CASE("logoff throws Unauthorized when refresh token is invalid")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row = false;

        CHECK(throws_with_code(
            [&env] { env.auth.logoff("usr_alice", "not-a-token", "access-token"); },
            common::ErrorCode::Unauthorized));
    }

    TEST_CASE("logoff deletes account and revokes both tokens")
    {
        AuthEnv<test_support::SpyDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row     = false;
        env.db.connection().state().update_rows = 1;
        const auto refresh                      = sign_refresh("usr_alice");
        const auto access                       = sign_access("usr_alice");

        env.auth.logoff("usr_alice", refresh.value, access.value);

        const std::string access_hash = hash_of(access.value);
        CHECK(env.cache.fake().contains(access_hash));
        CHECK(env.cache.fake().ttl_of(access_hash) == kAccessTtlSeconds);

        const auto& state = env.db.connection().state();
        REQUIRE(state.string_params.size() == 3);
        CHECK(state.string_params[0].second == hash_of(refresh.value));
        CHECK(state.string_params[1].second == "usr_alice");
        CHECK(state.string_params[2].second == hash_of(refresh.value));
        CHECK(state.update_calls == 2);
        CHECK(state.events ==
              std::vector<std::string>{"prepare", "bind_string", "query", "prepare", "bind_string",
                                       "update", "prepare", "bind_string", "bind_datetime", "update"});
    }

    TEST_CASE("logoff throws DatabaseError when no account row deleted")
    {
        AuthEnv<test_support::StubDbProvider, test_support::FakeCacheProvider> env;
        env.db.connection().state().has_row     = false;
        env.db.connection().state().update_rows = 0;
        const auto refresh                      = sign_refresh("usr_alice");

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logoff("usr_alice", refresh.value, "access-token"); },
            common::ErrorCode::DatabaseError));
        CHECK(env.cache.fake().state_ptr()->store.empty());
    }

    TEST_CASE("logoff throws DatabaseError when access token cannot be cached")
    {
        AuthEnv<test_support::StubDbProvider, test_support::StubCacheProvider> env;
        env.db.connection().state().has_row       = false;
        env.db.connection().state().update_rows   = 1;
        env.cache.connection().state().set_result = false;
        const auto refresh                        = sign_refresh("usr_alice");

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logoff("usr_alice", refresh.value, "access-token"); },
            common::ErrorCode::DatabaseError));
    }

    TEST_CASE("logoff throws DatabaseError when db unavailable")
    {
        AuthEnv<test_support::UnavailableDbProvider, test_support::FakeCacheProvider> env;
        const auto refresh = sign_refresh("usr_alice");

        CHECK(throws_with_code(
            [&env, &refresh] { env.auth.logoff("usr_alice", refresh.value, "access-token"); },
            common::ErrorCode::DatabaseError));
    }
}
