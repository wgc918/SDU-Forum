#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "repositories/connection_provider.h"

namespace repository
{

struct AccountRecord
{
    uint64_t    id;
    std::string public_id;
    std::string user_id;
    std::string password_hash;
    std::string salt;
    int         iterations;
};

/// account 表只读访问：登录流程只需要凭据字段
class AccountRepository
{
public:
    AccountRepository(IDbConnectionProvider& db_provider, std::string account_table);

    /// @throws common::AppException 数据库不可用
    std::optional<AccountRecord> find_account_by_user_id(const std::string& user_id);

    std::optional<AccountRecord> find_account_by_public_id(const std::string& public_id);

    /// @throws common::AppException 数据库不可用
    bool insert_account_by_user_id(const std::string& user_id, const AccountRecord& account);

    /// @throws common::AppException 数据库不可用
    bool delete_account_by_public_id(const std::string& public_id);

private:
    IDbConnectionProvider& m_db_provider;
    std::string            m_account_table;
};

}  // namespace repository
