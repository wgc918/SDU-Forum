#include "controllers/auth_controller.h"

#include <json/json.h>

#include <string>

#include "common/api_response.h"
#include "common/errors.h"
#include "services/auth_service.h"

namespace controller
{
namespace
{

constexpr std::size_t kMaxUserIdLength = 64;
constexpr std::size_t kMaxPasswordLength = 128;

std::string trim(const std::string& value)
{
    std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

}  // namespace

AuthController::AuthController(service::AuthService& auth_service) : m_auth_service(auth_service)
{
}

void AuthController::login(const drogon::HttpRequestPtr&                       req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    const auto& body = *json;
    if (!body["user_id"].isString() || !body["password"].isString())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "user_id and password are required and must be strings");

    const std::string user_id  = trim(body["user_id"].asString());
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
    data["user_id"]           = result.user_id;
    data["token_type"]        = "Bearer";
    data["access_token"]      = result.access_token;
    data["access_token_exp"]  = Json::Int64(result.access_token_exp);
    data["refresh_token"]     = result.refresh_token;
    data["refresh_token_exp"] = Json::Int64(result.refresh_token_exp);

    callback(common::data_response(200, data));
}

}  // namespace controller
