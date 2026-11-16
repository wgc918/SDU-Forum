#include "token_repository.h"

#include <jdbc/cppconn/exception.h>

#include <exception>
#include <optional>

#include "common/errors.h"
#include "conn-pool/pooledConnGuard.h"
#include "log.h"
#include "redis-pool/RedisConnGuard.h"
#include "utils/utils.h"

namespace repository
{
TokenRepository::TokenRepository(IDbConnectionProvider&    db_provider,
                                 ICacheConnectionProvider& cache_provider,
                                 const TokenStoreConfig&   config)
    : m_db_provider(db_provider),
      m_cache_provider(cache_provider),
      m_refresh_token_table(config.refresh_token_table),
      m_hash_secret(config.hash_secret),
      m_refresh_ttl_s(config.refresh_ttl_s),
      m_access_ttl_s(config.access_ttl_s)
{
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::find_refresh_token(const std::string& token_hash)
{
    // 直接把token_hash当作key来存到redis中
    auto redis_conn = m_cache_provider.acquire();
    if (!redis_conn)
        throw common::AppException(common::ErrorCode::RedisError, "redis connection unavailable");
    std::optional<std::string> res;
    try
    {
        res = redis_conn->get(token_hash);
    }
    catch (const std::exception& e)
    {
        LOGW("get refresh_token cache failed:%s", e.what());
        throw common::AppException(common::ErrorCode::RedisError, "redis query failed");
    }

    // 缓存未命中
    if (!res)
    {
        auto db_conn = m_db_provider.acquire();
        if (!db_conn)
            throw common::AppException(common::ErrorCode::DatabaseError,
                                       "database connection unavailable");
        try
        {
            db_conn->prepare_statement("SELECT id FROM " + m_refresh_token_table +
                                       " WHERE token_hash=?");
            db_conn->set_string(1, token_hash);
            auto db_res = db_conn->execute_query();
            if (!db_res)
                throw common::AppException(common::ErrorCode::DatabaseError,
                                           "database query returned no result set");
            if (!db_res->next())
                return false;
        }
        catch (const sql::SQLException& e)
        {
            LOGW("query refresh_token failed:%s", e.what());
            throw common::AppException(common::ErrorCode::DatabaseError, "database query failed");
        }

        // 缓存回写是 best-effort：写入失败不影响数据库命中结果
        try
        {
            redis_conn->set(token_hash, token_hash, m_refresh_ttl_s);
        }
        catch (const std::exception& e)
        {
            LOGW("set refresh_token cache failed:%s", e.what());
            throw common::AppException(common::ErrorCode::RedisError, "redis write failed");
        }
        return true;
    }

    return true;
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::insert_refresh_token(const TokenRecord& token)
{
    auto db_conn = m_db_provider.acquire();
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
    auto redis_conn = m_cache_provider.acquire();
    if (!redis_conn)
        throw common::AppException(common::ErrorCode::RedisError, "redis connection unavailable");
    std::optional<std::string> res;
    try
    {
        res = redis_conn->get(token_hash);
    }
    catch (const std::exception& e)
    {
        LOGW("get access_token cache failed:%s", e.what());
        throw common::AppException(common::ErrorCode::RedisError, "redis query failed");
    }
    return res.has_value();
}

/// @throws common::AppException 数据库不可用
bool TokenRepository::insert_access_token(const TokenRecord& token)
{
    auto redis_conn = m_cache_provider.acquire();
    if (!redis_conn)
        throw common::AppException(common::ErrorCode::RedisError, "redis connection unavailable");
    try
    {
        return redis_conn->set(token.token_hash, token.token_hash, m_access_ttl_s);
    }
    catch (const std::exception& e)
    {
        LOGW("set access_token cache failed:%s", e.what());
        throw common::AppException(common::ErrorCode::RedisError, "redis write failed");
    }
}

const std::string& TokenRepository::hash_secret() const noexcept
{
    return m_hash_secret;
}

}  // namespace repository