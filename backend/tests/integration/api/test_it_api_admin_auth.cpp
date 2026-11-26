//-----------------------------------------------------------------------------
// 文件: test_it_api_admin_auth.cpp
// 描述: L1 接口层 —— 管理员认证 HTTP 契约与用户/管理员双密钥隔离。
//-----------------------------------------------------------------------------
#include <json/json.h>

#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_test_common.h"

namespace
{

constexpr char kAdminLogin[]   = "/admin/api/v1/auth/login";
constexpr char kAdminRefresh[] = "/admin/api/v1/auth/refresh-token";
constexpr char kAdminLogout[]  = "/admin/api/v1/auth/logout";

constexpr char kUserLogin[]   = "/sdu_forum/api/v1/auth/login";
constexpr char kUserRefresh[] = "/sdu_forum/api/v1/auth/refresh-token";

constexpr char kPassword[] = "secret12";  // admin_id/password 长度上限均为 8

struct AdminTokens
{
    std::string admin_id;
    std::string access;
    std::string refresh;
};

void prepare()
{
    it::load_config();
    it::server();
    it::reset_db();
}

Json::Value admin_credentials(const std::string& admin_id, const std::string& password)
{
    Json::Value body;
    body["admin_id"] = admin_id;
    body["password"] = password;
    return body;
}

AdminTokens login_admin(const std::string& admin_id, const std::string& password = kPassword)
{
    auto response = it::post_json(kAdminLogin, it::to_json(admin_credentials(admin_id, password)));
    REQUIRE(response.status == 200);
    REQUIRE(response.has_json());

    AdminTokens tokens;
    tokens.admin_id = it::data_string(response, "admin_id");
    tokens.access   = it::data_string(response, "access_token");
    tokens.refresh  = it::data_string(response, "refresh_token");
    return tokens;
}

std::string login_user(const std::string& user_id, std::string* refresh = nullptr)
{
    Json::Value body;
    body["user_id"]  = user_id;
    body["password"] = "secret12";

    auto response = it::post_json(kUserLogin, it::to_json(body));
    REQUIRE(response.status == 200);
    if (refresh)
        *refresh = it::data_string(response, "refresh_token");
    return it::data_string(response, "access_token");
}

}  // namespace

//-----------------------------------------------------------------------------
// 登录
//-----------------------------------------------------------------------------

TEST_CASE("api_admin_auth: 管理员登录成功返回双 token")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);

    auto response = it::post_json(kAdminLogin, it::to_json(admin_credentials(admin_id, kPassword)));

    CHECK(response.status == 200);
    REQUIRE(response.has_json());
    CHECK(it::data_string(response, "admin_id") == admin_id);
    CHECK(it::data_string(response, "token_type") == "Bearer");
    CHECK(it::data_string(response, "access_token").size() > 20);
    CHECK(it::data_string(response, "refresh_token").size() > 20);
    CHECK(it::data_int64(response, "access_token_exp") > 0);
}

TEST_CASE("api_admin_auth: 管理员密码错误返回 401 INVALID_CREDENTIALS")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);

    auto response =
        it::post_json(kAdminLogin, it::to_json(admin_credentials(admin_id, "wrong-pw")));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "INVALID_CREDENTIALS");
}

TEST_CASE("api_admin_auth: 管理员不存在返回 401 INVALID_CREDENTIALS")
{
    prepare();

    auto response =
        it::post_json(kAdminLogin, it::to_json(admin_credentials(it::new_admin_id(), kPassword)));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "INVALID_CREDENTIALS");
}

TEST_CASE("api_admin_auth: 管理员登录缺少字段返回 400 INVALID_PARAM")
{
    prepare();

    Json::Value body;
    body["admin_id"] = it::new_admin_id();

    auto response = it::post_json(kAdminLogin, it::to_json(body));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

TEST_CASE("api_admin_auth: 管理员 admin_id 超长返回 400 INVALID_PARAM")
{
    prepare();

    auto response =
        it::post_json(kAdminLogin, it::to_json(admin_credentials(std::string(9, 'a'), kPassword)));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_PARAM");
}

//-----------------------------------------------------------------------------
// 刷新
//-----------------------------------------------------------------------------

TEST_CASE("api_admin_auth: 管理员刷新 access token 且不轮换 refresh")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);
    const auto tokens = login_admin(admin_id);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = tokens.access;

    auto response =
        it::post_json(kAdminRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));

    CHECK(response.status == 200);
    REQUIRE(response.has_json());
    CHECK(!it::data_string(response, "access_token").empty());
    CHECK(it::data_string(response, "refresh_token").empty());

    const auto&        jwt    = infra::AppConfig::instance().jwt();
    infra::TokenSigner signer(jwt.admin_access_secret, jwt.issuer);
    const auto         payload = signer.verify(it::data_string(response, "access_token"));
    REQUIRE(payload.has_value());
    CHECK(payload->sub == admin_id);
    CHECK(payload->role == infra::kAdminRole);
}

TEST_CASE("api_admin_auth: 管理员轮换 refresh token 后旧 token 被拒绝")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);
    const auto tokens = login_admin(admin_id);

    Json::Value body;
    body["refreshing"]   = true;
    body["access_token"] = tokens.access;

    auto first =
        it::post_json(kAdminRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));
    REQUIRE(first.status == 200);
    CHECK(!it::data_string(first, "refresh_token").empty());

    const auto&        jwt = infra::AppConfig::instance().jwt();
    infra::TokenSigner refresh_signer(jwt.admin_refresh_secret, jwt.issuer);
    const auto payload = refresh_signer.verify(it::data_string(first, "refresh_token"));
    REQUIRE(payload.has_value());
    CHECK(payload->sub == admin_id);
    CHECK(payload->role == infra::kAdminRole);

    Json::Value replay;
    replay["refreshing"]   = false;
    replay["access_token"] = tokens.access;

    auto second =
        it::post_json(kAdminRefresh, it::to_json(replay), it::auth_json_headers(tokens.refresh));

    CHECK(second.status == 401);
    CHECK(it::error_code(second) == "UNAUTHORIZED");
    CHECK(it::error_message(second).find("old") != std::string::npos);
}

//-----------------------------------------------------------------------------
// 登出（真实 Redis 吊销）
//-----------------------------------------------------------------------------

TEST_CASE("api_admin_auth: 管理员登出成功且重复登出被拒绝"
          * doctest::skip(!it::redis_available()))
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);
    const auto tokens = login_admin(admin_id);

    Json::Value body;
    body["refresh_token"] = tokens.refresh;

    auto first = it::post_json(kAdminLogout, it::to_json(body), it::auth_json_headers(tokens.access));
    CHECK(first.status == 204);
    CHECK(first.body.empty());

    auto second =
        it::post_json(kAdminLogout, it::to_json(body), it::auth_json_headers(tokens.access));
    CHECK(second.status == 401);
    CHECK(it::error_code(second) == "UNAUTHORIZED");
}

//-----------------------------------------------------------------------------
// 双密钥 / 角色隔离
//-----------------------------------------------------------------------------

TEST_CASE("api_admin_auth: 普通用户 access token 无法访问管理端")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret12");
    const auto user_access = login_user(user_id);

    auto response = it::post_json(kAdminLogout, "{}", it::auth_json_headers(user_access));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}

TEST_CASE("api_admin_auth: 管理员 access token 无法访问用户端")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);
    const auto tokens = login_admin(admin_id);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = tokens.access;

    auto response =
        it::post_json(kUserRefresh, it::to_json(body), it::auth_json_headers(tokens.access));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}

TEST_CASE("api_admin_auth: 用户 refresh token 无法刷新管理员 token")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret12");
    std::string user_refresh;
    const auto  user_access = login_user(user_id, &user_refresh);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = user_access;

    auto response =
        it::post_json(kAdminRefresh, it::to_json(body), it::auth_json_headers(user_refresh));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}

TEST_CASE("api_admin_auth: 管理员 refresh token 无法刷新用户 token")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, kPassword);
    const auto tokens = login_admin(admin_id);

    Json::Value body;
    body["refreshing"]   = false;
    body["access_token"] = tokens.access;

    auto response =
        it::post_json(kUserRefresh, it::to_json(body), it::auth_json_headers(tokens.refresh));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
}

TEST_CASE("api_admin_auth: 管理端缺少 Bearer 返回 401")
{
    prepare();

    auto response = it::post_json(kAdminLogout, "{}");

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
    CHECK(it::error_message(response).find("authorization") != std::string::npos);
}

TEST_CASE("api_admin_auth: 过期管理员 access token 返回 401")
{
    prepare();

    const auto&         jwt    = infra::AppConfig::instance().jwt();
    infra::TokenSigner  signer(jwt.admin_access_secret, jwt.issuer);
    const auto          expired = signer.sign(it::new_admin_id(), infra::kAdminRole, -60);

    auto response = it::post_json(kAdminLogout, "{}", it::auth_json_headers(expired.value));

    CHECK(response.status == 401);
    CHECK(it::error_code(response) == "UNAUTHORIZED");
    CHECK(it::error_message(response).find("invalid or expired") != std::string::npos);
}
