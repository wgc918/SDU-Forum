#pragma once

#include <json/json.h>

#include <stdexcept>
#include <string>

namespace common
{

/// 业务错误码：与 HTTP 状态码解耦，便于客户端稳定判断
enum class ErrorCode
{
    InvalidJson,
    InvalidParam,
    Unauthorized,
    InvalidCredentials,
    AccountBanned,
    NotFound,
    Conflict,
    DatabaseError,
    InternalError,
    RedisError,
};

inline const char* error_code_name(ErrorCode code) noexcept
{
    switch (code)
    {
        case ErrorCode::InvalidJson:
            return "INVALID_JSON";
        case ErrorCode::InvalidParam:
            return "INVALID_PARAM";
        case ErrorCode::Unauthorized:
            return "UNAUTHORIZED";
        case ErrorCode::InvalidCredentials:
            return "INVALID_CREDENTIALS";
        case ErrorCode::AccountBanned:
            return "ACCOUNT_BANNED";
        case ErrorCode::NotFound:
            return "NOT_FOUND";
        case ErrorCode::Conflict:
            return "CONFLICT";
        case ErrorCode::DatabaseError:
            return "DB_ERROR";
        case ErrorCode::InternalError:
            return "INTERNAL_ERROR";
        case ErrorCode::RedisError:
            return " REDISE_RROR";
    }
    return "INTERNAL_ERROR";
}

inline int http_status(ErrorCode code) noexcept
{
    switch (code)
    {
        case ErrorCode::InvalidJson:
        case ErrorCode::InvalidParam:
            return 400;
        case ErrorCode::Unauthorized:
        case ErrorCode::InvalidCredentials:
            return 401;
        case ErrorCode::AccountBanned:
            return 403;
        case ErrorCode::NotFound:
            return 404;
        case ErrorCode::Conflict:
            return 409;
        case ErrorCode::DatabaseError:
        case ErrorCode::InternalError:
        case ErrorCode::RedisError:
            return 500;
    }
    return 500;
}

/// 业务异常：由 Service 层抛出，异常中间件统一转换为 JSON 响应
class AppException : public std::runtime_error
{
public:
    explicit AppException(ErrorCode code, const std::string& message,
                          const Json::Value& details = Json::Value())
        : std::runtime_error(message), m_code(code), m_details(details)
    {
    }

    ErrorCode code() const noexcept
    {
        return m_code;
    }

    const Json::Value& details() const noexcept
    {
        return m_details;
    }

private:
    ErrorCode   m_code;
    Json::Value m_details;
};

}  // namespace common
