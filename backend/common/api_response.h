#pragma once

#include <drogon/HttpResponse.h>

#include <json/json.h>
#include <string>

#include "common/errors.h"

namespace common
{

inline drogon::HttpResponsePtr json_response(int status, const Json::Value& body)
{
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status));
    return resp;
}

/// 成功响应统一包装：{"data": ...}
inline drogon::HttpResponsePtr data_response(int status, const Json::Value& data)
{
    Json::Value body;
    body["data"] = data;
    return json_response(status, body);
}

/// 失败响应统一包装：{"error": {"code": "...", "message": "...", "details": {...}}}
inline drogon::HttpResponsePtr error_response(ErrorCode code, const std::string& message,
                                              const Json::Value& details = Json::Value())
{
    Json::Value err;
    err["code"]    = error_code_name(code);
    err["message"] = message;
    if (!details.isNull())
        err["details"] = details;

    Json::Value body;
    body["error"] = err;
    return json_response(http_status(code), body);
}

}  // namespace common
