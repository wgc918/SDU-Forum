//-----------------------------------------------------------------------------
// 文件: test_it_token_repo.cpp
// 描述: L3 数据层 —— TokenRepository 的 MySQL 持久化与缓存回写行为。
//-----------------------------------------------------------------------------
#include <ctime>
#include <string>

#include "common/errors.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"
#include "repositories/token_repository.h"
#include "support/test_doubles.h"

namespace
{

uint64_t in_one_hour()
{
    return static_cast<uint64_t>(std::time(nullptr)) + 3600;
}

std::string unique_hash(const std::string& prefix)
{
    return prefix + it::new_user_id();
}

}  // namespace

TEST_CASE("token_repo: refresh token 持久化与查询")
{
    it::load_config();
    it::reset_db();

    test_support::FakeCacheProvider cache;
    repository::TokenRepository     repo(it::db(), cache, it::token_store_config());

    const std::string hash = unique_hash("itr_");
    CHECK(repo.insert_refresh_token(repository::TokenRecord{hash, in_one_hour()}));

    CHECK(repo.find_refresh_token(hash));
    CHECK_FALSE(repo.find_refresh_token("itr_not_exists"));
    CHECK(repo.hash_secret() == infra::AppConfig::instance().jwt().hash_secret);
}

TEST_CASE("token_repo: 数据库命中后回写缓存并可按缓存命中")
{
    it::load_config();
    it::reset_db();

    test_support::FakeCacheProvider cache;
    repository::TokenRepository     repo(it::db(), cache, it::token_store_config());

    const std::string hash = unique_hash("itr_");
    CHECK(repo.insert_refresh_token(repository::TokenRecord{hash, in_one_hour()}));

    CHECK(repo.find_refresh_token(hash));
    CHECK(cache.fake().contains(hash));
    CHECK(cache.fake().ttl_of(hash) == it::token_store_config().refresh_ttl_s);

    // 删除数据库记录后仍能通过缓存命中，证明回写生效
    it::exec("DELETE FROM `it_refresh_tokens` WHERE token_hash = '" + hash + "'");
    CHECK(repo.find_refresh_token(hash));
}

TEST_CASE("token_repo: access token 仅写缓存")
{
    it::load_config();
    it::reset_db();

    test_support::FakeCacheProvider cache;
    repository::TokenRepository     repo(it::db(), cache, it::token_store_config());

    const std::string hash = unique_hash("ita_");
    CHECK(repo.insert_access_token(repository::TokenRecord{hash, 0}));
    CHECK(cache.fake().contains(hash));
    CHECK(cache.fake().ttl_of(hash) == it::token_store_config().access_ttl_s);
    CHECK(repo.find_access_token(hash));
    CHECK_FALSE(repo.find_access_token("ita_not_exists"));
}

TEST_CASE("token_repo: 缓存不可用时 refresh 降级数据库、access 直接失败")
{
    it::load_config();
    it::reset_db();

    test_support::UnavailableCacheProvider cache;
    repository::TokenRepository            repo(it::db(), cache, it::token_store_config());

    const std::string refresh_hash = unique_hash("itr_");
    CHECK(repo.insert_refresh_token(repository::TokenRecord{refresh_hash, in_one_hour()}));
    CHECK(repo.find_refresh_token(refresh_hash));  // 走数据库降级

    CHECK_FALSE(repo.insert_access_token(repository::TokenRecord{unique_hash("ita_"), 0}));
    CHECK_FALSE(repo.find_access_token(unique_hash("ita_")));
}

TEST_CASE("token_repo: 重复 refresh token 触发数据库异常")
{
    it::load_config();
    it::reset_db();

    test_support::FakeCacheProvider cache;
    repository::TokenRepository     repo(it::db(), cache, it::token_store_config());

    const std::string hash = unique_hash("itr_");
    CHECK(repo.insert_refresh_token(repository::TokenRecord{hash, in_one_hour()}));

    try
    {
        repo.insert_refresh_token(repository::TokenRecord{hash, in_one_hour()});
        FAIL("expected AppException for duplicated token_hash");
    }
    catch (const common::AppException& error)
    {
        CHECK(static_cast<int>(error.code()) ==
              static_cast<int>(common::ErrorCode::DatabaseError));
    }
}
