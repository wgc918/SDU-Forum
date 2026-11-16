/**
 * @file test_token_repository.cpp
 * @brief 基于 doctest 的 TokenRepository 测试套件
 *
 * 测试覆盖范围:
 * - hash_secret 配置读取
 * - find_access_token 缓存命中、未命中、连接不可用
 * - insert_access_token 写入成功、写入被拒、连接不可用、TTL 转发
 * - insert_refresh_token 插入成功、零行、连接不可用、SQL 异常
 * - find_refresh_token 缓存命中、回源查询、缓存回写、未命中、异常分支
 * - 五类测试替身：Dummy / Stub / Spy / Mock / Fake
 */

#include <string>
#include <utility>
#include <vector>

#include "doctest.h"

#include "common/errors.h"
#include "repositories/token_repository.h"
#include "support/test_doubles.h"

using repository::TokenRecord;
using repository::TokenRepository;
using repository::TokenStoreConfig;

namespace
{

/**
 * @brief 构造单元测试使用的 token 存储配置
 */
TokenStoreConfig make_config()
{
    TokenStoreConfig config;
    config.refresh_token_table = "refresh_token_it";
    config.hash_secret         = "unit-test-secret";
    config.refresh_ttl_s       = 3600;
    config.access_ttl_s        = 600;
    return config;
}

/**
 * @brief 判断可调用对象是否抛出指定错误码的 AppException
 */
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

// ============================================
// 测试套件: 配置读取
// ============================================

TEST_SUITE("TokenRepository")
{
    TEST_CASE("hash_secret returns configured secret")
    {
        // Dummy: 两个提供者仅用于满足构造参数，本用例不访问它们
        test_support::DummyDbProvider    db;
        test_support::DummyCacheProvider cache;
        TokenRepository                  repo(db, cache, make_config());

        CHECK(repo.hash_secret() == "unit-test-secret");
    }

    // ============================================
    // 测试套件: access_token 缓存读写
    // ============================================

    TEST_CASE("find_access_token returns true on cache hit")
    {
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;
        cache.fake().put("access-hash", "access-hash", 600);

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_access_token("access-hash"));
    }

    TEST_CASE("find_access_token returns false on cache miss")
    {
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK_FALSE(repo.find_access_token("access-hash"));
    }

    TEST_CASE("find_access_token throws RedisError when cache unavailable")
    {
        test_support::DummyDbProvider          db;
        test_support::UnavailableCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_access_token("access-hash"); },
                               common::ErrorCode::RedisError));
    }

    TEST_CASE("insert_access_token stores token with configured ttl")
    {
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.insert_access_token({"access-hash", 1700000000}));
        CHECK(cache.fake().contains("access-hash"));
        CHECK(cache.fake().ttl_of("access-hash") == 600);
    }

    TEST_CASE("insert_access_token returns false when cache rejects write")
    {
        test_support::DummyDbProvider      db;
        test_support::StubCacheProvider    cache;
        cache.connection().state().set_result = false;

        TokenRepository repo(db, cache, make_config());

        CHECK_FALSE(repo.insert_access_token({"access-hash", 1700000000}));
    }

    TEST_CASE("insert_access_token throws RedisError when cache unavailable")
    {
        test_support::DummyDbProvider          db;
        test_support::UnavailableCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code(
            [&repo] { repo.insert_access_token({"access-hash", 1700000000}); },
            common::ErrorCode::RedisError));
    }

    // ============================================
    // 测试套件: refresh_token 数据库写入
    // ============================================

    TEST_CASE("insert_refresh_token binds hash and expiry datetime")
    {
        // Dummy: 插入流程不访问缓存
        test_support::DummyCacheProvider cache;
        // Spy: 记录 SQL 与绑定参数，用于断言交互
        test_support::SpyDbProvider db;
        db.connection().state().update_rows = 1;

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.insert_refresh_token({"refresh-hash", 1700000000}));
        CHECK(db.connection().state().prepare_calls == 1);
        CHECK(db.connection().state().prepared_sql ==
              "INSERT INTO refresh_token_it (token_hash,expires_at) VALUES (?,?)");
        REQUIRE(db.connection().state().string_params.size() == 1);
        CHECK(db.connection().state().string_params[0].first == 1);
        CHECK(db.connection().state().string_params[0].second == "refresh-hash");
        REQUIRE(db.connection().state().datetime_params.size() == 1);
        CHECK(db.connection().state().datetime_params[0].first == 2);
        CHECK(db.connection().state().datetime_params[0].second == "2023-11-14 22:13:20");
        CHECK(db.connection().state().update_calls == 1);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "bind_datetime", "update"});
    }

    TEST_CASE("insert_refresh_token returns false when no row inserted")
    {
        test_support::DummyCacheProvider cache;
        test_support::StubDbProvider     db;
        db.connection().state().update_rows = 0;

        TokenRepository repo(db, cache, make_config());

        CHECK_FALSE(repo.insert_refresh_token({"refresh-hash", 1700000000}));
    }

    TEST_CASE("insert_refresh_token throws DatabaseError when db unavailable")
    {
        test_support::DummyCacheProvider    cache;
        test_support::UnavailableDbProvider db;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code(
            [&repo] { repo.insert_refresh_token({"refresh-hash", 1700000000}); },
            common::ErrorCode::DatabaseError));
    }

    TEST_CASE("insert_refresh_token throws DatabaseError on sql failure")
    {
        test_support::DummyCacheProvider cache;
        test_support::StubDbProvider     db;
        db.connection().state().throw_on_update = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code(
            [&repo] { repo.insert_refresh_token({"refresh-hash", 1700000000}); },
            common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: refresh_token 查询与缓存回写
    // ============================================

    TEST_CASE("find_refresh_token returns true on cache hit without db access")
    {
        // Dummy: 命中缓存时不应访问数据库
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;
        cache.fake().put("refresh-hash", "refresh-hash", 3600);

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_refresh_token("refresh-hash"));
    }

    TEST_CASE("find_refresh_token loads from db and writes back to cache")
    {
        // Stub: 固定返回“存在一行”
        test_support::StubDbProvider db;
        db.connection().state().has_row = true;
        // Mock: 预置缓存回写契约，调用后统一校验
        test_support::MockCacheProvider cache;
        cache.mock().expect_set("refresh-hash", "refresh-hash", 3600);

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_refresh_token("refresh-hash"));
        cache.mock().verify();
        CHECK(cache.mock().state().events == std::vector<std::string>{"get", "set"});
    }

    TEST_CASE("find_refresh_token returns false and skips cache on db miss")
    {
        test_support::StubDbProvider db;
        db.connection().state().has_row = false;
        test_support::MockCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK_FALSE(repo.find_refresh_token("refresh-hash"));
        cache.mock().verify();
    }

    TEST_CASE("find_refresh_token queries table and binds token hash")
    {
        // Spy: 断言 SQL 语句与绑定参数
        test_support::SpyDbProvider db;
        db.connection().state().has_row = true;
        test_support::FakeCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_refresh_token("refresh-hash"));
        CHECK(db.connection().state().prepare_calls == 1);
        CHECK(db.connection().state().prepared_sql ==
              "SELECT id FROM refresh_token_it WHERE token_hash=?");
        REQUIRE(db.connection().state().string_params.size() == 1);
        CHECK(db.connection().state().string_params[0].first == 1);
        CHECK(db.connection().state().string_params[0].second == "refresh-hash");
        CHECK(db.connection().state().query_calls == 1);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "query"});
    }

    TEST_CASE("find_refresh_token throws RedisError when cache unavailable")
    {
        test_support::DummyDbProvider          db;
        test_support::UnavailableCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_refresh_token("refresh-hash"); },
                               common::ErrorCode::RedisError));
    }

    TEST_CASE("find_refresh_token throws DatabaseError when db unavailable")
    {
        test_support::UnavailableDbProvider db;
        test_support::FakeCacheProvider     cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_refresh_token("refresh-hash"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_refresh_token throws DatabaseError on sql failure")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_query = true;
        test_support::FakeCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_refresh_token("refresh-hash"); },
                               common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: 空结果集与缓存运行时异常
    // ============================================

    TEST_CASE("find_refresh_token throws DatabaseError when query result is null")
    {
        test_support::StubDbProvider db;
        db.connection().state().return_null_result_set = true;
        test_support::FakeCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_refresh_token("refresh-hash"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_refresh_token throws RedisError when cache get throws")
    {
        test_support::DummyDbProvider db;
        test_support::MockCacheProvider cache;
        cache.mock().state().throw_on_get = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_refresh_token("refresh-hash"); },
                               common::ErrorCode::RedisError));
    }

    TEST_CASE("find_access_token throws RedisError when cache get throws")
    {
        test_support::DummyDbProvider db;
        test_support::StubCacheProvider cache;
        cache.connection().state().throw_on_get = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_access_token("access-hash"); },
                               common::ErrorCode::RedisError));
    }

    TEST_CASE("insert_access_token throws RedisError when cache set throws")
    {
        test_support::DummyDbProvider db;
        test_support::StubCacheProvider cache;
        cache.connection().state().throw_on_set = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code(
            [&repo] { repo.insert_access_token({"access-hash", 1700000000}); },
            common::ErrorCode::RedisError));
    }

    TEST_CASE("find_refresh_token throws RedisError when cache set throws")
    {
        test_support::StubDbProvider db;
        db.connection().state().has_row = true;
        test_support::MockCacheProvider cache;
        cache.mock().state().throw_on_set = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code([&repo] { repo.find_refresh_token("refresh-hash"); },
                               common::ErrorCode::RedisError));
    }

    // ============================================
    // 测试套件: 缓存回写失败语义
    // ============================================

    TEST_CASE("find_refresh_token returns true when cache write-back fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().has_row = true;
        test_support::MockCacheProvider cache;
        cache.mock().expect_set("refresh-hash", "refresh-hash", 3600);
        cache.mock().state().set_result = false;

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_refresh_token("refresh-hash"));
        cache.mock().verify();
    }

    // ============================================
    // 测试套件: 数据库 prepare / bind 异常
    // ============================================

    TEST_CASE("insert_refresh_token throws DatabaseError when prepare fails")
    {
        test_support::DummyCacheProvider cache;
        test_support::StubDbProvider db;
        db.connection().state().throw_on_prepare = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code(
            [&repo] { repo.insert_refresh_token({"refresh-hash", 1700000000}); },
            common::ErrorCode::DatabaseError));
    }

    TEST_CASE("insert_refresh_token throws DatabaseError when binding fails")
    {
        test_support::DummyCacheProvider cache;
        test_support::StubDbProvider db;
        db.connection().state().throw_on_set_string = true;

        TokenRepository repo(db, cache, make_config());

        CHECK(throws_with_code(
            [&repo] { repo.insert_refresh_token({"refresh-hash", 1700000000}); },
            common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: 缓存值、TTL 与负向交互
    // ============================================

    TEST_CASE("insert_access_token stores the exact token value")
    {
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.insert_access_token({"access-hash", 1700000000}));
        auto stored = cache.fake().get("access-hash");
        REQUIRE(stored.has_value());
        CHECK(*stored == "access-hash");
    }

    TEST_CASE("find_access_token returns true for empty-string value")
    {
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;
        cache.fake().put("access-hash", "", 600);

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_access_token("access-hash"));
    }

    TEST_CASE("insert_access_token forwards zero ttl")
    {
        test_support::DummyDbProvider db;
        test_support::FakeCacheProvider cache;
        TokenStoreConfig config = make_config();
        config.access_ttl_s     = 0;

        TokenRepository repo(db, cache, config);

        CHECK(repo.insert_access_token({"access-hash", 1700000000}));
        CHECK(cache.fake().ttl_of("access-hash") == 0);
    }

    TEST_CASE("find_refresh_token does not write cache on cache hit")
    {
        test_support::DummyDbProvider db;
        test_support::MockCacheProvider cache;
        cache.mock().state().get_result = "refresh-hash";

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.find_refresh_token("refresh-hash"));
        CHECK(cache.mock().state().set_calls.empty());
        CHECK(cache.mock().state().last_get_key == "refresh-hash");
        CHECK(cache.mock().state().events == std::vector<std::string>{"get"});
        cache.mock().verify();
    }

    TEST_CASE("find_access_token does not write cache")
    {
        test_support::DummyDbProvider db;
        test_support::MockCacheProvider cache;

        TokenRepository repo(db, cache, make_config());

        CHECK_FALSE(repo.find_access_token("access-hash"));
        CHECK(cache.mock().state().set_calls.empty());
        CHECK(cache.mock().state().last_get_key == "access-hash");
        CHECK(cache.mock().state().events == std::vector<std::string>{"get"});
        cache.mock().verify();
    }

    TEST_CASE("insert_refresh_token does not execute query")
    {
        test_support::DummyCacheProvider cache;
        test_support::SpyDbProvider db;
        db.connection().state().update_rows = 1;

        TokenRepository repo(db, cache, make_config());

        CHECK(repo.insert_refresh_token({"refresh-hash", 1700000000}));
        CHECK(db.connection().state().query_calls == 0);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "bind_datetime", "update"});
    }
}
