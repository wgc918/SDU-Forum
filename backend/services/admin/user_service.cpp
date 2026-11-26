#include "user_service.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "common/ban_enums.h"
#include "common/errors.h"
#include "log.h"
#include "repositories/blacklist_repository.h"

namespace
{

/// 永久封禁的时间单位标记
constexpr uint16_t kPermanentTimeType = 0xFFFF;

/**
 * @brief 将当前时间加上指定的时间，返回格式化后的字符串
 *
 * @param time_type 时间单位: 0-分钟(m), 1-小时(h), 2-天(d)
 * @param time      要加上的时间数值
 * @return std::string 格式为 "YYYY-MM-DD HH:MM:SS" 的时间字符串
 */
std::string add_time_to_now(uint16_t time_type, uint32_t time)
{
    auto now = std::chrono::system_clock::now();

    std::chrono::seconds offset{0};
    switch (time_type)
    {
        case 0:
            offset = std::chrono::seconds(static_cast<long long>(time) * 60);
            break;
        case 1:
            offset = std::chrono::seconds(static_cast<long long>(time) * 3600);
            break;
        case 2:
            offset = std::chrono::seconds(static_cast<long long>(time) * 86400);
            break;
        default:
            break;
    }

    auto        new_time = now + offset;
    std::time_t tt       = std::chrono::system_clock::to_time_t(new_time);

    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    char buf[32];  // "YYYY-MM-DD HH:MM:SS" 共 19 字符 + '\0'
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", local_tm.tm_year + 1900,
                  local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min,
                  local_tm.tm_sec);

    return std::string(buf);
}

/// 根据封禁时长计算过期时间；永久封禁返回 PERMANENT_EXPIRE
/// @throws common::AppException 时长参数非法
std::string resolve_expire_at(const admin::service::BanMeta& data)
{
    if (data.time_type == kPermanentTimeType)
        return repository::PERMANENT_EXPIRE;

    if (data.time_type > 2 || data.time == 0)
        throw common::AppException(common::ErrorCode::InvalidParam, "invalid ban duration");

    return add_time_to_now(data.time_type, data.time);
}

/// 取写库用的原因原文：自定义原因优先，否则使用标准文案兜底
std::string resolve_reason_text(const admin::service::BanMeta& data)
{
    if (!data.reason.empty() && (data.reason_code == ban::ReasonCode::Unspecified ||
                                 data.reason_code == ban::ReasonCode::ManualCustom))
        return data.reason;
    return ban::ReasonDisplayText(data.reason_code);
}

}  // namespace

namespace admin::service
{

AdminUserService::AdminUserService(repository::BlacklistRepository& blacklist_repo)
    : m_blacklist_repo(blacklist_repo)
{
}

/// 将用户加入黑名单（按 ban_type 维度幂等：已存在同维度生效记录则拒绝）
/// @throws common::AppException 参数非法 / 重复封禁 / 数据库不可用
void AdminUserService::ban(uint64_t account_id, const std::string& operator_id, const BanMeta& data)
{
    ban::BanRecordInput input;
    input.reason_code = static_cast<uint16_t>(data.reason_code);
    input.reason      = resolve_reason_text(data);
    input.ban_type    = static_cast<uint8_t>(data.type);
    input.source      = data.source;

    std::string error;
    if (!ban::ValidateBanInput(input, &error))
    {
        LOGW("ban failed: invalid input, account_id=%llu, error=%s",
             static_cast<unsigned long long>(account_id), error.c_str());
        throw common::AppException(common::ErrorCode::InvalidParam, error);
    }

    /// BanType::All 在语义上是所有维度的并集
    /// 已有all则不能再插入
    /// 已有具体类型all能覆盖
    if (m_blacklist_repo.exists_active(account_id, ban::BanType::All))
    {
        LOGW("ban failed: account already banned, account_id=%llu, ban_type=%u",
             static_cast<unsigned long long>(account_id), static_cast<unsigned>(data.type));
        throw common::AppException(common::ErrorCode::Conflict, "account is already banned");
    }

    if (m_blacklist_repo.exists_active(account_id, data.type))
    {
        LOGW("ban failed: account already banned, account_id=%llu, ban_type=%u",
             static_cast<unsigned long long>(account_id), static_cast<unsigned>(data.type));
        throw common::AppException(common::ErrorCode::Conflict, "account is already banned");
    }

    const std::string expire_at = resolve_expire_at(data);

    repository::BlacklistEntry entry;
    entry.account_id  = account_id;
    entry.reason      = input.reason;
    entry.reason_code = data.reason_code;
    entry.ban_type    = data.type;
    entry.banned_by   = operator_id;
    entry.expire_at   = expire_at;
    entry.status      = ban::BanStatus::Active;
    entry.source      = data.source;
    entry.ref_id      = data.ref_id;

    if (!m_blacklist_repo.insert(entry))
    {
        LOGW("ban failed: insert blacklist failed, account_id=%llu",
             static_cast<unsigned long long>(account_id));
        throw common::AppException(common::ErrorCode::DatabaseError, "failed to ban account");
    }

    LOGI("ban succeeded: account_id=%llu, ban_type=%u, operator_id=%s",
         static_cast<unsigned long long>(account_id), static_cast<unsigned>(data.type),
         operator_id.c_str());
}

/// 解除封禁（按 ban_type 维度；ban_type=All 时解除全部）
/// @throws common::AppException 无生效记录 / 数据库不可用
void AdminUserService::unban(uint64_t account_id, const std::string& operator_id, ban::BanType type,
                             const std::string& unban_reason)
{
    /// 1.封禁all 解除单一维度无效
    /// 2.封禁单一维度（含多个） 用all可全部解除
    const bool has_active = m_blacklist_repo.exists_active(account_id, type) ||
                            m_blacklist_repo.is_blacklisted(account_id);
    if (!has_active)
    {
        LOGW("unban failed: no active ban, account_id=%llu, ban_type=%u",
             static_cast<unsigned long long>(account_id), static_cast<unsigned>(type));
        throw common::AppException(common::ErrorCode::NotFound, "no active ban record");
    }

    const int affected = m_blacklist_repo.unban(account_id, type, operator_id, unban_reason);
    if (affected <= 0)
    {
        LOGW("unban failed: no record updated, account_id=%llu",
             static_cast<unsigned long long>(account_id));
        throw common::AppException(common::ErrorCode::NotFound, "no active ban record");
    }

    LOGI("unban succeeded: account_id=%llu, ban_type=%u, operator_id=%s",
         static_cast<unsigned long long>(account_id), static_cast<unsigned>(type),
         operator_id.c_str());
}

/// 获取某维度的生效封禁详情
std::optional<repository::BlacklistEntry> AdminUserService::get_deatil(uint64_t     account_id,
                                                                       ban::BanType type) const
{
    return m_blacklist_repo.get_detail(account_id, type);
}

/// 获取用户全部封禁历史（含已解除/已过期）
BanPageResult AdminUserService::get_ban_history(uint64_t account_id) const
{
    return {};
}

/// 分页查询黑名单列表（仅生效中）
BanPageResult AdminUserService::list_blacklist(int page, int page_size) const
{
    repository::BlacklistFilter filter;
    filter.status = ban::BanStatus::Active;
    return {};
}

/// 按封禁原因/时间范围过滤查询
BanPageResult AdminUserService::search_blacklist_by_reason(ban::ReasonCode                   reason,
                                                           const std::optional<std::string>& from,
                                                           const std::optional<std::string>& to,
                                                           std::optional<ban::BanStatus>     status,
                                                           int page, int page_size) const
{
    repository::BlacklistFilter filter;
    filter.reason     = reason;
    filter.from       = from;
    filter.to         = to;
    filter.status     = status;
    filter.account_id = std::nullopt;

    return {};
}

/// 按封禁类型/时间范围过滤查询
BanPageResult AdminUserService::search_blacklist_by_type(ban::BanType                      type,
                                                         const std::optional<std::string>& from,
                                                         const std::optional<std::string>& to,
                                                         std::optional<ban::BanStatus>     status,
                                                         int page, int page_size) const
{
    repository::BlacklistFilter filter;
    filter.type       = type;
    filter.from       = from;
    filter.to         = to;
    filter.status     = status;
    filter.account_id = std::nullopt;

    return {};
}

}  // namespace admin::service
