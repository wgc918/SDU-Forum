//-----------------------------------------------------------------------------
// 文件: test_it_admin_user_flow.cpp
// 描述: L2 服务层 —— 管理端封禁/解封业务编排（真实 MySQL 黑名单表）。
//-----------------------------------------------------------------------------
#include <optional>
#include <string>

#include "common/ban_enums.h"
#include "common/errors.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"
#include "repositories/blacklist_repository.h"
#include "services/admin/user_service.h"

namespace
{

template <typename Callable>
common::ErrorCode error_code_of(Callable&& callable)
{
    try
    {
        callable();
    }
    catch (const common::AppException& error)
    {
        return error.code();
    }
    return common::ErrorCode::InternalError;
}

struct Fixture
{
    repository::BlacklistRepository repo;
    admin::service::AdminUserService service;

    Fixture()
        : repo(it::db(), infra::AppConfig::instance().table().blacklist_table), service(repo)
    {
    }
};

admin::service::BanMeta permanent_meta(ban::BanType type, ban::ReasonCode code)
{
    admin::service::BanMeta meta;
    meta.type        = type;
    meta.reason_code = code;
    meta.time_type   = 0xFFFF;
    meta.source      = ban::BanSource::Manual;
    return meta;
}

admin::service::BanMeta timed_meta(ban::BanType type, ban::ReasonCode code, uint16_t time_type,
                                   uint32_t time)
{
    admin::service::BanMeta meta;
    meta.type        = type;
    meta.reason_code = code;
    meta.time_type   = time_type;
    meta.time        = time;
    meta.source      = ban::BanSource::Manual;
    return meta;
}

}  // namespace

TEST_CASE("admin_user_flow: 永久封禁并查询详情")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");

    fx.service.ban(account_id, "admin", permanent_meta(ban::BanType::All, ban::ReasonCode::SpamAd));

    auto detail = fx.service.get_deatil(account_id, ban::BanType::All);
    REQUIRE(detail.has_value());
    CHECK(detail->expire_at == repository::PERMANENT_EXPIRE);
    CHECK(detail->reason == ban::ReasonDisplayText(ban::ReasonCode::SpamAd));
    CHECK(static_cast<int>(detail->status) == static_cast<int>(ban::BanStatus::Active));
    CHECK(fx.repo.is_blacklisted(account_id));
}

TEST_CASE("admin_user_flow: 重复封禁返回冲突")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    fx.service.ban(account_id, "admin", permanent_meta(ban::BanType::All, ban::ReasonCode::SpamAd));

    CHECK(static_cast<int>(error_code_of([&] {
              fx.service.ban(account_id, "admin",
                             permanent_meta(ban::BanType::All, ban::ReasonCode::Porn));
          })) == static_cast<int>(common::ErrorCode::Conflict));
}

TEST_CASE("admin_user_flow: 限时封禁的过期时间在未来")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    fx.service.ban(account_id, "admin",
                   timed_meta(ban::BanType::Login, ban::ReasonCode::SpamAd, 1, 30));

    auto detail = fx.service.get_deatil(account_id, ban::BanType::Login);
    REQUIRE(detail.has_value());
    CHECK(detail->expire_at > it::local_timestamp(0));
}

TEST_CASE("admin_user_flow: 全站封禁下新增其他维度被拒绝")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    fx.service.ban(account_id, "admin", permanent_meta(ban::BanType::All, ban::ReasonCode::SpamAd));

    CHECK(static_cast<int>(error_code_of([&] {
              fx.service.ban(account_id, "admin",
                             permanent_meta(ban::BanType::Comment, ban::ReasonCode::Porn));
          })) == static_cast<int>(common::ErrorCode::Conflict));
}

TEST_CASE("admin_user_flow: 全站封禁下解除单一维度无效")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    fx.service.ban(account_id, "admin", permanent_meta(ban::BanType::All, ban::ReasonCode::SpamAd));

    CHECK(static_cast<int>(error_code_of([&] {
              fx.service.unban(account_id, "admin", ban::BanType::Comment, "oops");
          })) == static_cast<int>(common::ErrorCode::NotFound));
}

TEST_CASE("admin_user_flow: 解除全部封禁后不再生效")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    fx.service.ban(account_id, "admin", permanent_meta(ban::BanType::All, ban::ReasonCode::SpamAd));

    fx.service.unban(account_id, "admin", ban::BanType::All, "appeal accepted");

    CHECK_FALSE(fx.service.get_deatil(account_id, ban::BanType::All).has_value());
    CHECK_FALSE(fx.repo.is_blacklisted(account_id));

    repository::BlacklistFilter filter;
    filter.account_id = account_id;
    const auto page   = fx.repo.search(filter, 1, 20);
    REQUIRE(page.total == 1);
    CHECK(static_cast<int>(page.item.front().status) ==
          static_cast<int>(ban::BanStatus::Revoked));
}

TEST_CASE("admin_user_flow: 解除不存在的封禁返回未找到")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    CHECK(static_cast<int>(error_code_of([&] {
              fx.service.unban(account_id, "admin", ban::BanType::All, "none");
          })) == static_cast<int>(common::ErrorCode::NotFound));
}

TEST_CASE("admin_user_flow: 过期兜底后不再生效")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    it::seed_ban(account_id, ban::BanType::All, ban::BanStatus::Active, ban::ReasonCode::SpamAd,
                 "expired", it::local_timestamp(-3600), it::local_timestamp(-60));

    CHECK(fx.repo.expire_outdated() >= 1);

    CHECK_FALSE(fx.service.get_deatil(account_id, ban::BanType::All).has_value());
}

TEST_CASE("admin_user_flow: 自定义原因写入原文")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    auto meta = permanent_meta(ban::BanType::All, ban::ReasonCode::ManualCustom);
    meta.reason = "custom reason text";
    fx.service.ban(account_id, "admin", meta);

    const auto detail = fx.service.get_deatil(account_id, ban::BanType::All);
    REQUIRE(detail.has_value());
    CHECK(detail->reason == "custom reason text");
}

TEST_CASE("admin_user_flow: 标准原因忽略自定义文案")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    auto meta = permanent_meta(ban::BanType::All, ban::ReasonCode::SpamAd);
    meta.reason = "should be ignored";
    fx.service.ban(account_id, "admin", meta);

    const auto detail = fx.service.get_deatil(account_id, ban::BanType::All);
    REQUIRE(detail.has_value());
    CHECK(detail->reason == ban::ReasonDisplayText(ban::ReasonCode::SpamAd));
}

TEST_CASE("admin_user_flow: 参数校验失败场景")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");

    // 非法时长
    CHECK(static_cast<int>(error_code_of([&] {
              fx.service.ban(account_id, "admin",
                             timed_meta(ban::BanType::All, ban::ReasonCode::SpamAd, 0, 0));
          })) == static_cast<int>(common::ErrorCode::InvalidParam));

    // 自动类原因必须 source=auto/report
    CHECK(static_cast<int>(error_code_of([&] {
              fx.service.ban(account_id, "admin",
                             permanent_meta(ban::BanType::All, ban::ReasonCode::AutoRuleHit));
          })) == static_cast<int>(common::ErrorCode::InvalidParam));
}

TEST_CASE("admin_user_flow: 自动类原因配合自动来源可写入")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const uint64_t account_id = it::seed_account(it::new_user_id(), "secret123");
    auto meta        = permanent_meta(ban::BanType::All, ban::ReasonCode::AutoRuleHit);
    meta.source      = ban::BanSource::Auto;
    fx.service.ban(account_id, "system", meta);

    const auto detail = fx.service.get_deatil(account_id, ban::BanType::All);
    REQUIRE(detail.has_value());
    CHECK(static_cast<int>(detail->source) == static_cast<int>(ban::BanSource::Auto));
}
