#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "common/ban_enums.h"
#include "connection_provider.h"

namespace repository
{

const std::string PERMANENT_EXPIRE = "9999-12-30 23:59:59";
const std::string NOT_UNBANNED     = "1970-01-01 00:00:00";

struct BlacklistEntry
{
    uint64_t        id         = 0;
    uint64_t        account_id = 0;
    std::string     reason;  // reason default ''
    ban::ReasonCode reason_code = ban::ReasonCode::Unspecified;
    ban::BanType    ban_type    = ban::BanType::All;
    std::string     banned_by;     // default auto = 系统自动
    std::string     banned_at;     // "YYYY-MM-DD HH:MM:SS"
    std::string     expire_at;     // default 9999-12-30=永久
    std::string     unbanned_at;   // default 1970-01-01=未解封
    std::string     unbanned_by;   // default ''
    std::string     unban_reason;  // default ''
    ban::BanStatus  status = ban::BanStatus::Active;
    ban::BanSource  source = ban::BanSource::Manual;
    uint64_t        ref_id = 0;  // 0 = 无关联
};

/// 搜索过滤条件（各字段为 nullopt 表示不过滤）
struct BlacklistFilter
{
    std::optional<ban::ReasonCode> reason;
    std::optional<ban::BanType>    type;
    std::optional<std::string>     from;  // "YYYY-MM-DD HH:MM:SS"
    std::optional<std::string>     to;
    std::optional<ban::BanStatus>  status;
    std::optional<uint64_t>        account_id;
};

struct PageResult
{
    std::vector<BlacklistEntry> item;
    uint64_t                    total;
};

/*
封禁思路：在鉴权中间件中检测ALL,能立即响应全局的封禁（在下一次请求前）
具体的 banType 在执行具体业务前检测
这样就能避免了 all和单一类型的联合判断
*/

class BlacklistRepository
{
public:
    BlacklistRepository(IDbConnectionProvider& db_provider, std::string blacklist_table);

    // ---------- 写入 ----------

    /// 插入一条封禁记录
    /// @throws common::AppException 数据库不可用
    bool insert(const BlacklistEntry& data);

    /// 幂等检查：某账号在某维度是否存在生效中记录
    ///  @note 仅按维度匹配，不包含 ALL 覆盖的情况
    /// 当仅存在 ALL 时，其他维度仍返回 false
    bool exists_active(uint64_t account_id, ban::BanType type);

    /// 解除封禁（按维度；type=All 表示解除该账号全部生效记录）
    /// 会写入 unbanned_at=NOW()、unbanned_by、unban_reason、status=0
    /// @return 实际更新的行数
    int unban(uint64_t account_id, ban::BanType type, const std::string& operator_id,
              const std::string& unban_reason);

    /// 全局兜底：把已过期但仍为 status=1 的记录批量置为 status=2（已过期）
    /// @return 受影响行数
    int expire_outdated();

    // ---------- 查询（生效判定统一：status=1 AND expire_at 未到） ----------

    /// 任一维度生效即返回 true
    bool is_blacklisted(uint64_t account_id);

    /// 某维度生效详情
    std::optional<BlacklistEntry> get_detail(uint64_t account_id, ban::BanType type);

    /// 条件查询（分页），按 banned_at DESC
    PageResult search(const BlacklistFilter& filter, int page, int page_size);

private:
    IDbConnectionProvider& m_db_provider;
    std::string            m_blacklist_table;
};

}  // namespace repository
