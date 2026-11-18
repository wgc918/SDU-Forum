/**
 * @file test_account_repository.cpp
 * @brief 基于 doctest 的 AccountRepository 测试套件
 *
 * 测试覆盖范围:
 * - find_account_by_user_id 查到一行、无行、连接不可用、空结果集
 * - find_account_by_user_id prepare / 绑定 / 查询异常
 * - find_account_by_user_id SELECT 语句与参数绑定交互
 * - insert_account_by_user_id 插入成功、零行、连接不可用
 * - insert_account_by_user_id prepare / 字符串绑定 / 整型绑定 / 更新异常
 * - insert_account_by_user_id INSERT 语句与五参数绑定交互
 * - delete_account_by_public_id 删除成功、零行、连接不可用、更新异常
 * - delete_account_by_public_id DELETE 语句与参数绑定交互
 * - 测试替身：Stub / Spy / Unavailable
 */

#include <string>
#include <utility>
#include <vector>

#include "common/errors.h"
#include "doctest.h"
#include "repositories/account_repository.h"
#include "support/test_doubles.h"

using repository::AccountRecord;
using repository::AccountRepository;

namespace
{

constexpr const char* kAccountTable = "account_it";

/**
 * @brief 构造单元测试使用的账号记录
 */
AccountRecord make_account()
{
    AccountRecord account;
    account.id            = 0;
    account.public_id     = "usr_alice";
    account.password_hash = "hash-value";
    account.salt          = "salt-value";
    account.iterations    = 120000;
    return account;
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

TEST_SUITE("AccountRepository")
{
    // ============================================
    // 测试套件: 账号查询
    // ============================================

    TEST_CASE("find_account_by_user_id returns record when row exists")
    {
        // Stub: 固定返回一行，并预置该行的列值
        test_support::StubDbProvider db;
        auto&                        state         = db.connection().state();
        state.has_row                              = true;
        state.row->uint64_columns["id"]            = 42;
        state.row->string_columns["public_id"]     = "usr_alice";
        state.row->string_columns["password_hash"] = "hash-value";
        state.row->string_columns["salt"]          = "salt-value";
        state.row->int_columns["iterations"]       = 120000;

        AccountRepository repo(db, kAccountTable);

        auto found = repo.find_account_by_user_id("alice");

        REQUIRE(found.has_value());
        CHECK(found->id == 42);
        CHECK(found->public_id == "usr_alice");
        CHECK(found->password_hash == "hash-value");
        CHECK(found->salt == "salt-value");
        CHECK(found->iterations == 120000);
    }

    TEST_CASE("find_account_by_user_id returns nullopt when no row")
    {
        test_support::StubDbProvider db;
        db.connection().state().has_row = false;

        AccountRepository repo(db, kAccountTable);

        auto found = repo.find_account_by_user_id("alice");

        CHECK_FALSE(found.has_value());
    }

    TEST_CASE("find_account_by_user_id binds user id and selects credential columns")
    {
        // Spy: 断言 SQL 文本、绑定参数与调用次数
        test_support::SpyDbProvider db;
        db.connection().state().has_row = true;

        AccountRepository repo(db, kAccountTable);

        auto found = repo.find_account_by_user_id("alice");

        CHECK(found.has_value());
        CHECK(db.connection().state().prepared_sql ==
              "SELECT id, public_id, password_hash, salt, iterations FROM account_it "
              "WHERE user_id = ?");
        REQUIRE(db.connection().state().string_params.size() == 1);
        CHECK(db.connection().state().string_params[0].first == 1);
        CHECK(db.connection().state().string_params[0].second == "alice");
        CHECK(db.connection().state().int_params.empty());
        CHECK(db.connection().state().query_calls == 1);
        CHECK(db.connection().state().update_calls == 0);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "query"});
    }

    TEST_CASE("find_account_by_user_id throws DatabaseError when db unavailable")
    {
        test_support::UnavailableDbProvider db;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.find_account_by_user_id("alice"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_account_by_user_id throws DatabaseError on sql failure")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_query = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.find_account_by_user_id("alice"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_account_by_user_id throws DatabaseError when prepare fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_prepare = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.find_account_by_user_id("alice"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_account_by_user_id throws DatabaseError when binding fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_set_string = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.find_account_by_user_id("alice"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_account_by_user_id throws DatabaseError when query result is null")
    {
        test_support::StubDbProvider db;
        db.connection().state().return_null_result_set = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.find_account_by_user_id("alice"); },
                               common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: 账号写入
    // ============================================

    TEST_CASE("insert_account_by_user_id returns true when row inserted")
    {
        test_support::StubDbProvider db;
        db.connection().state().update_rows = 1;

        AccountRepository repo(db, kAccountTable);

        CHECK(repo.insert_account_by_user_id("alice", make_account()));
    }

    TEST_CASE("insert_account_by_user_id returns false when no row inserted")
    {
        test_support::StubDbProvider db;
        db.connection().state().update_rows = 0;

        AccountRepository repo(db, kAccountTable);

        CHECK_FALSE(repo.insert_account_by_user_id("alice", make_account()));
    }

    TEST_CASE("insert_account_by_user_id binds public id user id hash salt iterations")
    {
        test_support::SpyDbProvider db;
        db.connection().state().update_rows = 1;

        AccountRepository repo(db, kAccountTable);

        CHECK(repo.insert_account_by_user_id("alice", make_account()));

        CHECK(db.connection().state().prepared_sql ==
              "INSERT INTO account_it (public_id,user_id,password_hash,salt,iterations) "
              "VALUES(?,?,?,?,?)");

        const auto& params = db.connection().state().string_params;
        REQUIRE(params.size() == 4);
        CHECK(params[0].first == 1);
        CHECK(params[0].second == "usr_alice");
        CHECK(params[1].first == 2);
        CHECK(params[1].second == "alice");
        CHECK(params[2].first == 3);
        CHECK(params[2].second == "hash-value");
        CHECK(params[3].first == 4);
        CHECK(params[3].second == "salt-value");

        REQUIRE(db.connection().state().int_params.size() == 1);
        CHECK(db.connection().state().int_params[0].first == 5);
        CHECK(db.connection().state().int_params[0].second == 120000);

        CHECK(db.connection().state().query_calls == 0);
        CHECK(db.connection().state().update_calls == 1);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "bind_string", "bind_string",
                                       "bind_string", "bind_int", "update"});
    }

    TEST_CASE("insert_account_by_user_id throws DatabaseError when db unavailable")
    {
        test_support::UnavailableDbProvider db;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.insert_account_by_user_id("alice", make_account()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("insert_account_by_user_id throws DatabaseError on sql failure")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_update = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.insert_account_by_user_id("alice", make_account()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("insert_account_by_user_id throws DatabaseError when prepare fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_prepare = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.insert_account_by_user_id("alice", make_account()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("insert_account_by_user_id throws DatabaseError when string binding fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_set_string = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.insert_account_by_user_id("alice", make_account()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("insert_account_by_user_id throws DatabaseError when iteration binding fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_set_int = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.insert_account_by_user_id("alice", make_account()); },
                               common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: 账号删除
    // ============================================

    TEST_CASE("delete_account_by_public_id returns true when row deleted")
    {
        test_support::StubDbProvider db;
        db.connection().state().update_rows = 1;

        AccountRepository repo(db, kAccountTable);

        CHECK(repo.delete_account_by_public_id("usr_alice"));
    }

    TEST_CASE("delete_account_by_public_id returns false when no row deleted")
    {
        test_support::StubDbProvider db;
        db.connection().state().update_rows = 0;

        AccountRepository repo(db, kAccountTable);

        CHECK_FALSE(repo.delete_account_by_public_id("usr_alice"));
    }

    TEST_CASE("delete_account_by_public_id binds public id and targets account table")
    {
        test_support::SpyDbProvider db;
        db.connection().state().update_rows = 1;

        AccountRepository repo(db, kAccountTable);

        CHECK(repo.delete_account_by_public_id("usr_alice"));
        CHECK(db.connection().state().prepared_sql == "DELETE FROM account_it WHERE public_id=?");
        REQUIRE(db.connection().state().string_params.size() == 1);
        CHECK(db.connection().state().string_params[0].first == 1);
        CHECK(db.connection().state().string_params[0].second == "usr_alice");
        CHECK(db.connection().state().query_calls == 0);
        CHECK(db.connection().state().update_calls == 1);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "update"});
    }

    TEST_CASE("delete_account_by_public_id throws DatabaseError when db unavailable")
    {
        test_support::UnavailableDbProvider db;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.delete_account_by_public_id("usr_alice"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("delete_account_by_public_id throws DatabaseError on sql failure")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_update = true;

        AccountRepository repo(db, kAccountTable);

        CHECK(throws_with_code([&repo] { repo.delete_account_by_public_id("usr_alice"); },
                               common::ErrorCode::DatabaseError));
    }
}
