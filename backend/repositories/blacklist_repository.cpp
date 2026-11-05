#include "repositories/blacklist_repository.h"

#include <jdbc/cppconn/exception.h>

#include <memory>
#include <utility>

#include "common/errors.h"
#include "conn-pool/ConnectionPool.h"
#include "conn-pool/pooledConnGuard.h"
#include "log.h"

namespace repository
{

BlacklistRepository::BlacklistRepository(dcp::ConnectionPool& pool, std::string blacklist_table)
    : m_pool(pool), m_blacklist_table(std::move(blacklist_table))
{
}

std::optional<BlacklistEntry> BlacklistRepository::find_active_by_user_id(
    const std::string& user_id)
{
    auto conn = m_pool.get_connection();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        conn->prepare_statement("SELECT user_id, reason, end_time FROM " + m_blacklist_table +
                                " WHERE user_id = ? AND (end_time IS NULL OR end_time > NOW())");
        conn->set_string(1, user_id);

        std::unique_ptr<dcp::IResultSet> result(conn->execute_query());
        if (!result->next())
            return std::nullopt;

        BlacklistEntry entry;
        entry.user_id = result->get_string("user_id");
        entry.reason  = result->is_null("reason") ? "" : result->get_string("reason");
        entry.end_time = result->is_null("end_time") ? "" : result->get_string("end_time");
        return entry;
    }
    catch (const sql::SQLException& e)
    {
        LOGE("%s", (std::string("query account_blacklist failed: ") + e.what()).c_str());
        throw common::AppException(common::ErrorCode::DatabaseError, "database query failed");
    }
}

}  // namespace repository
