#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "connection_provider.h"

namespace repository
{

struct AdminAccountRecord
{
    uint64_t    id;
    std::string admin_id;
    std::string password_hash;
    std::string salt;
    int         iterations;
};

class AdminAccountRepository
{
public:
    AdminAccountRepository(IDbConnectionProvider& db_provider, const std::string& amin_table);

    std::optional<AdminAccountRecord> find_admin_account_by_admin_id(const std::string& admin_id);

    bool inisert_admin_account(const AdminAccountRecord& account_record);

private:
    IDbConnectionProvider& m_db_provider;
    std::string            m_admin_table;
};
}  // namespace repository