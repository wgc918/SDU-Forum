//-----------------------------------------------------------------------------
// 文件: test_it_account_repo.cpp
// 描述: L3 数据层 —— AccountRepository 对真实 MySQL 的读写行为。
//-----------------------------------------------------------------------------
#include <optional>
#include <string>

#include "common/errors.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"
#include "repositories/account_repository.h"
#include "security/password_hasher.h"

namespace
{

repository::AccountRecord make_record(const std::string& public_id, const std::string& password,
                                      int iterations = 1000)
{
    const auto hash = infra::hash_password(password, iterations);
    repository::AccountRecord record{};
    record.public_id     = public_id;
    record.password_hash = hash.hash;
    record.salt          = hash.salt;
    record.iterations    = hash.iterations;
    return record;
}

}  // namespace

TEST_CASE("account_repo: 插入后可按 user_id 与 public_id 查询")
{
    it::load_config();
    it::reset_db();

    repository::AccountRepository repo(it::db(), infra::AppConfig::instance().table().account_table);

    const std::string user_id   = it::new_user_id();
    const std::string public_id = it::new_public_id();
    const auto        record    = make_record(public_id, "secret123");

    CHECK(repo.insert_account_by_user_id(user_id, record));

    auto by_user = repo.find_account_by_user_id(user_id);
    REQUIRE(by_user.has_value());
    CHECK(by_user->id > 0);
    CHECK(by_user->public_id == public_id);
    CHECK(by_user->password_hash == record.password_hash);
    CHECK(by_user->salt == record.salt);
    CHECK(by_user->iterations == record.iterations);
    CHECK(infra::verify_password(
        "secret123",
        infra::PasswordHash{by_user->password_hash, by_user->salt, by_user->iterations}));

    auto by_public = repo.find_account_by_public_id(public_id);
    REQUIRE(by_public.has_value());
    CHECK(by_public->user_id == user_id);
    CHECK(by_public->id == by_user->id);
}

TEST_CASE("account_repo: 不存在的账号返回空")
{
    it::load_config();
    it::reset_db();

    repository::AccountRepository repo(it::db(), infra::AppConfig::instance().table().account_table);

    CHECK_FALSE(repo.find_account_by_user_id("__it_missing__").has_value());
    CHECK_FALSE(repo.find_account_by_public_id("__it_missing_public__").has_value());
}

TEST_CASE("account_repo: 删除账号")
{
    it::load_config();
    it::reset_db();

    repository::AccountRepository repo(it::db(), infra::AppConfig::instance().table().account_table);

    const std::string user_id   = it::new_user_id();
    const std::string public_id = it::new_public_id();
    CHECK(repo.insert_account_by_user_id(user_id, make_record(public_id, "secret123")));

    CHECK(repo.delete_account_by_public_id(public_id));
    CHECK_FALSE(repo.find_account_by_user_id(user_id).has_value());
    CHECK_FALSE(repo.delete_account_by_public_id(public_id));
}

TEST_CASE("account_repo: 重复 user_id 触发数据库异常")
{
    it::load_config();
    it::reset_db();

    repository::AccountRepository repo(it::db(), infra::AppConfig::instance().table().account_table);

    const std::string user_id = it::new_user_id();
    CHECK(repo.insert_account_by_user_id(user_id, make_record(it::new_public_id(), "secret123")));

    try
    {
        repo.insert_account_by_user_id(user_id, make_record(it::new_public_id(), "secret123"));
        FAIL("expected AppException for duplicated user_id");
    }
    catch (const common::AppException& error)
    {
        CHECK(error.code() == common::ErrorCode::DatabaseError);
    }
}

TEST_CASE("account_repo: 重复 public_id 触发数据库异常")
{
    it::load_config();
    it::reset_db();

    repository::AccountRepository repo(it::db(), infra::AppConfig::instance().table().account_table);

    const std::string public_id = it::new_public_id();
    CHECK(repo.insert_account_by_user_id(it::new_user_id(), make_record(public_id, "secret123")));

    try
    {
        repo.insert_account_by_user_id(it::new_user_id(), make_record(public_id, "secret123"));
        FAIL("expected AppException for duplicated public_id");
    }
    catch (const common::AppException& error)
    {
        CHECK(error.code() == common::ErrorCode::DatabaseError);
    }
}
