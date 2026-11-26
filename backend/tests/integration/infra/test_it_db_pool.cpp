//-----------------------------------------------------------------------------
// 文件: test_it_db_pool.cpp
// 描述: L4 组件层 —— 真实 MySQL 连接池与连接提供者的集成行为。
//-----------------------------------------------------------------------------
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "database/IResultSet.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"

TEST_CASE("db_pool: 获取连接并执行查询")
{
    it::load_config();
    auto& provider = it::db();

    auto conn = provider.acquire();
    REQUIRE(conn != nullptr);

    conn->prepare_statement("SELECT 1 AS v");
    auto result = conn->execute_query();
    REQUIRE(result != nullptr);
    REQUIRE(result->next());
    CHECK(result->get_uint64("v") == 1);
    CHECK_FALSE(result->next());
}

TEST_CASE("db_pool: 绑定参数查询")
{
    it::load_config();
    auto& provider = it::db();

    auto conn = provider.acquire();
    REQUIRE(conn != nullptr);

    conn->prepare_statement("SELECT COUNT(*) AS v FROM `it_account` WHERE user_id = ?");
    conn->set_string(1, "__it_no_such_user__");
    auto result = conn->execute_query();
    REQUIRE(result != nullptr);
    REQUIRE(result->next());
    CHECK(result->get_uint64("v") == 0);
}

TEST_CASE("db_pool: 更新语句返回受影响行数")
{
    it::load_config();
    const int affected =
        it::exec("UPDATE `it_admin_account` SET password_hash = password_hash "
                 "WHERE admin_id = '__it_no_such_admin__'");
    CHECK(affected == 0);
}

TEST_CASE("db_pool: 并发获取连接")
{
    it::load_config();
    auto& provider = it::db();

    // 先做一次初始化，避免多线程同时触发懒初始化
    REQUIRE(provider.acquire() != nullptr);

    constexpr int      kThreads = 4;
    std::atomic<int>   ok{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i)
    {
        threads.emplace_back(
            [&provider, &ok]()
            {
                auto conn = provider.acquire();
                if (!conn)
                    return;
                conn->prepare_statement("SELECT 42 AS v");
                auto result = conn->execute_query();
                if (result && result->next() && result->get_uint64("v") == 42)
                    ok.fetch_add(1);
            });
    }
    for (auto& thread : threads)
        thread.join();

    CHECK(ok.load() == kThreads);
}

TEST_CASE("db_pool: it_ 测试表已由夹具创建")
{
    it::load_config();
    const uint64_t count = it::query_u64(
        "SELECT COUNT(*) AS v FROM information_schema.tables "
        "WHERE table_schema = DATABASE() AND table_name LIKE 'it\\_%'");
    CHECK(count == 4);
}
