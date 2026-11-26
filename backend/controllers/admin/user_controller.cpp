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
#include "common/param_validator.h"
#include "controllers/constants.h"
#include "middleware/admin_auth_middleware.h"
#include "services/admin/user_service.h"
#include "utils/utils.h"

namespace admin::controller
{
namespace
{

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

}  // namespace

AdminUserController::AdminUserController(admin::service::AdminUserService& admin_user_service)
    : m_service(admin_user_service)
{
}

void AdminUserController::ban(const drogon::HttpRequestPtr&                         req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const uint64_t                                        account_id)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    const uint64_t reason_code =
        static_cast<uint16_t>(param.field("reason_code").require_int().asInt());
    const uint8_t ban_type = static_cast<uint8_t>(param.field("ban_type").require_int().asInt());
    const std::string operator_id = param.field("operator_id")
                                        .require_string()
                                        .not_empty()
                                        .equal_length(kAdminIdLength)
                                        .trimmed();
    const std::string source      = param.field("source").require_string().not_empty().trimmed();
    const uint16_t    time_type   = param.field("time_type").require_uint64().asUint64();
    const uint32_t    time        = param.field("time").require_uint64().asUint64();

    // 可选字段
    std::string    reason = param.field("reason")
                                .optional()
                                .require_string()
                                .not_empty()
                                .max_length(kReasonMaxLength)
                                .trimmed();
    const uint64_t ref_id = param.field("ref_id").optional().require_uint64().asUint64();

    service::BanMeta meta;
    meta.reason_code = static_cast<ban::ReasonCode>(reason_code);
    meta.type        = static_cast<ban::BanType>(ban_type);
    meta.time_type   = time_type;

    meta.time   = time;
    meta.reason = reason;
    meta.source = ban::string_to_ban_source(source);
    meta.ref_id = ref_id;

    m_service.ban(account_id, middleware::current_admin_id(req), meta);
    callback(common::no_content_response());
}

void AdminUserController::unban(const drogon::HttpRequestPtr&                         req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const uint64_t                                        account_id)
{
    auto json = req->getJsonObject();
    if (json == nullptr)
        throw common::AppException(common::ErrorCode::InvalidJson,
                                   "request body must be a valid JSON object");

    common::BodyParams param(*json);

    const uint8_t     type = static_cast<uint8_t>(param.field("ban_type").require_int().asInt());
    const std::string reason =
        param.field("reason").require_string().not_empty().max_length(kReasonMaxLength).trimmed();

    m_service.unban(account_id, middleware::current_admin_id(req), static_cast<ban::BanType>(type),
                    reason);
    callback(common::no_content_response());
}

void AdminUserController::get_history(
    const drogon::HttpRequestPtr&                         req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback, const uint64_t account_id)
{
}

void AdminUserController::get_blacklist(
    const drogon::HttpRequestPtr&                         req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
}

}  // namespace admin::controller
