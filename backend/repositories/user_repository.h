#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace dcp
{
class ConnectionPool;
}

namespace repository
{

struct AccountRecord
{
    int64_t     id;
    std::string user_id;
    std::string password_hash;
    std::string salt;
    int         iterations = 0;
};

/// account 表只读访问：登录流程只需要凭据字段
class UserRepository
{
public:
    UserRepository(dcp::ConnectionPool& pool, std::string account_table);

    /// @throws common::AppException 数据库不可用
    std::optional<AccountRecord> find_account_by_user_id(const std::string& user_id);

private:
    dcp::ConnectionPool& m_pool;
    std::string          m_account_table;
};

}  // namespace repository
