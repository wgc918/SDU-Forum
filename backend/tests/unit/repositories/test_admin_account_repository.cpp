/**
 * @file test_admin_account_repository.cpp
 * @brief 基于 doctest 的 AdminAccountRepository 测试套件
 *
 * 测试覆盖范围:
 * - find_admin_account_by_admin_id 查到一行、无行、连接不可用、空结果集
 * - find_admin_account_by_admin_id prepare / 绑定 / 查询异常
 * - find_admin_account_by_admin_id SELECT 语句与参数绑定交互
 * - inisert_admin_account 插入成功、零行、连接不可用
 * - inisert_admin_account prepare / 字符串绑定 / 更新异常
 * - inisert_admin_account INSERT 语句与三参数绑定交互
 * - 测试替身：Stub / Spy / Unavailable
 */

#include <string>
#include <utility>
#include <vector>

#include "common/errors.h"
#include "doctest.h"
#include "repositories/admin_account_repository.h"
#include "support/test_doubles.h"

using repository::AdminAccountRecord;
using repository::AdminAccountRepository;

namespace
{

constexpr const char* kAdminTable = "admin_it";

/**
 * @brief 构造单元测试使用的管理员账号记录
 */
AdminAccountRecord make_record()
{
    AdminAccountRecord record;
    record.id            = 0;
    record.admin_id      = "admin";
    record.password_hash = "hash-value";
    record.salt          = "salt-value";
    record.iterations    = 120000;
    return record;
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

TEST_SUITE("AdminAccountRepository")
{
    // ============================================
    // 测试套件: 管理员账号查询
    // ============================================

    TEST_CASE("find_admin_account_by_admin_id returns record when row exists")
    {
        // Stub: 固定返回一行，并预置该行的列值
        test_support::StubDbProvider db;
        auto&                        state         = db.connection().state();
        state.has_row                              = true;
        state.row->uint64_columns["id"]            = 7;
        state.row->string_columns["password_hash"] = "hash-value";
        state.row->string_columns["salt"]          = "salt-value";
        state.row->int_columns["iterations"]       = 120000;

        AdminAccountRepository repo(db, kAdminTable);

        auto found = repo.find_admin_account_by_admin_id("admin");

        REQUIRE(found.has_value());
        CHECK(found->id == 7);
        CHECK(found->admin_id == "admin");
        CHECK(found->password_hash == "hash-value");
        CHECK(found->salt == "salt-value");
        CHECK(found->iterations == 120000);
    }

    TEST_CASE("find_admin_account_by_admin_id returns nullopt when no row")
    {
        test_support::StubDbProvider db;
        db.connection().state().has_row = false;

        AdminAccountRepository repo(db, kAdminTable);

        auto found = repo.find_admin_account_by_admin_id("admin");

        CHECK_FALSE(found.has_value());
    }

    TEST_CASE("find_admin_account_by_admin_id binds admin id and selects columns")
    {
        // Spy: 断言 SQL 文本、绑定参数与调用次数
        test_support::SpyDbProvider db;
        db.connection().state().has_row = true;

        AdminAccountRepository repo(db, kAdminTable);

        auto found = repo.find_admin_account_by_admin_id("admin");

        CHECK(found.has_value());
        CHECK(db.connection().state().prepared_sql ==
              "SELECT id, password_hash, salt, iterations FROM admin_it WHERE admin_id=?");
        REQUIRE(db.connection().state().string_params.size() == 1);
        CHECK(db.connection().state().string_params[0].first == 1);
        CHECK(db.connection().state().string_params[0].second == "admin");
        CHECK(db.connection().state().int_params.empty());
        CHECK(db.connection().state().query_calls == 1);
        CHECK(db.connection().state().update_calls == 0);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "query"});
    }

    TEST_CASE("find_admin_account_by_admin_id throws DatabaseError when db unavailable")
    {
        test_support::UnavailableDbProvider db;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.find_admin_account_by_admin_id("admin"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_admin_account_by_admin_id throws DatabaseError on sql failure")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_query = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.find_admin_account_by_admin_id("admin"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_admin_account_by_admin_id throws DatabaseError when prepare fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_prepare = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.find_admin_account_by_admin_id("admin"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_admin_account_by_admin_id throws DatabaseError when binding fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_set_string = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.find_admin_account_by_admin_id("admin"); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("find_admin_account_by_admin_id throws DatabaseError when query result is null")
    {
        test_support::StubDbProvider db;
        db.connection().state().return_null_result_set = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.find_admin_account_by_admin_id("admin"); },
                               common::ErrorCode::DatabaseError));
    }

    // ============================================
    // 测试套件: 管理员账号写入
    // ============================================

    TEST_CASE("inisert_admin_account returns true when row inserted")
    {
        test_support::StubDbProvider db;
        db.connection().state().update_rows = 1;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(repo.inisert_admin_account(make_record()));
    }

    TEST_CASE("inisert_admin_account returns false when no row inserted")
    {
        test_support::StubDbProvider db;
        db.connection().state().update_rows = 0;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK_FALSE(repo.inisert_admin_account(make_record()));
    }

    TEST_CASE("inisert_admin_account binds admin id hash and salt")
    {
        // Spy: 断言 SQL 文本、绑定参数与调用次数
        test_support::SpyDbProvider db;
        db.connection().state().update_rows = 1;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(repo.inisert_admin_account(make_record()));

        CHECK(db.connection().state().prepared_sql ==
              "INSERT INTO admin_it (admin_id,password_hash,salt) VALUES(?,?,?)");

        const auto& params = db.connection().state().string_params;
        REQUIRE(params.size() == 3);
        CHECK(params[0].first == 1);
        CHECK(params[0].second == "admin");
        CHECK(params[1].first == 2);
        CHECK(params[1].second == "hash-value");
        CHECK(params[2].first == 3);
        CHECK(params[2].second == "salt-value");

        CHECK(db.connection().state().int_params.empty());
        CHECK(db.connection().state().query_calls == 0);
        CHECK(db.connection().state().update_calls == 1);
        CHECK(db.connection().state().events ==
              std::vector<std::string>{"prepare", "bind_string", "bind_string", "bind_string",
                                       "update"});
    }

    TEST_CASE("inisert_admin_account throws DatabaseError when db unavailable")
    {
        test_support::UnavailableDbProvider db;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.inisert_admin_account(make_record()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("inisert_admin_account throws DatabaseError on sql failure")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_update = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.inisert_admin_account(make_record()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("inisert_admin_account throws DatabaseError when prepare fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_prepare = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.inisert_admin_account(make_record()); },
                               common::ErrorCode::DatabaseError));
    }

    TEST_CASE("inisert_admin_account throws DatabaseError when string binding fails")
    {
        test_support::StubDbProvider db;
        db.connection().state().throw_on_set_string = true;

        AdminAccountRepository repo(db, kAdminTable);

        CHECK(throws_with_code([&repo] { repo.inisert_admin_account(make_record()); },
                               common::ErrorCode::DatabaseError));
    }
}
