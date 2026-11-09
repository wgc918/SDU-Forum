#include "repositories/account_repository.h"

#include <jdbc/cppconn/exception.h>

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
        conn->prepare_statement("SELECT id, user_id, password_hash, salt, iterations FROM " +
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

}  // namespace repository
