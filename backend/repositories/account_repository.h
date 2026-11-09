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
    uint64_t    id;
    std::string public_id;
    std::string password_hash;
    std::string salt;
    int         iterations;
};



/// account 表只读访问：登录流程只需要凭据字段
class AccountRepository
{
public:
    AccountRepository(dcp::ConnectionPool& pool, std::string account_table);

    /// @throws common::AppException 数据库不可用
    std::optional<AccountRecord> find_account_by_user_id(const std::string& user_id);

    /// @throws common::AppException 数据库不可用
    bool add_account_by_user_id(const std::string& user_id, const AccountRecord& account);

private:
    dcp::ConnectionPool& m_pool;
    std::string          m_account_table;
};

}  // namespace repository
