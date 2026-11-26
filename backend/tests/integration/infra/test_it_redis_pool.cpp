//-----------------------------------------------------------------------------
// 文件: test_it_redis_pool.cpp
// 描述: L4 组件层 —— Redis 连接池集成行为（无 Redis 时整组用例确定性跳过）。
//-----------------------------------------------------------------------------
#include <string>

#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "redis-pool/RedisConnGuard.h"
#include "redis-pool/RedisConnectionPool.h"

namespace
{

bool redis_ready()
{
    return it::redis_available();
}

rcp::RedisConnectionPool& test_pool()
{
    static bool initialized = false;
    if (!initialized)
    {
        it::load_config();
        const auto& cfg = infra::AppConfig::instance().redis();

        rcp::RedisConnectionConfig redis_cfg;
        redis_cfg.host     = cfg.host;
        redis_cfg.port     = cfg.port;
        redis_cfg.password = cfg.password;
        redis_cfg.db       = cfg.db;

        rcp::RedisConnectionPool::instance().init(redis_cfg, 1, 4, 30, 2000, 1);
        initialized = true;
    }
    return rcp::RedisConnectionPool::instance();
}

}  // namespace

TEST_CASE("redis_pool: 获取连接并 PING" * doctest::skip(!redis_ready()))
{
    auto guard = test_pool().get_connection();
    REQUIRE(static_cast<bool>(guard));
    CHECK(guard->ping() == "PONG");
}

TEST_CASE("redis_pool: SET/GET/DEL 基本读写" * doctest::skip(!redis_ready()))
{
    auto guard = test_pool().get_connection();
    REQUIRE(static_cast<bool>(guard));

    const std::string key = "it_pool_key";
    guard->set(key, "value-123");
    auto value = guard->get(key);
    REQUIRE(value.has_value());
    CHECK(*value == "value-123");

    CHECK(guard->del(key) == 1);
    CHECK_FALSE(guard->get(key).has_value());
}

TEST_CASE("redis_pool: flushdb 清理测试库" * doctest::skip(!redis_ready()))
{
    auto guard = test_pool().get_connection();
    REQUIRE(static_cast<bool>(guard));

    guard->set("it_flush_probe", "1");
    guard->flushdb();
    CHECK_FALSE(guard->exists("it_flush_probe"));
}

TEST_CASE("redis_pool: 连接可多次借还" * doctest::skip(!redis_ready()))
{
    for (int i = 0; i < 5; ++i)
    {
        auto guard = test_pool().get_connection();
        REQUIRE(static_cast<bool>(guard));
        CHECK(guard->ping() == "PONG");
    }
}
