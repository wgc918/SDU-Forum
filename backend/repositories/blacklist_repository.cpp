#include "repositories/blacklist_repository.h"

#include <jdbc/cppconn/exception.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "common/ban_enums.h"
#include "common/errors.h"
#include "database/IResultSet.h"
#include "log.h"

namespace repository
{

namespace
{

/// 将结果集当前行映射为黑名单记录
BlacklistEntry read_entry(dcp::IResultSet& res)
{
    BlacklistEntry entry;
    entry.id           = res.get_uint64("id");
    entry.account_id   = res.get_uint64("account_id");
    entry.reason       = res.get_string("reason");
    entry.reason_code  = static_cast<ban::ReasonCode>(res.get_uint("reason_code"));
    entry.ban_type     = static_cast<ban::BanType>(res.get_uint("ban_type"));
    entry.banned_by    = res.get_string("banned_by");
    entry.banned_at    = res.get_string("banned_at");
    entry.expire_at    = res.get_string("expire_at");
    entry.unbanned_at  = res.get_string("unbanned_at");
    entry.unbanned_by  = res.get_string("unbanned_by");
    entry.unban_reason = res.get_string("unban_reason");
    entry.status       = static_cast<ban::BanStatus>(res.get_int("status"));
    entry.source       = ban::string_to_ban_source(res.get_string("source"));
    entry.ref_id       = res.get_uint64("ref_id");
    return entry;
}

}  // namespace

BlacklistRepository::BlacklistRepository(IDbConnectionProvider& db_provider,
                                         std::string            blacklist_table)
    : m_db_provider(db_provider), m_blacklist_table(std::move(blacklist_table))
{
}

/// 插入一条封禁记录
/// @throws common::AppException 数据库不可用
bool BlacklistRepository::insert(const BlacklistEntry& data)
{
    /// 单一维度幂等：应用层保证不会有两个管理员同时调用该接口，所以在插入前判重即可
    // if (exists_active(data.account_id, data.ban_type))  应由业务层判断
    //     return false;

    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        conn->prepare_statement("INSERT INTO " + m_blacklist_table +
                                " (account_id, reason, reason_code, ban_type, banned_by,"
                                " expire_at, `status`, source, ref_id) VALUES (?,?,?,?,?,?,?,?,?)");
        conn->set_uint64(1, data.account_id);
        conn->set_string(2, data.reason);
        conn->set_int(3, static_cast<uint64_t>(data.reason_code));
        conn->set_int(4, static_cast<uint8_t>(data.ban_type));
        conn->set_string(5, data.banned_by);
        conn->set_datetime(6, data.expire_at);
        conn->set_int(7, static_cast<int>(data.status));
        conn->set_string(8, ban::ban_source_to_string(data.source));
        conn->set_uint64(9, data.ref_id);

        return conn->execute_update() > 0;
    }
    catch (const sql::SQLException& e)
    {
        LOGW("insert blacklist failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database insert failed");
    }
}

/// 幂等检查：某账号在某维度是否存在生效中记录
/// 生效判定：status=1 AND expire_at > NOW()
bool BlacklistRepository::exists_active(uint64_t account_id, ban::BanType type)
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");
    try
    {
        conn->prepare_statement("SELECT id FROM " + m_blacklist_table +
                                " WHERE account_id=? AND status=? AND expire_at > NOW()"
                                " AND ban_type=?");
        conn->set_uint64(1, account_id);
        conn->set_int(2, static_cast<uint8_t>(ban::BanStatus::Active));
        conn->set_int(3, static_cast<uint8_t>(type));
        auto res = conn->execute_query();
        if (!res)
            throw common::AppException(common::ErrorCode::DatabaseError, "database no result");

        return res->next();
    }
    catch (const sql::SQLException& e)
    {
        LOGW("query blacklist failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database select failed");
    }
}

/// 解除封禁（按维度；type=All 表示解除该账号全部生效记录）
/// 会写入 unbanned_at=NOW()、unbanned_by、unban_reason、status=0
/// @return 实际更新的行数
int BlacklistRepository::unban(uint64_t account_id, ban::BanType type,
                               const std::string& operator_id, const std::string& unban_reason)
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        if (type == ban::BanType::All)
        {
            conn->prepare_statement(
                "UPDATE " + m_blacklist_table +
                " SET unbanned_at=NOW(), unbanned_by=?, unban_reason=?, `status`=?"
                " WHERE account_id=? AND status=? AND expire_at > NOW()");
            conn->set_string(1, operator_id);
            conn->set_string(2, unban_reason);
            conn->set_int(3, static_cast<uint8_t>(ban::BanStatus::Revoked));
            conn->set_uint64(4, account_id);
            conn->set_int(5, static_cast<uint8_t>(ban::BanStatus::Active));
            return conn->execute_update();
        }
        else
        {
            conn->prepare_statement(
                "UPDATE " + m_blacklist_table +
                " SET unbanned_at=NOW(), unbanned_by=?, unban_reason=?, `status`=?"
                " WHERE account_id=? AND ban_type=? AND status=? AND expire_at > NOW()");
            conn->set_string(1, operator_id);
            conn->set_string(2, unban_reason);
            conn->set_int(3, static_cast<uint8_t>(ban::BanStatus::Revoked));
            conn->set_uint64(4, account_id);
            conn->set_int(5, static_cast<uint8_t>(type));
            conn->set_int(6, static_cast<uint8_t>(ban::BanStatus::Active));
            return conn->execute_update();
        }
    }
    catch (const sql::SQLException& e)
    {
        LOGW("update blacklist failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database update failed");
    }
}

/// 全局兜底：把已过期但仍为 status=1 的记录批量置为 status=2（已过期）
/// @return 受影响行数
int BlacklistRepository::expire_outdated()
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");

    try
    {
        conn->prepare_statement("UPDATE " + m_blacklist_table +
                                " SET unbanned_at=expire_at, `status`=?,"
                                " unbanned_by=?, unban_reason=? WHERE `status`=? AND expire_at"
                                " < NOW()");
        conn->set_int(1, static_cast<uint8_t>(ban::BanStatus::Expired));
        conn->set_string(2, "system");
        conn->set_string(3, "到期自动解除");
        conn->set_int(4, static_cast<uint8_t>(ban::BanStatus::Active));
        int rv = conn->execute_update();
        LOGI("expire_outdated update rows=%d", rv);
        return rv;
    }
    catch (const sql::SQLException& e)
    {
        LOGW("update blacklist failed:%s", e.what());
        return 0;
    }
}

// ---------- 查询（生效判定统一：status=1 AND expire_at 未到） ----------

/// 任一维度生效即返回 true
bool BlacklistRepository::is_blacklisted(uint64_t account_id)
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");
    try
    {
        conn->prepare_statement("SELECT id FROM " + m_blacklist_table +
                                " WHERE account_id=? AND status=? AND"
                                " expire_at > NOW()");
        conn->set_uint64(1, account_id);
        conn->set_int(2, static_cast<uint8_t>(ban::BanStatus::Active));
        auto res = conn->execute_query();
        if (!res)
            throw common::AppException(common::ErrorCode::DatabaseError, "database no result");

        return res->next();
    }
    catch (const sql::SQLException& e)
    {
        LOGW("query blacklist failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database select failed");
    }
}

/// 某维度生效详情
std::optional<BlacklistEntry> BlacklistRepository::get_detail(uint64_t     account_id,
                                                              ban::BanType type)
{
    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");
    try
    {
        conn->prepare_statement("SELECT * FROM " + m_blacklist_table +
                                " WHERE account_id=? AND status=? AND expire_at > NOW()"
                                " AND ban_type=?");
        conn->set_uint64(1, account_id);
        conn->set_int(2, static_cast<uint8_t>(ban::BanStatus::Active));
        conn->set_int(3, static_cast<uint8_t>(type));
        auto res = conn->execute_query();
        if (!res)
            throw common::AppException(common::ErrorCode::DatabaseError, "database no result");

        if (!res->next())
            return std::nullopt;

        return read_entry(*res);
    }
    catch (const sql::SQLException& e)
    {
        LOGW("query blacklist failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database select failed");
    }
}

/// 条件查询（分页），按 banned_at DESC
PageResult BlacklistRepository::search(const BlacklistFilter& filter, int page, int page_size)
{
    if (page < 1 || page_size < 1)
        return {};

    auto conn = m_db_provider.acquire();
    if (!conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");
    PageResult result;
    try
    {
        std::vector<std::string> conditions;
        if (filter.reason)
            conditions.emplace_back("reason_code=?");
        if (filter.type)
            conditions.emplace_back("ban_type=?");
        if (filter.from)
            conditions.emplace_back("banned_at>=?");
        if (filter.to)
            conditions.emplace_back("banned_at<=?");

        if (filter.status)
        {
            if (*filter.status == ban::BanStatus::Active)
                conditions.emplace_back("`status`=1 AND expire_at > NOW()");
            else if (*filter.status == ban::BanStatus::Expired)
                conditions.emplace_back(
                    "(`status`=2 OR (`status`=1 AND"
                    " expire_at <= NOW()))");
            else
                conditions.emplace_back("`status`=0");
        }

        if (filter.account_id)
            conditions.emplace_back("account_id =?");

        std::string sql       = "SELECT * FROM " + m_blacklist_table;
        std::string sql_count = "SELECT COUNT(*) FROM " + m_blacklist_table;
        if (!conditions.empty())
        {
            sql += " WHERE ";
            sql_count += " WHERE ";
            for (std::size_t i = 0; i < conditions.size(); i++)
            {
                if (i > 0)
                    sql += " AND ", sql_count += " AND ";
                sql += conditions[i];
                sql_count += conditions[i];
            }
        }
        sql += " ORDER BY banned_at DESC LIMIT ? OFFSET ?";

        conn->prepare_statement(sql_count);
        uint16_t index = 1;
        if (filter.reason)
            conn->set_int(index++, static_cast<int>(*filter.reason));
        if (filter.type)
            conn->set_int(index++, static_cast<int>(*filter.type));
        if (filter.from)
            conn->set_datetime(index++, *filter.from);
        if (filter.to)
            conn->set_datetime(index++, *filter.to);
        if (filter.account_id)
            conn->set_uint64(index++, *filter.account_id);

        auto res = conn->execute_query();
        if (!res)
            throw common::AppException(common::ErrorCode::DatabaseError, "database no result");

        if (!res->next())
            result.total = 0;
        else
            result.total = res->get_uint64(1);

        conn->prepare_statement(sql);

        index = 1;
        if (filter.reason)
            conn->set_int(index++, static_cast<int>(*filter.reason));
        if (filter.type)
            conn->set_int(index++, static_cast<int>(*filter.type));
        if (filter.from)
            conn->set_datetime(index++, *filter.from);
        if (filter.to)
            conn->set_datetime(index++, *filter.to);
        if (filter.account_id)
            conn->set_uint64(index++, *filter.account_id);
        conn->set_int(index++, page_size);
        conn->set_int(index++, (page - 1) * page_size);

        res = conn->execute_query();
        if (!res)
            throw common::AppException(common::ErrorCode::DatabaseError, "database no result");

        std::vector<BlacklistEntry> entries;
        while (res->next())
            entries.push_back(read_entry(*res));

        result.item = std::move(entries);
        return result;
    }
    catch (const sql::SQLException& e)
    {
        LOGW("query blacklist failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database select failed");
    }
}

}  // namespace repository
