#include "admin_account_repository.h"

#include <jdbc/cppconn/exception.h>

#include "common/errors.h"
#include "database/IResultSet.h"
#include "log.h"

namespace repository
{
AdminAccountRepository::AdminAccountRepository(IDbConnectionProvider& db_provider,
                                               const std::string&     admin_table)
    : m_db_provider(db_provider), m_admin_table(admin_table)
{
}

std::optional<AdminAccountRecord> AdminAccountRepository::find_admin_account_by_admin_id(
    const std::string& admin_id)
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        conn->prepare_statement("SELECT id, password_hash, salt, iterations FROM " + m_admin_table +
                                " WHERE admin_id=?");
        conn->set_string(1, admin_id);

        auto res = conn->execute_query();
        if (!res)
            throw common::AppException(common::ErrorCode::DatabaseError,
                                       "database query returned no result set");
        if (!res->next())
            return std::nullopt;

        AdminAccountRecord record;
        record.admin_id      = admin_id;
        record.id            = res->get_uint64("id");
        record.password_hash = res->get_string("password_hash");
        record.salt          = res->get_string("salt");
        record.iterations    = res->get_int("iterations");
        return record;
    }
    catch (const sql::SQLException& e)
    {
        LOGW("query admin account failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database select failed");
    }
}

bool AdminAccountRepository::inisert_admin_account(const AdminAccountRecord& account_record)
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        conn->prepare_statement("INSERT INTO " + m_admin_table +
                                " (admin_id,password_hash,salt) VALUES(?,?,?)");
        conn->set_string(1, account_record.admin_id);
        conn->set_string(2, account_record.password_hash);
        conn->set_string(3, account_record.salt);
        int rv = conn->execute_update();
        return rv > 0;
    }
    catch (const sql::SQLException& e)
    {
        LOGW("insert admin account failed: %s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database insert failed");
    }
}
}  // namespace repository