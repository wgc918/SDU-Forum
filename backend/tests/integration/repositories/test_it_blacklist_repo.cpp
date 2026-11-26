//-----------------------------------------------------------------------------
// 文件: test_it_blacklist_repo.cpp
// 描述: L3 数据层 —— BlacklistRepository 现有接口的生效判定、解封、过期与检索。
//-----------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include "common/ban_enums.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"
#include "repositories/blacklist_repository.h"

namespace
{

repository::BlacklistEntry make_entry(uint64_t account_id, ban::BanType type,
                                      ban::ReasonCode reason_code, const std::string& reason,
                                      ban::BanStatus status, const std::string& expire_at)
{
    repository::BlacklistEntry entry;
    entry.account_id  = account_id;
    entry.reason      = reason;
    entry.reason_code = reason_code;
    entry.ban_type    = type;
    entry.banned_by   = "admin";
    entry.expire_at   = expire_at;
    entry.status      = status;
    entry.source      = ban::BanSource::Manual;
    return entry;
}

repository::BlacklistRepository& repo()
{
    static repository::BlacklistRepository instance(
        it::db(), infra::AppConfig::instance().table().blacklist_table);
    return instance;
}

repository::PageResult search_account(uint64_t account_id, int page = 1, int page_size = 20)
{
    repository::BlacklistFilter filter;
    filter.account_id = account_id;
    return repo().search(filter, page, page_size);
}

}  // namespace

TEST_CASE("blacklist_repo: 插入生效记录并查询")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    const auto     entry = make_entry(account_id, ban::BanType::All, ban::ReasonCode::SpamAd,
                                     "spam", ban::BanStatus::Active, it::local_timestamp(3600));

    CHECK(repo().insert(entry));

    auto detail = repo().get_detail(account_id, ban::BanType::All);
    REQUIRE(detail.has_value());
    CHECK(detail->reason == "spam");
    CHECK(static_cast<int>(detail->reason_code) ==
          static_cast<int>(ban::ReasonCode::SpamAd));
    CHECK(static_cast<int>(detail->status) == static_cast<int>(ban::BanStatus::Active));
    CHECK(detail->expire_at == entry.expire_at);

    CHECK(repo().is_blacklisted(account_id));
    CHECK(repo().exists_active(account_id, ban::BanType::All));
    CHECK_FALSE(repo().exists_active(account_id, ban::BanType::Comment));
    CHECK_FALSE(repo().is_blacklisted(999999999));
}

TEST_CASE("blacklist_repo: 已过期但 status=1 的记录不生效")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    CHECK(repo().insert(make_entry(account_id, ban::BanType::All, ban::ReasonCode::SpamAd,
                                   "expired", ban::BanStatus::Active, it::local_timestamp(-60))));

    CHECK_FALSE(repo().is_blacklisted(account_id));
    CHECK_FALSE(repo().get_detail(account_id, ban::BanType::All).has_value());

    // 未生效的记录仍然可按过期状态检索到
    repository::BlacklistFilter expired;
    expired.status = ban::BanStatus::Expired;
    const auto expired_page = repo().search(expired, 1, 20);
    REQUIRE(expired_page.total == 1);
    CHECK(static_cast<int>(expired_page.item.front().status) ==
          static_cast<int>(ban::BanStatus::Active));

    repository::BlacklistFilter active;
    active.status = ban::BanStatus::Active;
    CHECK(repo().search(active, 1, 20).total == 0);
}

TEST_CASE("blacklist_repo: expire_outdated 批量置为已过期")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    CHECK(repo().insert(make_entry(account_id, ban::BanType::All, ban::ReasonCode::SpamAd,
                                   "expired", ban::BanStatus::Active, it::local_timestamp(-60))));

    CHECK(repo().expire_outdated() >= 1);

    CHECK_FALSE(repo().get_detail(account_id, ban::BanType::All).has_value());

    const auto page = search_account(account_id);
    REQUIRE(page.total == 1);
    CHECK(static_cast<int>(page.item.front().status) ==
          static_cast<int>(ban::BanStatus::Expired));
    CHECK(page.item.front().unbanned_by == "system");
}

TEST_CASE("blacklist_repo: 按维度解封")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    CHECK(repo().insert(make_entry(account_id, ban::BanType::Comment, ban::ReasonCode::SpamAd,
                                   "comment ban", ban::BanStatus::Active,
                                   it::local_timestamp(3600))));

    CHECK(repo().unban(account_id, ban::BanType::Comment, "admin", "appeal") == 1);
    CHECK_FALSE(repo().exists_active(account_id, ban::BanType::Comment));
    CHECK(repo().unban(account_id, ban::BanType::Comment, "admin", "again") == 0);

    const auto page = search_account(account_id);
    REQUIRE(page.total == 1);
    CHECK(static_cast<int>(page.item.front().status) ==
          static_cast<int>(ban::BanStatus::Revoked));
    CHECK(page.item.front().unban_reason == "appeal");
}

TEST_CASE("blacklist_repo: 解封 All 清除全部生效记录")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    CHECK(repo().insert(make_entry(account_id, ban::BanType::All, ban::ReasonCode::SpamAd,
                                   "all", ban::BanStatus::Active, it::local_timestamp(3600))));
    CHECK(repo().insert(make_entry(account_id, ban::BanType::Comment, ban::ReasonCode::Porn,
                                   "comment", ban::BanStatus::Active, it::local_timestamp(3600))));

    CHECK(repo().unban(account_id, ban::BanType::All, "admin", "release") == 2);
    CHECK_FALSE(repo().is_blacklisted(account_id));
    for (const auto& item : search_account(account_id).item)
        CHECK(static_cast<int>(item.status) == static_cast<int>(ban::BanStatus::Revoked));
}

TEST_CASE("blacklist_repo: 检索按 banned_at 倒序")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    it::seed_ban(account_id, ban::BanType::All, ban::BanStatus::Revoked, ban::ReasonCode::SpamAd,
                 "older", it::local_timestamp(-120), it::local_timestamp(3600));
    it::seed_ban(account_id, ban::BanType::Comment, ban::BanStatus::Active, ban::ReasonCode::Porn,
                 "newer", it::local_timestamp(-60), it::local_timestamp(3600));

    const auto page = search_account(account_id);
    REQUIRE(page.total == 2);
    CHECK(page.item.front().reason == "newer");
    CHECK(page.item.back().reason == "older");
}

TEST_CASE("blacklist_repo: 条件检索与分页")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    CHECK(repo().insert(make_entry(account_id, ban::BanType::All, ban::ReasonCode::SpamAd,
                                   "spam", ban::BanStatus::Active, it::local_timestamp(3600))));
    CHECK(repo().insert(make_entry(account_id, ban::BanType::Comment, ban::ReasonCode::Porn,
                                   "porn", ban::BanStatus::Active, it::local_timestamp(3600))));
    CHECK(repo().insert(make_entry(account_id, ban::BanType::Post, ban::ReasonCode::SpamAd,
                                   "old spam", ban::BanStatus::Revoked,
                                   it::local_timestamp(3600))));

    repository::BlacklistFilter by_reason;
    by_reason.reason = ban::ReasonCode::SpamAd;
    CHECK(repo().search(by_reason, 1, 20).item.size() == 2);

    repository::BlacklistFilter by_reason_active;
    by_reason_active.reason = ban::ReasonCode::SpamAd;
    by_reason_active.status = ban::BanStatus::Active;
    CHECK(repo().search(by_reason_active, 1, 20).item.size() == 1);

    repository::BlacklistFilter by_type;
    by_type.type   = ban::BanType::Comment;
    by_type.status = ban::BanStatus::Active;
    CHECK(repo().search(by_type, 1, 20).item.size() == 1);

    repository::BlacklistFilter revoked;
    revoked.status = ban::BanStatus::Revoked;
    CHECK(repo().search(revoked, 1, 20).item.size() == 1);

    repository::BlacklistFilter by_account;
    by_account.account_id = account_id;
    CHECK(repo().search(by_account, 1, 20).item.size() == 3);

    repository::BlacklistFilter active;
    active.status = ban::BanStatus::Active;
    CHECK(repo().search(active, 1, 1).item.size() == 1);

    CHECK(repo().search(active, 0, 10).item.empty());
    CHECK(repo().search(active, 1, 0).item.empty());
}

TEST_CASE("blacklist_repo: 已解除记录不再生效")
{
    it::load_config();
    it::reset_db();

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    it::seed_ban(account_id, ban::BanType::All, ban::BanStatus::Revoked, ban::ReasonCode::SpamAd,
                 "revoked", it::local_timestamp(-60), it::local_timestamp(3600));

    CHECK_FALSE(repo().is_blacklisted(account_id));
    CHECK_FALSE(repo().get_detail(account_id, ban::BanType::All).has_value());
}
