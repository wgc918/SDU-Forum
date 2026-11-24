#pragma once

#include <cstdint>
#include <vector>

#include "common/ban_enums.h"
#include "repositories/blacklist_repository.h"

namespace admin::service
{
struct BanMeta
{
    std::string     reason;  // 自定义原因原文（code=Unspecified/ManualCustom 时必填）
    ban::ReasonCode reason_code = ban::ReasonCode::Unspecified;
    ban::BanType    type        = ban::BanType::All;
    uint16_t        time_type   = 0;  // 0=分钟 1=小时 2=天 0xFFFF=永久
    uint32_t        time        = 0;  // 时长数值；永久时忽略
    uint64_t        ref_id      = 0;  // 关联工单/举报 ID，0=无
    ban::BanSource  source      = ban::BanSource::Manual;
};

class AdminUserService
{
public:
    AdminUserService(repository::BlacklistRepository& blacklist_repo);

    // ---------- 黑名单操作 ----------

    /// 将用户加入黑名单（按 ban_type 维度幂等：已存在同维度生效记录则拒绝）
    /// @param user_id     被拉黑的用户 ID
    /// @param operator_id 执行操作的管理员 ID
    /// @param reason      拉黑原因（用于审计）
    void ban(uint64_t account_id, const std::string& operator_id, const BanMeta& data);

    /// 解除封禁（按 ban_type 维度；ban_type=All 时解除全部）
    /// @param user_id     被解除的用户 ID
    /// @param operator_id 执行操作的管理员 ID
    void unban(uint64_t account_id, const std::string& operator_id,
               ban::BanType type = ban::BanType::All, const std::string& unban_reason = "");

    /// 获取某维度的生效封禁详情
    std::optional<repository::BlacklistEntry> get_active(uint64_t     account_id,
                                                         ban::BanType type) const;
    /// 获取用户全部封禁历史（含已解除/已过期）
    std::vector<repository::BlacklistEntry> get_ban_history(uint64_t account_id) const;

    /// 分页查询黑名单列表（仅生效中）
    /// @param page      页码（从 1 开始）
    /// @param page_size 每页条数
    std::vector<repository::BlacklistEntry> list_blacklist(int page, int page_size) const;

    /// 按封禁原因/时间范围过滤查询
    /// @param keyword  匹配 reason / account_id 字符串
    /// @param from     起始时间（含），空则不限
    /// @param to       结束时间（含），空则不限
    /// @param status   过滤状态，nullopt=全部
    std::vector<repository::BlacklistEntry> search_blacklist_by_reason(
        ban::ReasonCode reason, const std::optional<std::string>& from,
        const std::optional<std::string>& to, std::optional<ban::BanStatus> status, int page,
        int page_size) const;

    /// 按封禁类型/时间范围过滤查询
    /// @param keyword  匹配 reason / account_id 字符串
    /// @param from     起始时间（含），空则不限
    /// @param to       结束时间（含），空则不限
    /// @param status   过滤状态，nullopt=全部
    std::vector<repository::BlacklistEntry> search_blacklist_by_type(
        ban::BanType type, const std::optional<std::string>& from,
        const std::optional<std::string>& to, std::optional<ban::BanStatus> status, int page,
        int page_size) const;

    // ---------- 用户状态管理 ----------

    /// 查询用户当前状态：任一维度 Active 则 Active，否则看是否有 Expired
    std::optional<ban::BanStatus> get_user_status(uint64_t account_id) const;

    /// 对于过期自动解封的处理
    /// 不设定时任务，靠查询时兜底——但"兜底"必须是"查询时不依赖 status 字段"
    /// WHERE account_id = ? AND ban_type = ?
    /// AND status = 1 AND expire_at > NOW()
    /// 周期线程做全局兜底清理

private:
    repository::BlacklistRepository& m_blacklist_repo;
};
}  // namespace admin::service