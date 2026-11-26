//-----------------------------------------------------------------------------
// 文件: it_test_common.h
// 描述: 集成测试用例共用的小工具（响应解包、请求头、唯一标识）。
//-----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "http_client.h"
#include "it_support.h"
#include "server_process.h"

namespace it
{

/// 纯 JSON 请求头（无鉴权）
inline std::map<std::string, std::string> json_headers()
{
    return {{"Content-Type", "application/json"}};
}

/// 带鉴权的 JSON 请求头
inline std::map<std::string, std::string> auth_json_headers(const std::string& token)
{
    auto headers             = json_headers();
    headers["Authorization"] = "Bearer " + token;
    return headers;
}

/// 紧凑序列化 JSON 请求体
inline std::string to_json(const Json::Value& value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

/// 发送 JSON POST（默认自动带 Content-Type）
inline HttpResponse post_json(const std::string&                        path,
                              const std::string&                        body,
                              const std::map<std::string, std::string>& headers = json_headers())
{
    return http().post(path, body, headers);
}

/// 读取响应 data 节点；缺失返回静态 null
inline const Json::Value& data_of(const HttpResponse& response)
{
    static const Json::Value kNull;
    if (!response.has_json())
        return kNull;
    const auto& data = response.json["data"];
    return data;
}

/// 读取 data.<key> 的数值；缺失或非数值返回 0
inline int64_t data_int64(const HttpResponse& response, const char* key)
{
    const auto& data = data_of(response);
    if (!data.isObject() || !data[key].isNumeric())
        return 0;
    return data[key].asInt64();
}

/// 读取 data.<key> 的布尔值；缺失返回 false
inline bool data_bool(const HttpResponse& response, const char* key)
{
    const auto& data = data_of(response);
    return data.isObject() && data[key].isBool() && data[key].asBool();
}

/// 读取 error.code；缺失返回空串
inline std::string error_code(const HttpResponse& response)
{
    if (!response.has_json())
        return "";
    const auto& error = response.json["error"];
    if (!error.isObject() || !error["code"].isString())
        return "";
    return error["code"].asString();
}

/// 读取 error.message；缺失返回空串
inline std::string error_message(const HttpResponse& response)
{
    if (!response.has_json())
        return "";
    const auto& error = response.json["error"];
    if (!error.isObject() || !error["message"].isString())
        return "";
    return error["message"].asString();
}

/// 读取 data.<key> 的字符串值；缺失返回空串
inline std::string data_string(const HttpResponse& response, const char* key)
{
    if (!response.has_json())
        return "";
    const auto& data = response.json["data"];
    if (!data.isObject() || !data[key].isString())
        return "";
    return data[key].asString();
}

/// data 是否为对象
inline bool has_data_object(const HttpResponse& response)
{
    return response.has_json() && response.json["data"].isObject();
}

/// data 是否为数组
inline bool has_data_array(const HttpResponse& response)
{
    return response.has_json() && response.json["data"].isArray();
}

}  // namespace it
