#include "controllers/admin/auth_controller.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <json/value.h>

#include <functional>
#include <string>

#include "common/api_response.h"
#include "common/errors.h"
#include "common/param_validator.h"
#include "controllers/constants.h"
#include "services/admin/auth_service.h"
#include "utils/utils.h"

namespace admin::controller
{

AdminAuthController::AdminAuthController(admin::service::AdminAuthService& auth_service)
    : m_auth_service(auth_service)
{
}

void AdminAuthController::login(const drogon::HttpRequestPtr&                         req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    const std::string admin_id =
        param.field("admin_id").require_string().not_empty().equal_length(kAdminIdLength).trimmed();

    const std::string password = param.field("password")
                                     .require_string()
                                     .not_empty()
                                     .equal_length(kPasswordLength)
                                     .trimmed();

    auto result = m_auth_service.login(admin_id, password);

    Json::Value data;
    data["admin_id"]          = result.admin_id;
    data["token_type"]        = "Bearer";
    data["access_token"]      = result.access_token;
    data["access_token_exp"]  = Json::Int64(result.access_token_exp);
    data["refresh_token"]     = result.refresh_token;
    data["refresh_token_exp"] = Json::Int64(result.refresh_token_exp);

    callback(common::data_response(200, data));
}

void AdminAuthController::logout(const drogon::HttpRequestPtr&                         req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // 头部放access_token 载荷放refresh_token
    utils::BearerError bearer_error = utils::BearerError::None;
    auto               token = utils::parse_bearer(req->getHeader("authorization"), &bearer_error);
    if (!token)
        throw common::AppException(common::ErrorCode::Unauthorized,
                                   utils::bearer_error_message(bearer_error));

    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    const std::string refresh_token =
        param.field("refresh_token").require_string().not_empty().trimmed();

    m_auth_service.logout(refresh_token, *token);
    callback(common::no_content_response());
}

void AdminAuthController::refresh_token(
    const drogon::HttpRequestPtr&                         req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // 头部放 refresh_token 载荷放 access_token(便于将旧的吊销)
    utils::BearerError bearer_error = utils::BearerError::None;
    auto               token = utils::parse_bearer(req->getHeader("authorization"), &bearer_error);
    if (!token)
        throw common::AppException(common::ErrorCode::Unauthorized,
                                   utils::bearer_error_message(bearer_error));

    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    bool              refreshing = param.field("refreshing").require_bool().asBool();
    const std::string access_token =
        param.field("access_token").require_string().not_empty().trimmed();

    service::AdminLoginResult result =
        m_auth_service.refresh_token(*token, access_token, refreshing);
    Json::Value data;
    data["admin_id"]          = result.admin_id;
    data["refresh_token"]     = result.refresh_token;
    data["refresh_token_exp"] = result.refresh_token_exp;
    data["access_token"]      = result.access_token;
    data["access_token_exp"]  = result.access_token_exp;
    callback(common::data_response(200, data));
}

}  // namespace admin::controller
