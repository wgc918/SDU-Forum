//-----------------------------------------------------------------------------
// 文件: test_it_api_admin_users.cpp
// 描述: L1 接口层 —— 管理端封禁/解封 HTTP 契约（/history 与 /blacklist 尚未实现，不测）。
//-----------------------------------------------------------------------------
#include <json/json.h>

#include <string>

#include "common/ban_enums.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_test_common.h"
#include "repositories/blacklist_repository.h"

namespace
{

constexpr char kAdminLogin[] = "/admin/api/v1/auth/login";

constexpr char kAdminPassword[] = "secret12";
constexpr uint16_t kPermanent   = 0xFFFF;

void prepare()
{
    it::load_config();
    it::server();
    it::reset_db();
}

/// 播种普通账号，返回自增主键
uint64_t seed_user()
{
    return it::seed_account(it::new_user_id(), "secret123");
}

struct AdminSession
{
    std::string admin_id;
    std::string access_token;
};

/// 播种管理员并登录
AdminSession admin_login()
{
    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kAdminPassword);

    Json::Value body;
    body["admin_id"] = admin_id;
    body["password"] = kAdminPassword;

    auto response = it::post_json(kAdminLogin, it::to_json(body));
    REQUIRE(response.status == 200);

    AdminSession session;
    session.admin_id     = admin_id;
    session.access_token = it::data_string(response, "access_token");
    return session;
}

std::string ban_path(uint64_t account_id)
{
    return "/admin/api/v1/users/" + std::to_string(account_id) + "/ban";
}

std::string unban_path(uint64_t account_id)
{
    return "/admin/api/v1/users/" + std::to_string(account_id) + "/unban";
}

Json::Value ban_request(const std::string& operator_id, int reason_code, int ban_type,
                        int time_type, int time = 0, const std::string& source = "manual")
{
    Json::Value body;
    body["reason_code"] = reason_code;
    body["ban_type"]    = ban_type;
    body["operator_id"] = operator_id;
    body["source"]      = source;
    body["time_type"]   = time_type;
    body["time"]        = time;
    return body;
}

/// 直接查询黑名单表，避免依赖尚未实现的 /history 与 /blacklist 接口
std::string blacklist_table()
{
    return "`" + infra::AppConfig::instance().table().blacklist_table + "`";
}

uint64_t count_bans(uint64_t account_id)
{
    return it::query_u64("SELECT COUNT(*) AS v FROM " + blacklist_table() +
                         " WHERE account_id = " + std::to_string(account_id));
}

uint64_t ban_field_u64(uint64_t account_id, const std::string& field)
{
    return it::query_u64("SELECT " + field + " AS v FROM " + blacklist_table() +
                         " WHERE account_id = " + std::to_string(account_id) +
                         " ORDER BY id DESC LIMIT 1");
}

std::string ban_field_string(uint64_t account_id, const std::string& field)
{
    return it::query_string("SELECT " + field + " AS v FROM " + blacklist_table() +
                            " WHERE account_id = " + std::to_string(account_id) +
                            " ORDER BY id DESC LIMIT 1");
}

}  // namespace

//-----------------------------------------------------------------------------
// 封禁
//-----------------------------------------------------------------------------

TEST_CASE("api_admin_users: 全站永久封禁写入生效记录")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();

    auto response = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 1001, 0, kPermanent)),
        it::auth_json_headers(admin.access_token));
    CHECK(response.status == 204);

    CHECK(count_bans(account_id) == 1);
    CHECK(ban_field_u64(account_id, "status") == 1);
    CHECK(ban_field_u64(account_id, "ban_type") == 0);
    CHECK(ban_field_u64(account_id, "reason_code") == 1001);
    CHECK(ban_field_string(account_id, "expire_at") == repository::PERMANENT_EXPIRE);
    CHECK(ban_field_string(account_id, "banned_by") == admin.admin_id);
}

TEST_CASE("api_admin_users: 按登录维度与时长封禁")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();

    auto response = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 3003, 4, 0, 30)),
        it::auth_json_headers(admin.access_token));
    CHECK(response.status == 204);

    CHECK(count_bans(account_id) == 1);
    CHECK(ban_field_u64(account_id, "ban_type") == 4);
    const std::string expire_at = ban_field_string(account_id, "expire_at");
    CHECK(expire_at != repository::PERMANENT_EXPIRE);
    CHECK(expire_at > it::local_timestamp(0));
}

TEST_CASE("api_admin_users: 同维度重复封禁返回 409 CONFLICT")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();
    const auto headers    = it::auth_json_headers(admin.access_token);

    auto first = it::post_json(ban_path(account_id),
                               it::to_json(ban_request(admin.admin_id, 1001, 0, kPermanent)),
                               headers);
    REQUIRE(first.status == 204);

    auto second = it::post_json(ban_path(account_id),
                                it::to_json(ban_request(admin.admin_id, 1002, 0, kPermanent)),
                                headers);
    CHECK(second.status == 409);
    CHECK(it::error_code(second) == "CONFLICT");
}

TEST_CASE("api_admin_users: 系统自动原因必须配合 auto/report 来源")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();
    const auto headers    = it::auth_json_headers(admin.access_token);

    auto manual = it::post_json(ban_path(account_id),
                                it::to_json(ban_request(admin.admin_id, 4001, 0, kPermanent)),
                                headers);
    CHECK(manual.status == 400);
    CHECK(it::error_code(manual) == "INVALID_PARAM");

    auto automatic = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 4001, 0, kPermanent, 0, "auto")), headers);
    CHECK(automatic.status == 204);
}

TEST_CASE("api_admin_users: 封禁参数非法返回 400")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();
    const auto headers    = it::auth_json_headers(admin.access_token);

    Json::Value missing_reason = ban_request(admin.admin_id, 1001, 0, kPermanent);
    missing_reason.removeMember("reason_code");
    auto missing_reason_resp = it::post_json(ban_path(account_id), it::to_json(missing_reason),
                                             headers);
    CHECK(missing_reason_resp.status == 400);
    CHECK(it::error_code(missing_reason_resp) == "INVALID_PARAM");

    Json::Value missing_operator = ban_request(admin.admin_id, 1001, 0, kPermanent);
    missing_operator.removeMember("operator_id");
    CHECK(it::post_json(ban_path(account_id), it::to_json(missing_operator), headers).status ==
          400);

    auto bad_operator = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request("short", 1001, 0, kPermanent)), headers);
    CHECK(bad_operator.status == 400);
    CHECK(it::error_code(bad_operator) == "INVALID_PARAM");

    auto bad_reason = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 1999, 0, kPermanent)), headers);
    CHECK(bad_reason.status == 400);
    CHECK(it::error_code(bad_reason) == "INVALID_PARAM");

    auto bad_type = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 1001, 99, kPermanent)), headers);
    CHECK(bad_type.status == 400);

    auto bad_duration = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 1001, 0, 3, 10)), headers);
    CHECK(bad_duration.status == 400);

    auto zero_duration = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 1001, 0, 0, 0)), headers);
    CHECK(zero_duration.status == 400);

    auto bad_source = it::post_json(
        ban_path(account_id),
        it::to_json(ban_request(admin.admin_id, 1001, 0, kPermanent, 0, "bad")), headers);
    CHECK(bad_source.status == 400);
}

TEST_CASE("api_admin_users: 自定义原因封禁后保留原因原文")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();

    Json::Value body = ban_request(admin.admin_id, 5999, 0, kPermanent);
    body["reason"]   = "站外违规，站长决定封禁";

    auto response = it::post_json(ban_path(account_id), it::to_json(body),
                                  it::auth_json_headers(admin.access_token));
    CHECK(response.status == 204);
    CHECK(ban_field_string(account_id, "reason") == "站外违规，站长决定封禁");
}

//-----------------------------------------------------------------------------
// 解封
//-----------------------------------------------------------------------------

TEST_CASE("api_admin_users: 解封成功后记录标记解除")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();
    it::seed_ban(account_id, ban::BanType::All, ban::BanStatus::Active, ban::ReasonCode::SpamAd,
                 "spam", it::local_timestamp(-60), repository::PERMANENT_EXPIRE);

    Json::Value body;
    body["ban_type"] = 0;
    body["reason"]   = "申诉通过";

    auto response = it::http().request("PATCH", unban_path(account_id), it::to_json(body),
                                       it::auth_json_headers(admin.access_token));
    CHECK(response.status == 204);

    CHECK(ban_field_u64(account_id, "status") == 0);
    CHECK(ban_field_string(account_id, "unban_reason") == "申诉通过");
    CHECK(ban_field_string(account_id, "unbanned_by") == admin.admin_id);
}

TEST_CASE("api_admin_users: 解封未封禁账号返回 404 NOT_FOUND")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();

    Json::Value body;
    body["ban_type"] = 0;
    body["reason"]   = "none";

    auto response = it::http().request("PATCH", unban_path(account_id), it::to_json(body),
                                       it::auth_json_headers(admin.access_token));
    CHECK(response.status == 404);
    CHECK(it::error_code(response) == "NOT_FOUND");
}

TEST_CASE("api_admin_users: 解封缺少参数返回 400 INVALID_PARAM")
{
    prepare();
    const auto admin      = admin_login();
    const auto account_id = seed_user();
    const auto headers    = it::auth_json_headers(admin.access_token);

    Json::Value no_type;
    no_type["reason"] = "none";
    CHECK(it::http().request("PATCH", unban_path(account_id), it::to_json(no_type), headers).status ==
          400);

    Json::Value no_reason;
    no_reason["ban_type"] = 0;
    auto response = it::http().request("PATCH", unban_path(account_id), it::to_json(no_reason),
                                       headers);
    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}
