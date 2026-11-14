#include "token_repository.h"

#include <jdbc/cppconn/exception.h>

#include <optional>

#include "common/errors.h"
#include "conn-pool/pooledConnGuard.h"
#include "log.h"
#include "redis-pool/RedisConnGuard.h"
#include "utils/utils.h"

namespace repository
{
TokenRepository::TokenRepository(dcp::ConnectionPool& db_pool, rcp::RedisConnectionPool& redis_pool,
                                 const std::string& refresh_token_table, int refresh_ttl_s,
                                 int access_ttl_s)
    : m_db_pool(db_pool),
      m_redis_pool(redis_pool),
      m_refresh_token_table(refresh_token_table),
      m_refresh_ttl_s(refresh_ttl_s),
      m_access_ttl_s(access_ttl_s)

{
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::find_refresh_token(const std::string& token_hash)
{
    // 直接把token_hash当作key来存到redis中
    auto redis_conn = m_redis_pool.get_connection();
    if (!redis_conn)
        throw common::AppException(common::ErrorCode::RedisError, "redis connection unavailable");
    auto res = redis_conn->get(token_hash);
    // 缓存未命中
    if (!res)
    {
        auto db_conn = m_db_pool.get_connection();
        if (!db_conn)
            throw common::AppException(common::ErrorCode::DatabaseError,
                                       "database connection unavailable");
        try
        {
            db_conn->prepare_statement("SELECT id FROM " + m_refresh_token_table +
                                       " WHERE token_hash=?");
            db_conn->set_string(1, token_hash);
            auto db_res = db_conn->execute_query();
            if (!db_res->next())
                return false;
            else  // 写缓存
                redis_conn->set(token_hash, token_hash, m_refresh_ttl_s);
            return true;
        }
        catch (const sql::SQLException& e)
        {
            LOGW("query refresh_token failed:%s", e.what());
            throw common::AppException(common::ErrorCode::DatabaseError, "database query failed");
        }
    }

    return true;
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::insert_refresh_token(const TokenRecord& token)
{
    auto db_conn = m_db_pool.get_connection();
    if (!db_conn)
        throw common::AppException(common::ErrorCode::DatabaseError,
                                   "database connection unavailable");
    try
    {
        db_conn->prepare_statement("INSERT INTO " + m_refresh_token_table +
                                   " (token_hash,expires_at) VALUES (?,?)");
        db_conn->set_string(1, token.token_hash);
        db_conn->set_datetime(2, utils::timestamp_to_string(token.exp_at));
        int rows = db_conn->execute_update();
        return rows > 0 ? true : false;
    }
    catch (const sql::SQLException& e)
    {
        LOGW("insert refresh_token failed:%s", e.what());
        throw common::AppException(common::ErrorCode::DatabaseError, "database insert failed");
    }
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::find_access_token(const std::string& token_hash)
{
    auto redis_conn = m_redis_pool.get_connection();
    if (!redis_conn)
        throw common::AppException(common::ErrorCode::RedisError, "redis connection unavailable");
    auto res = redis_conn->get(token_hash);
    return res.has_value();
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::insert_access_token(const TokenRecord& token)
{
    auto redis_conn = m_redis_pool.get_connection();
    if (!redis_conn)
        throw common::AppException(common::ErrorCode::RedisError, "redis connection unavailable");
    return redis_conn->set(token.token_hash, token.token_hash, m_access_ttl_s);
}
}  // namespace repository