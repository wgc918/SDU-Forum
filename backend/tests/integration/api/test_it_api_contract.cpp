//-----------------------------------------------------------------------------
// 文件: test_it_api_contract.cpp
// 描述: L1 接口层 —— 路由/方法/JSON/错误信封等通用 HTTP 契约。
//-----------------------------------------------------------------------------
#include <json/json.h>

#include <string>

#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_test_common.h"

namespace
{

constexpr char kLogin[]      = "/sdu_forum/api/v1/auth/login";
constexpr char kSignUp[]     = "/sdu_forum/api/v1/auth/sign-up";
constexpr char kAdminLogin[] = "/admin/api/v1/auth/login";
constexpr char kAdminLogout[] = "/admin/api/v1/auth/logout";

void prepare()
{
    it::load_config();
    it::server();
    it::reset_db();
}

Json::Value login_body()
{
    Json::Value body;
    body["user_id"]  = it::new_user_id();
    body["password"] = "secret12";
    return body;
}

}  // namespace

TEST_CASE("api_contract: 未知路由返回 404")
{
    prepare();

    auto response = it::http().get("/sdu_forum/api/v1/not-exists");

    CHECK(response.status == 404);
}

TEST_CASE("api_contract: 已注册路径使用错误方法返回 405")
{
    prepare();

    auto response = it::http().get(kLogin);

    CHECK(response.status == 405);
}

TEST_CASE("api_contract: 未注册子路径返回 404")
{
    prepare();

    auto response = it::post_json("/sdu_forum/api/v1/auth/login/extra", it::to_json(login_body()));

    CHECK(response.status == 404);
}

TEST_CASE("api_contract: 非法 JSON 返回 400 INVALID_JSON")
{
    prepare();

    auto response = it::post_json(kLogin, "{");

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_JSON");
    REQUIRE(response.headers.count("content-type") == 1);
    CHECK(response.headers.at("content-type").find("application/json") != std::string::npos);
}

TEST_CASE("api_contract: 缺少 Content-Type 的 JSON 请求返回 400 INVALID_JSON")
{
    prepare();

    auto response = it::http().post(kLogin, it::to_json(login_body()));

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_JSON");
}

TEST_CASE("api_contract: 空请求体返回 400 INVALID_JSON")
{
    prepare();

    auto response = it::post_json(kLogin, "");

    CHECK(response.status == 400);
    CHECK(it::error_code(response) == "INVALID_JSON");
}

TEST_CASE("api_contract: 错误响应使用统一信封且不泄漏堆栈")
{
    prepare();

    auto response = it::post_json(kAdminLogout, "{}", {});

    CHECK(response.status == 401);
    REQUIRE(response.has_json());
    REQUIRE(response.json.isMember("error"));
    CHECK(!response.json.isMember("data"));

    const auto& error = response.json["error"];
    REQUIRE(error.isObject());
    CHECK(error["code"].isString());
    CHECK(error["message"].isString());
    CHECK(error["code"].asString() == "UNAUTHORIZED");

    for (const char* leaked : {"stack", "trace", "backtrace", "exception"})
        CHECK(response.body.find(leaked) == std::string::npos);
}

TEST_CASE("api_contract: 成功响应使用 data 信封与 JSON Content-Type")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret12");

    Json::Value body;
    body["user_id"]  = user_id;
    body["password"] = "secret12";

    auto response = it::post_json(kLogin, it::to_json(body));

    REQUIRE(response.status == 200);
    REQUIRE(response.headers.count("content-type") == 1);
    CHECK(response.headers.at("content-type").find("application/json") != std::string::npos);
    REQUIRE(response.has_json());
    CHECK(response.json.isMember("data"));
    CHECK(!response.json.isMember("error"));
}

TEST_CASE("api_contract: 请求体多余字段被忽略")
{
    prepare();

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret12");

    Json::Value body;
    body["user_id"]  = user_id;
    body["password"] = "secret12";
    body["remember"] = true;
    body["extra"]    = "ignored";

    auto response = it::post_json(kLogin, it::to_json(body));

    CHECK(response.status == 200);
}

TEST_CASE("api_contract: 超大请求体被拒绝且服务端仍可用")
{
    prepare();

    Json::Value body;
    body["user_id"]  = std::string(1200 * 1024, 'a');
    body["password"] = "x";

    bool rejected = false;
    int  status   = 0;
    try
    {
        auto response = it::post_json(kLogin, it::to_json(body));
        status        = response.status;
    }
    catch (const std::exception&)
    {
        // 服务端在接收完请求体之前即关闭连接，同样属于拒绝行为
        rejected = true;
    }

    CHECK((rejected || status == 413 || status == 400));

    auto after = it::post_json(kSignUp, it::to_json(login_body()));
    CHECK(after.status == 201);
}

TEST_CASE("api_contract: 未处理异常返回 500 且不暴露内部细节")
{
    prepare();

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "secret12");

    Json::Value credentials;
    credentials["admin_id"] = admin_id;
    credentials["password"] = "secret12";
    auto login              = it::post_json(kAdminLogin, it::to_json(credentials));
    REQUIRE(login.status == 200);
    const auto token = it::data_string(login, "access_token");

    // 不存在的 account_id 触发外键失败 → DB_ERROR → 500
    Json::Value body;
    body["reason_code"] = 1001;
    body["ban_type"]    = 0;
    body["operator_id"] = admin_id;
    body["source"]      = "manual";
    body["time_type"]   = 0xFFFF;
    body["time"]        = 0;

    auto response = it::post_json("/admin/api/v1/users/999999/ban", it::to_json(body),
                                  it::auth_json_headers(token));

    CHECK(response.status == 500);
    CHECK(it::error_code(response) == "DB_ERROR");
    CHECK(!it::error_message(response).empty());

    for (const char* leaked : {"stack", "trace", "backtrace", ".cpp", ".h:"})
        CHECK(response.body.find(leaked) == std::string::npos);
}
