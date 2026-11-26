//-----------------------------------------------------------------------------
// 文件: test_it_api_user_auth.cpp
// 描述: L1 接口层 —— 用户认证 HTTP 契约（真实服务端 + 真实 MySQL；Redis 用例可跳过）。
//-----------------------------------------------------------------------------
#include <json/json.h>

#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_test_common.h"
#include "repositories/blacklist_repository.h"

namespace
{

constexpr char kLogin[]   = "/sdu_forum/api/v1/auth/login";
constexpr char kSignUp[]  = "/sdu_forum/api/v1/auth/sign-up";
constexpr char kRefresh[] = "/sdu_forum/api/v1/auth/refresh-token";
constexpr char kLogout[]  = "/sdu_forum/api/v1/auth/logout";
constexpr char kLogoff[]  = "/sdu_forum/api/v1/auth/logoff";

constexpr char kPassword[] = "secret12";

struct UserTokens
{
    std::string public_id;
    std::string access;
    std::string refresh;
};

/// 每个用例：加载配置 → 启动服务端（仅首次）→ 清空 it_ 表
void prepare()
{
    it::load_config();
    it::server();
    it::reset_db();
}

Json::Value credentials(const std::string& user_id, const std::string& password)
{
    Json::Value body;
    body["user_id"]  = user_id;
    body["password"] = password;
    return body;
}

UserTokens login(const std::string& user_id, const std::string& password = kPassword)
{
    auto response = it::post_json(kLogin, it::to_json(credentials(user_id, password)));
    REQUIRE(response.status == 200);
    REQUIRE(response.has_json());

    UserTokens tokens;
    tokens.public_id = it::data_string(response, "public_id");
    tokens.access    = it::data_string(response, "access_token");
    tokens.refresh   = it::data_string(response, "refresh_token");
    return tokens;
}

std::string account_table()
{
    return infra::AppConfig::instance().table().account_table;
}

}  // namespace

//-----------------------------------------------------------------------------
// 注册
//-----------------------------------------------------------------------------

TEST_CASE("api_user_auth: 注册成功返回 201 与 24 位 public_id")
{
    prepare();

    const std::string user_id = it::new_user_id();
    auto response = it::post_json(kSignUp, it::to_json(credentials(user_id, kPassword)));

    CHECK(response.status == 201);
    REQUIRE(response.has_json());
    CHECK(it::data_string(response, "public_id").size() == 24);
    CHECK(it::account_id_of(user_id) > 0);

    const auto iterations = it::query_u64("SELECT iterations AS v FROM `" + account_table() +
                                          "` WHERE user_id = '" + user_id + "'");
    CHECK(iterations >= 150000);  // 注册走生产默认迭代次数
}

TEST_CASE("api_user_auth: 重复 user_id 注册返回 400 INVALID_PARAM")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);

    auto response = it::post_json(kSignUp, it::to_json(credentials(user_id, kPassword)));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

TEST_CASE("api_user_auth: 注册缺少字段返回 400 INVALID_PARAM")
{
    prepare();

    Json::Value body;
    body["user_id"] = it::new_user_id();

    auto response = it::post_json(kSignUp, it::to_json(body));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
    CHECK(it::error_message(response).find("password") != std::string::npos);
}

TEST_CASE("api_user_auth: 注册字段类型错误返回 400 INVALID_PARAM")
{
    prepare();

    Json::Value body;
    body["user_id"]  = it::new_user_id();
    body["password"] = 123456;

    auto response = it::post_json(kSignUp, it::to_json(body));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

TEST_CASE("api_user_auth: 注册 user_id 超长返回 400 INVALID_PARAM")
{
    prepare();

    auto response =
        it::post_json(kSignUp, it::to_json(credentials(std::string(65, 'a'), kPassword)));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

TEST_CASE("api_user_auth: 注册空 user_id 返回 400 INVALID_PARAM")
{
    prepare();

    auto response = it::post_json(kSignUp, it::to_json(credentials("", kPassword)));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

TEST_CASE("api_user_auth: 注册请求体非法 JSON 返回 400 INVALID_JSON")
{
    prepare();

    auto response = it::post_json(kSignUp, "{");

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_JSON");
}

//-----------------------------------------------------------------------------
// 登录
//-----------------------------------------------------------------------------

TEST_CASE("api_user_auth: 登录成功返回双 token 与过期时间")
{
    prepare();

    const std::string user_id = it::new_user_id();
    std::string       public_id;
    it::seed_account(user_id, kPassword, &public_id);

    auto response = it::post_json(kLogin, it::to_json(credentials(user_id, kPassword)));

    CHECK(response.status == 200);
    REQUIRE(response.has_json());
    CHECK(it::data_string(response, "public_id") == public_id);
    CHECK(it::data_string(response, "token_type") == "Bearer");
    CHECK(it::data_string(response, "access_token").size() > 20);
    CHECK(it::data_string(response, "refresh_token").size() > 20);
    CHECK(it::data_int64(response, "access_token_exp") > 0);
    CHECK(it::data_int64(response, "refresh_token_exp") >
          it::data_int64(response, "access_token_exp"));
}

TEST_CASE("api_user_auth: 密码错误返回 401 INVALID_CREDENTIALS")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);

    auto response = it::post_json(kLogin, it::to_json(credentials(user_id, "wrongpas")));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "INVALID_CREDENTIALS");
}

TEST_CASE("api_user_auth: 账号不存在返回 401 INVALID_CREDENTIALS")
{
    prepare();

    auto response = it::post_json(kLogin, it::to_json(credentials(it::new_user_id(), kPassword)));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "INVALID_CREDENTIALS");
}

TEST_CASE("api_user_auth: 全站封禁账号登录返回 403 ACCOUNT_BANNED")
{
    prepare();

    const std::string user_id = it::new_user_id();
    const auto        id      = it::seed_account(user_id, kPassword);
    it::seed_ban(id, ban::BanType::All, ban::BanStatus::Active, ban::ReasonCode::SpamAd, "spam",
                 it::local_timestamp(-60), repository::PERMANENT_EXPIRE);

    auto response = it::post_json(kLogin, it::to_json(credentials(user_id, kPassword)));

    CHECK(response.status == 403);
    CHECK(it::error_code(response) == "ACCOUNT_BANNED");
}

TEST_CASE("api_user_auth: 禁止登录维度封禁返回 403 ACCOUNT_BANNED")
{
    prepare();

    const std::string user_id = it::new_user_id();
    const auto        id      = it::seed_account(user_id, kPassword);
    it::seed_ban(id, ban::BanType::Login, ban::BanStatus::Active, ban::ReasonCode::BotActivity,
                 "bot", it::local_timestamp(-60), repository::PERMANENT_EXPIRE);

    auto response = it::post_json(kLogin, it::to_json(credentials(user_id, kPassword)));

    CHECK(response.status == 403);
    CHECK(it::error_code(response) == "ACCOUNT_BANNED");
}

TEST_CASE("api_user_auth: 登录缺少字段返回 400 INVALID_PARAM")
{
    prepare();

    Json::Value body;
    body["user_id"] = it::new_user_id();

    auto response = it::post_json(kLogin, it::to_json(body));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

//-----------------------------------------------------------------------------
// 刷新
//-----------------------------------------------------------------------------

TEST_CASE("api_user_auth: 刷新 access token 且不轮换 refresh token")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = tokens.access;

    auto response = it::post_json(kRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));

    CHECK(response.status == 200);
    REQUIRE(response.has_json());
    CHECK(!it::data_string(response, "access_token").empty());
    CHECK(it::data_string(response, "refresh_token").empty());

    const auto&        jwt    = infra::AppConfig::instance().jwt();
    infra::TokenSigner signer(jwt.access_secret, jwt.issuer);
    const auto         payload = signer.verify(it::data_string(response, "access_token"));
    REQUIRE(payload.has_value());
    CHECK(payload->sub == tokens.public_id);
    CHECK(payload->role == infra::kUserRole);
}

TEST_CASE("api_user_auth: 轮换后返回新的 refresh token")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refreshing"]   = true;
    body["access_token"] = tokens.access;

    auto response = it::post_json(kRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));

    CHECK(response.status == 200);
    REQUIRE(response.has_json());
    CHECK(!it::data_string(response, "refresh_token").empty());
    CHECK(it::data_int64(response, "refresh_token_exp") > 0);

    const auto&        jwt = infra::AppConfig::instance().jwt();
    infra::TokenSigner refresh_signer(jwt.refresh_secret, jwt.issuer);
    const auto         payload = refresh_signer.verify(it::data_string(response, "refresh_token"));
    REQUIRE(payload.has_value());
    CHECK(payload->sub == tokens.public_id);
    CHECK(payload->role == infra::kUserRole);
}

TEST_CASE("api_user_auth: 轮换后旧 refresh token 再次刷新返回 401")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refreshing"]   = true;
    body["access_token"] = tokens.access;

    auto first =
        it::post_json(kRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));
    REQUIRE(first.status == 200);

    Json::Value replay;
    replay["refreshing"]   = false;
    replay["access_token"] = tokens.access;

    auto second =
        it::post_json(kRefresh, it::to_json(replay), it::auth_json_headers(tokens.refresh));

    CHECK(second.status == 401);
    CHECK(it::error_code(second) == "UNAUTHORIZED");
    CHECK(it::error_message(second).find("old") != std::string::npos);
}

TEST_CASE("api_user_auth: access token 不能用于刷新接口")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = tokens.access;

    auto response = it::post_json(kRefresh, it::to_json(body), it::auth_json_headers(tokens.access));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}

TEST_CASE("api_user_auth: 刷新缺少 refreshing 返回 400 INVALID_PARAM")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["access_token"] = tokens.access;

    auto response = it::post_json(kRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

TEST_CASE("api_user_auth: 刷新时校验 Login 维度封禁")
{
    prepare();

    const std::string user_id = it::new_user_id();
    const auto        id      = it::seed_account(user_id, kPassword);
    const auto        tokens  = login(user_id);
    it::seed_ban(id, ban::BanType::Login, ban::BanStatus::Active, ban::ReasonCode::BotActivity,
                 "bot", it::local_timestamp(-60), repository::PERMANENT_EXPIRE);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = tokens.access;

    auto response = it::post_json(kRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));

    CHECK(response.status == 403);
    CHECK(it::error_code(response) == "ACCOUNT_BANNED");
}

//-----------------------------------------------------------------------------
// 登出（真实 Redis 吊销）
//-----------------------------------------------------------------------------

TEST_CASE("api_user_auth: 登出成功返回 204 且重复登出被拒绝"
          * doctest::skip(!it::redis_available()))
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refresh_token"] = tokens.refresh;

    auto first = it::post_json(kLogout, it::to_json(body), it::auth_json_headers(tokens.access));
    CHECK(first.status == 204);
    CHECK(first.body.empty());

    auto second = it::post_json(kLogout, it::to_json(body), it::auth_json_headers(tokens.access));
    CHECK(second.status == 401);
    CHECK(it::error_code(second) == "UNAUTHORIZED");
}

TEST_CASE("api_user_auth: 登出缺少 Authorization 返回 401")
{
    prepare();

    Json::Value body;
    body["refresh_token"] = "whatever";

    auto response = it::post_json(kLogout, it::to_json(body));
    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}

TEST_CASE("api_user_auth: 登出 Authorization 方案错误返回 401")
{
    prepare();

    Json::Value body;
    body["refresh_token"] = "whatever";

    auto response = it::post_json(kLogout, it::to_json(body), {{"Authorization", "Token abc"}});
    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
    CHECK(it::error_message(response).find("Bearer") != std::string::npos);
}

//-----------------------------------------------------------------------------
// 注销
//-----------------------------------------------------------------------------

TEST_CASE("api_user_auth: 注销成功删除账号" * doctest::skip(!it::redis_available()))
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refresh_token"] = tokens.refresh;

    auto response =
        it::http().request("POST", kLogoff, it::to_json(body), it::auth_json_headers(tokens.access));

    CHECK(response.status == 204);
    CHECK(it::account_id_of(user_id) == 0);
}

TEST_CASE("api_user_auth: 注销后无法再登录" * doctest::skip(!it::redis_available()))
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refresh_token"] = tokens.refresh;

    auto logoff =
        it::http().request("POST", kLogoff, it::to_json(body), it::auth_json_headers(tokens.access));
    REQUIRE(logoff.status == 204);

    auto response = it::post_json(kLogin, it::to_json(credentials(user_id, kPassword)));
    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "INVALID_CREDENTIALS");
}

//-----------------------------------------------------------------------------
// Bearer 异常
//-----------------------------------------------------------------------------

TEST_CASE("api_user_auth: 过期 access token 返回 401")
{
    prepare();

    const auto& jwt  = infra::AppConfig::instance().jwt();
    infra::TokenSigner signer(jwt.access_secret, jwt.issuer);
    const auto expired = signer.sign(it::new_public_id(), infra::kUserRole, -60);

    Json::Value body;
    body["refresh_token"] = "whatever";

    auto response =
        it::post_json(kLogout, it::to_json(body), it::auth_json_headers(expired.value));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
    CHECK(it::error_message(response).find("invalid or expired") != std::string::npos);
}

TEST_CASE("api_user_auth: 篡改 access token 返回 401")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, kPassword);
    const auto tokens = login(user_id);

    Json::Value body;
    body["refresh_token"] = tokens.refresh;

    auto response =
        it::post_json(kLogout, it::to_json(body), it::auth_json_headers(tokens.access + "x"));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}
