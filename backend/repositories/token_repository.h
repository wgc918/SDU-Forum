#pragma once

#include "conn-pool/ConnectionPool.h"
#include "redis-pool/RedisConnectionPool.h"

namespace repository
{
// 负责存储未过期的token,用于吊销
// refresh_token 持久化到数据库以便审计
// access_token 定时存到redis即可
class TokenRepository
{
    struct RefreshTokenRecord
    {
        std::string token_hash;
        uint64_t    exp_at;
    };

public:
    TokenRepository();
    /// @throws common::AppException 数据库不可用
    std::optional<RefreshTokenRecord> find_refresh_token(const std::string& token_hash);

    /// @throws common::AppException 数据库不可用
    bool insert_refresh_token(const RefreshTokenRecord& account);

    /// @throws common::AppException 数据库不可用
    std::string find_access_token(const std::string& token_hash);

    /// @throws common::AppException 数据库不可用
    bool insert_access_token(const std::string& token_hash);

private:
    dcp::ConnectionPool&      m_db_pool;
    rcp::RedisConnectionPool& m_redis_pool;
    std::string               m_refresh_token_table;
    std::string               m_refresh_token_key;
    std::string               m_access_token_key;
};
}  // namespace repository