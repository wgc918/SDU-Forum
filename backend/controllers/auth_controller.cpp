#include "controllers/auth_controller.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <json/value.h>

#include <functional>
#include <string>

#include "common/api_response.h"
#include "common/errors.h"
#include "common/param_validator.h"
#include "constants.h"
#include "middleware/auth_middleware.h"
#include "services/auth_service.h"
#include "utils/utils.h"

namespace controller
{

AuthController::AuthController(service::AuthService& auth_service) : m_auth_service(auth_service)
{
}

void AuthController::login(const drogon::HttpRequestPtr&                         req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    const std::string user_id =
        param.field("user_id").require_string().not_empty().equal_length(kUserIdLength).trimmed();

    const std::string password = param.field("password")
                                     .require_string()
                                     .not_empty()
                                     .equal_length(kPasswordLength)
                                     .trimmed();

    auto result = m_auth_service.login(user_id, password);

    Json::Value data;
    data["public_id"]         = result.public_id;
    data["token_type"]        = "Bearer";
    data["access_token"]      = result.access_token;
    data["access_token_exp"]  = Json::Int64(result.access_token_exp);
    data["refresh_token"]     = result.refresh_token;
    data["refresh_token_exp"] = Json::Int64(result.refresh_token_exp);

    callback(common::data_response(200, data));
}

void AuthController::signUp(const drogon::HttpRequestPtr&                         req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    const std::string user_id =
        param.field("user_id").require_string().not_empty().equal_length(kUserIdLength).trimmed();

    const std::string password = param.field("password")
                                     .require_string()
                                     .not_empty()
                                     .equal_length(kPasswordLength)
                                     .trimmed();

    const std::string public_id = m_auth_service.signUp(user_id, password);
    Json::Value       data;
    data["public_id"] = public_id;
    callback(common::created_response(data));
}

void AuthController::logout(const drogon::HttpRequestPtr&                         req,
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
    const std::string  refresh_token =
        param.field("refresh_token").require_string().not_empty().trimmed();

    m_auth_service.logout(refresh_token, *token);
    callback(common::no_content_response());
}

void AuthController::refresh_token(const drogon::HttpRequestPtr&                         req,
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

    service::LoginResult result = m_auth_service.refresh_token(*token, access_token, refreshing);

    Json::Value data;
    data["public_id"]         = result.public_id;
    data["refresh_token"]     = result.refresh_token;
    data["refresh_token_exp"] = result.refresh_token_exp;
    data["access_token"]      = result.access_token;
    data["access_token_exp"]  = result.access_token_exp;
    callback(common::data_response(200, data));
}

void AuthController::logoff(const drogon::HttpRequestPtr&                         req,
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
    const std::string  refresh_token =
        param.field("refresh_token").require_string().not_empty().trimmed();

    m_auth_service.logoff(middleware::current_public_id(req), refresh_token, *token);
    callback(common::no_content_response());
}
}  // namespace controller
