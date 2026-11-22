#include "controllers/admin/user_controller.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <json/value.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "common/api_response.h"
#include "common/ban_enums.h"
#include "common/errors.h"
#include "middleware/admin_auth_middleware.h"
#include "services/admin/user_service.h"
#include "utils/utils.h"

namespace admin::controller
{
namespace
{

constexpr int         kDefaultPage     = 1;
constexpr int         kDefaultPageSize = 20;
constexpr int         kMaxPageSize     = 100;
constexpr std::size_t kMaxReasonLength = 255;

/// 读取必填 JSON 字段
/// @throws common::AppException 字段缺失
const Json::Value& require_member(const Json::Value& body, const char* name)
{
    if (!body.isMember(name))
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   std::string("missing param: ") + name);
    return body[name];
}

/// 将 JSON 值解析为非负整数（支持数字与纯数字字符串）
/// @throws common::AppException 类型或取值非法
uint64_t json_to_uint64(const Json::Value& value, const char* name)
{
    if (value.isUInt64())
        return value.asUInt64();

    if (value.isString())
    {
        const std::string text = utils::trim(value.asString());
        if (!text.empty() && text.find_first_not_of("0123456789") == std::string::npos)
        {
            try
            {
                return std::stoull(text);
            }
            catch (const std::exception&)
            {
            }
        }
    }

    throw common::AppException(common::ErrorCode::InvalidParam,
                               std::string(name) + " must be a non-negative integer");
}

/// 读取必填非负整数字段
uint64_t require_uint64(const Json::Value& body, const char* name)
{
    return json_to_uint64(require_member(body, name), name);
}

/// 读取可选非负整数字段
std::optional<uint64_t> optional_uint64(const Json::Value& body, const char* name)
{
    if (!body.isMember(name) || body[name].isNull())
        return std::nullopt;
    return json_to_uint64(body[name], name);
}

/// 读取可选字符串字段
std::optional<std::string> optional_string(const Json::Value& body, const char* name)
{
    if (!body.isMember(name) || body[name].isNull())
        return std::nullopt;
    if (!body[name].isString())
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   std::string(name) + " must be string");
    return body[name].asString();
}

/// 读取查询参数；缺失或空白返回 nullopt
std::optional<std::string> query_optional(const drogon::HttpRequestPtr& req, const char* name)
{
    auto value = req->getOptionalParameter<std::string>(name);
    if (!value)
        return std::nullopt;

    std::string text = utils::trim(*value);
    if (text.empty())
        return std::nullopt;
    return text;
}

/// 读取查询参数中的整数
/// @throws common::AppException 取值非法或超出范围
int query_int(const drogon::HttpRequestPtr& req, const char* name, int default_value, int min_value,
              int max_value)
{
    auto text = query_optional(req, name);
    if (!text)
        return default_value;
    if (text->find_first_not_of("0123456789") != std::string::npos)
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   std::string(name) + " must be a non-negative integer");

    int value = 0;
    try
    {
        value = std::stoi(*text);
    }
    catch (const std::exception&)
    {
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   std::string(name) + " is out of range");
    }

    if (value < min_value || value > max_value)
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   std::string(name) + " is out of range");
    return value;
}

/// 从请求体解析封禁维度
ban::BanType json_to_ban_type(const Json::Value& body, const char* name, bool required)
{
    std::optional<uint64_t> raw = required ? std::optional<uint64_t>{require_uint64(body, name)}
                                           : optional_uint64(body, name);
    if (!raw)
        return ban::BanType::All;

    if (*raw > 0xFF || !ban::IsBanTypeValid(static_cast<uint8_t>(*raw)))
        throw common::AppException(common::ErrorCode::InvalidParam, std::string("invalid ") + name);
    return static_cast<ban::BanType>(*raw);
}

/// 从查询参数解析封禁维度
ban::BanType query_to_ban_type(const drogon::HttpRequestPtr& req, const char* name)
{
    auto text = query_optional(req, name);
    if (!text)
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   std::string("missing param: ") + name);

    int value = query_int(req, name, 0, 0, 0xFF);
    if (!ban::IsBanTypeValid(static_cast<uint8_t>(value)))
        throw common::AppException(common::ErrorCode::InvalidParam, std::string("invalid ") + name);
    return static_cast<ban::BanType>(value);
}

/// 解析封禁来源，缺省为管理员手动
ban::BanSource parse_ban_source(const std::optional<std::string>& text)
{
    if (!text || *text == "manual")
        return ban::BanSource::Manual;
    if (*text == "auto")
        return ban::BanSource::Auto;
    if (*text == "report")
        return ban::BanSource::Report;

    throw common::AppException(common::ErrorCode::InvalidParam, "invalid source");
}

/// 解析封禁状态（支持 active/expired/revoked 与 1/2/0）
std::optional<ban::BanStatus> parse_ban_status(const std::optional<std::string>& text)
{
    if (!text)
        return std::nullopt;

    if (*text == "active")
        return ban::BanStatus::Active;
    if (*text == "expired")
        return ban::BanStatus::Expired;
    if (*text == "revoked")
        return ban::BanStatus::Revoked;

    if (text->find_first_not_of("0123456789") == std::string::npos)
    {
        int value = 0;
        try
        {
            value = std::stoi(*text);
        }
        catch (const std::exception&)
        {
            throw common::AppException(common::ErrorCode::InvalidParam, "invalid status");
        }

        if (value == static_cast<int>(ban::BanStatus::Active) ||
            value == static_cast<int>(ban::BanStatus::Expired) ||
            value == static_cast<int>(ban::BanStatus::Revoked))
            return static_cast<ban::BanStatus>(value);
    }

    throw common::AppException(common::ErrorCode::InvalidParam, "invalid status");
}

/// 可选时间字段序列化
Json::Value optional_to_json(const std::optional<std::string>& value)
{
    return value ? Json::Value(*value) : Json::Value(Json::nullValue);
}

/// 黑名单记录序列化
Json::Value entry_to_json(const repository::BlacklistEntry& entry)
{
    Json::Value item;
    item["id"]           = Json::UInt64(entry.id);
    item["account_id"]   = Json::UInt64(entry.account_id);
    item["reason"]       = entry.reason;
    item["reason_code"]  = static_cast<Json::UInt>(entry.reason_code);
    item["ban_type"]     = static_cast<Json::UInt>(entry.ban_type);
    item["banned_by"]    = optional_to_json(entry.banned_by);
    item["banned_at"]    = entry.banned_at;
    item["expire_at"]    = optional_to_json(entry.expire_at);
    item["unbanned_at"]  = optional_to_json(entry.unbanned_at);
    item["unbanned_by"]  = optional_to_json(entry.unbanned_by);
    item["unban_reason"] = optional_to_json(entry.unban_reason);
    item["status"]       = static_cast<Json::UInt>(entry.status);
    item["source"]       = ban::ban_source_to_string(entry.source);
    item["ref_id"]       = Json::UInt64(entry.ref_id);
    return item;
}

/// 批量序列化
Json::Value entries_to_json(const std::vector<repository::BlacklistEntry>& entries)
{
    Json::Value items(Json::arrayValue);
    for (const auto& entry : entries)
        items.append(entry_to_json(entry));
    return items;
}

/// 从查询参数读取账号 ID
/// @throws common::AppException 缺失或非法
uint64_t require_query_account_id(const drogon::HttpRequestPtr& req)
{
    auto text = query_optional(req, "account_id");
    if (!text)
        throw common::AppException(common::ErrorCode::InvalidParam, "missing param: account_id");
    if (text->find_first_not_of("0123456789") != std::string::npos)
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "account_id must be a non-negative integer");

    try
    {
        return std::stoull(*text);
    }
    catch (const std::exception&)
    {
        throw common::AppException(common::ErrorCode::InvalidParam, "account_id is out of range");
    }
}

}  // namespace

AdminUserController::AdminUserController(admin::service::AdminUserService& admin_user_service)
    : m_service(admin_user_service)
{
}

void AdminUserController::ban(const drogon::HttpRequestPtr&                         req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    const Json::Value& body = *json;

    const uint64_t account_id = require_uint64(body, "account_id");
    if (account_id == 0)
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "account_id must be greater than 0");

    const uint64_t raw_reason = require_uint64(body, "reason_code");
    if (raw_reason > 0xFFFF || !ban::IsReasonCodeValid(static_cast<uint16_t>(raw_reason)))
        throw common::AppException(common::ErrorCode::InvalidParam, "invalid reason_code");

    const ban::BanType type = json_to_ban_type(body, "ban_type", true);

    const uint64_t raw_time_type = require_uint64(body, "time_type");
    if (raw_time_type > 0xFFFF)
        throw common::AppException(common::ErrorCode::InvalidParam, "invalid time_type");

    service::BanMeta meta;
    meta.reason      = static_cast<ban::ReasonCode>(raw_reason);
    meta.type        = type;
    meta.time_type   = static_cast<uint16_t>(raw_time_type);
    meta.reason_text = optional_string(body, "reason").value_or("");
    meta.source      = parse_ban_source(optional_string(body, "source"));

    if (auto ref_id = optional_uint64(body, "ref_id"))
        meta.ref_id = *ref_id;

    if (meta.time_type == 0xFFFF)
        meta.time = 0;
    else
    {
        const uint64_t raw_time = require_uint64(body, "time");
        if (raw_time == 0 || raw_time > 0xFFFFFFFF)
            throw common::AppException(common::ErrorCode::InvalidParam, "invalid time");
        meta.time = static_cast<uint32_t>(raw_time);
    }

    if (meta.reason_text.size() > kMaxReasonLength)
        throw common::AppException(common::ErrorCode::InvalidParam, "reason is too long");

    m_service.ban(account_id, middleware::current_admin_id(req), meta);
    callback(common::no_content_response());
}

void AdminUserController::unban(const drogon::HttpRequestPtr&                         req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    const Json::Value& body = *json;

    const uint64_t account_id = require_uint64(body, "account_id");
    if (account_id == 0)
        throw common::AppException(common::ErrorCode::InvalidParam,
                                   "account_id must be greater than 0");

    const ban::BanType type = json_to_ban_type(body, "ban_type", false);

    const std::string unban_reason = optional_string(body, "unban_reason").value_or("");
    if (unban_reason.size() > kMaxReasonLength)
        throw common::AppException(common::ErrorCode::InvalidParam, "unban_reason is too long");

    m_service.unban(account_id, middleware::current_admin_id(req), type, unban_reason);
    callback(common::no_content_response());
}

void AdminUserController::get_status(const drogon::HttpRequestPtr&                         req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const uint64_t account_id = require_query_account_id(req);

    Json::Value data;
    data["account_id"] = Json::UInt64(account_id);
    data["status"]     = ban::ban_status_name(m_service.get_user_status(account_id));
    callback(common::data_response(200, data));
}

void AdminUserController::get_history(
    const drogon::HttpRequestPtr&                         req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const uint64_t account_id = require_query_account_id(req);

    callback(common::data_response(200, entries_to_json(m_service.get_ban_history(account_id))));
}

void AdminUserController::get_blacklist(
    const drogon::HttpRequestPtr&                         req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const int page      = query_int(req, "page", kDefaultPage, 1, 1000000);
    const int page_size = query_int(req, "page_size", kDefaultPageSize, 1, kMaxPageSize);

    const auto raw_reason = query_optional(req, "reason_code");
    const auto raw_type   = query_optional(req, "ban_type");
    const auto status     = parse_ban_status(query_optional(req, "status"));
    const auto from       = query_optional(req, "from");
    const auto to         = query_optional(req, "to");

    std::optional<ban::ReasonCode> reason;
    if (raw_reason)
    {
        const int value = query_int(req, "reason_code", 0, 0, 0xFFFF);
        if (!ban::IsReasonCodeValid(static_cast<uint16_t>(value)))
            throw common::AppException(common::ErrorCode::InvalidParam, "invalid reason_code");
        reason = static_cast<ban::ReasonCode>(value);
    }

    std::optional<ban::BanType> type;
    if (raw_type)
        type = query_to_ban_type(req, "ban_type");

    std::vector<repository::BlacklistEntry> entries;
    if (reason)
    {
        entries = m_service.search_blacklist_by_reason(*reason, from, to, status, page, page_size);
    }
    else if (type)
    {
        entries = m_service.search_blacklist_by_type(*type, from, to, status, page, page_size);
    }
    else
    {
        // 列表接口仅支持分页；按状态/时间过滤时必须指定 reason_code 或 ban_type
        if (status || from || to)
            throw common::AppException(common::ErrorCode::InvalidParam,
                                       "reason_code or ban_type is required when filtering");
        entries = m_service.list_blacklist(page, page_size);
    }

    Json::Value data;
    data["page"]      = page;
    data["page_size"] = page_size;
    data["items"]     = entries_to_json(entries);
    callback(common::data_response(200, data));
}

}  // namespace admin::controller
