#include "repositories/account_repository.h"

#include <jdbc/cppconn/exception.h>

#include <cstdlib>
#include <memory>
#include <utility>

#include "common/errors.h"
#include "conn-pool/ConnectionPool.h"
#include "conn-pool/pooledConnGuard.h"
#include "log.h"

namespace repository
{

AccountRepository::AccountRepository(dcp::ConnectionPool& pool, std::string account_table)
    : m_pool(pool), m_account_table(std::move(account_table))
{
}

std::optional<AccountRecord> AccountRepository::find_account_by_user_id(const std::string& user_id)
{
    auto conn = m_pool.get_connection();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        conn->prepare_statement("SELECT id, public_id, password_hash, salt, iterations FROM " +
                                m_account_table + " WHERE user_id = ?");
        conn->set_string(1, user_id);

        std::unique_ptr<dcp::IResultSet> result(conn->execute_query());
        if (!result->next())
            return std::nullopt;

        AccountRecord record;
        record.id            = result->get_uint64("id");
        record.public_id     = result->get_string("public_id");
        record.password_hash = result->get_string("password_hash");
        record.salt          = result->get_string("salt");
        record.iterations    = result->get_int("iterations");
        return record;
    }
    catch (const sql::SQLException& e)
    {
        LOGE("query account failed: %s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database query failed");
    }
}

bool AccountRepository::insert_account_by_user_id(const std::string&   user_id,
                                               const AccountRecord& account)
{
    auto conn = m_pool.get_connection();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");
    try
    {
        // 查重逻辑交由业务层负责

        conn->prepare_statement("INSERT INTO " + m_account_table +
                                " (public_id,user_id,password_hash,salt,iterations) "
                                "VALUES(?,?,?,?,?)");
        conn->set_string(1, account.public_id);
        conn->set_string(2, user_id);
        conn->set_string(3, account.password_hash);
        conn->set_string(4, account.salt);
        conn->set_int(5, account.iterations);

        int rt = conn->execute_update();
        if (rt == 0)
            return false;
        return true;
    }
    catch (const sql::SQLException& e)
    {
        LOGE("update account failed: %s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database update failed");
    }
}

}  // namespace repository
