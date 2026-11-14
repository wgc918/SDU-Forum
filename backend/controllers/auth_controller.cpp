#include "controllers/auth_controller.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <json/value.h>

#include <functional>
#include <iostream>
#include <string>

#include "common/api_response.h"
#include "common/errors.h"
#include "middleware/auth_middleware.h"
#include "services/auth_service.h"
#include "utils/utils.h"

namespace controller
{
namespace
{

constexpr std::size_t kMaxUserIdLength   = 64;
constexpr std::size_t kMaxPasswordLength = 128;

}  // namespace

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

    const auto& body = *json;
    if (!body.isMember("user_id"))
        throw common::AppException(common::ErrorCode::InvalidJson, "missing param: user_id");
    if (!body.isMember("password"))
        throw common::AppException(common::ErrorCode::InvalidJson, "missing param: password");
    if (!body["user_id"].isString() || !body["password"].isString())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password are required and must be strings");

    const std::string user_id  = utils::trim(body["user_id"].asString());
    const std::string password = body["password"].asString();

    if (user_id.empty() || password.empty())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password must not be empty");
    if (user_id.size() > kMaxUserIdLength)
        throw common::AppException(common::ErrorCode::InvalidParam, "user_id is too long");
    if (password.size() > kMaxPasswordLength)
        throw common::AppException(common::ErrorCode::InvalidParam, "password is too long");

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

    const auto& body = *json;
    if (!body.isMember("user_id"))
        throw common::AppException(common::ErrorCode::InvalidJson, "missing param: user_id");
    if (!body.isMember("password"))
        throw common::AppException(common::ErrorCode::InvalidJson, "missing param: password");
    if (!body["user_id"].isString() || !body["password"].isString())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password are required and must be strings");

    const std::string user_id  = utils::trim(body["user_id"].asString());
    const std::string password = body["password"].asString();

    if (user_id.empty() || password.empty())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password must not be empty");
    if (user_id.size() > kMaxUserIdLength)
        throw common::AppException(common::ErrorCode::InvalidParam, "user_id is too long");
    if (password.size() > kMaxPasswordLength)
        throw common::AppException(common::ErrorCode::InvalidParam, "password is too long");

    bool        state = m_auth_service.signUp(user_id, password);
    Json::Value data;
    data["state"] = state;
    callback(common::data_response(200, data));
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
    const Json::Value body = *json;

    if (!body.isMember("refresh_token"))
        throw common::AppException(common::ErrorCode::InvalidParam, "missing param: refresh_token");
    if (!body["refresh_token"].isString())
        throw common::AppException(common::ErrorCode::InvalidParam, "refresh_token must be string");
    const std::string refresh_token = body["refresh_token"].asString();
    if (refresh_token.empty())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "refresh_token must not be empty");
    bool        state = m_auth_service.logout(refresh_token, *token);
    Json::Value data;
    data["state"] = state;
    callback(common::data_response(200, data));
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
    const Json::Value body = *json;
    if (!body.isMember("refreshing"))
        throw common::AppException(common::ErrorCode::InvalidParam, "missing param: refreshing");
    if (!body.isMember("access_token"))
        throw common::AppException(common::ErrorCode::InvalidParam, "missing param: access_token");
    if (!body["refreshing"].isBool())
        throw common::AppException(common::ErrorCode::InvalidParam, "refreshing must be bool");
    if (!body["access_token"].isString())
        throw common::AppException(common::ErrorCode::InvalidParam, "access_token must be string");
    bool              refreshing   = body["refreshing"].asBool();
    const std::string access_token = body["access_token"].asString();

    service::LoginResult result = m_auth_service.refresh_token(*token, access_token, refreshing);
    Json::Value          data;
    if (refreshing)
    {
        data["public_id"]         = result.public_id;
        data["refresh_token"]     = result.refresh_token;
        data["refresh_token_exp"] = result.refresh_token_exp;
    }
    data["access_token"]     = result.access_token;
    data["access_token_exp"] = result.access_token_exp;
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
    const Json::Value body = *json;

    if (!body.isMember("refresh_token"))
        throw common::AppException(common::ErrorCode::InvalidParam, "missing param: refresh_token");
    if (!body["refresh_token"].isString())
        throw common::AppException(common::ErrorCode::InvalidParam, "refresh_token must be string");
    const std::string refresh_token = body["refresh_token"].asString();
    if (refresh_token.empty())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "refresh_token must not be empty");

    bool state = m_auth_service.logoff(middleware::current_public_id(req), refresh_token, *token);
    Json::Value data;
    data["state"] = state;
    callback(common::data_response(200, data));
}
}  // namespace controller
