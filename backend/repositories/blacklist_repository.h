#pragma once

#include <optional>
#include <string>

namespace dcp
{
class ConnectionPool;
}

namespace repository
{

struct BlacklistEntry
{
    std::string user_id;
    std::string reason;
    std::string end_time;
};

/// account_blacklist 表只读访问：只返回当前仍生效的封禁记录
class BlacklistRepository
{
public:
    BlacklistRepository(dcp::ConnectionPool& pool, std::string blacklist_table);

    /// @throws common::AppException 数据库不可用
    std::optional<BlacklistEntry> find_active_by_user_id(const std::string& user_id);

private:
    dcp::ConnectionPool& m_pool;
    std::string          m_blacklist_table;
};

}  // namespace repository
