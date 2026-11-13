#pragma once

#include "conn-pool/ConnectionPool.h"
#include "redis-pool/RedisConnectionPool.h"

namespace repository
{
// 负责存储未过期的token,用于吊销
// refresh_token 持久化到数据库以便审计
// access_token 定时存到redis即可
struct TokenRecord
{
    std::string token_hash;
    uint64_t    exp_at;
};

class TokenRepository
{
public:
    TokenRepository(dcp::ConnectionPool& db_pool, rcp::RedisConnectionPool& redis_pool,
                    const std::string& refresh_token_table, const std::string& refresh_token_key,
                    const std::string& access_token_key, int refresh_ttl_s, int access_ttl_s);
    /// @throws common::AppException 数据库不可用
    bool find_refresh_token(const std::string& token_hash);

    /// @throws common::AppException 数据库不可用
    bool insert_refresh_token(const TokenRecord& token);

    /// @throws common::AppException 数据库不可用
    bool find_access_token(const std::string& token_hash);

    /// @throws common::AppException 数据库不可用
    bool insert_access_token(const TokenRecord& token);

private:
    dcp::ConnectionPool&      m_db_pool;
    rcp::RedisConnectionPool& m_redis_pool;
    std::string               m_refresh_token_table;
    std::string               m_refresh_token_key;
    std::string               m_access_token_key;
    int                       m_refresh_ttl_s;
    int                       m_access_ttl_s;
};
}  // namespace repository