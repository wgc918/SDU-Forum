#pragma once

#include <string>

#include "repositories/connection_provider.h"

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

struct TokenStoreConfig
{
    std::string refresh_token_table;
    std::string hash_secret;
    int         refresh_ttl_s = 0;
    int         access_ttl_s  = 0;
};

/// @brief token 持久化与缓存仓库
/// @details
///     refresh_token 写入数据库以便审计，access_token 写入 Redis。
///     通过连接提供者接口获取连接，便于替换为测试替身。
class TokenRepository
{
public:
    /// @brief 构造 token 仓库
    /// @param db_provider 数据库连接提供者
    /// @param cache_provider 缓存连接提供者
    /// @param config token 存储配置
    TokenRepository(IDbConnectionProvider& db_provider,
                    ICacheConnectionProvider& cache_provider,
                    const TokenStoreConfig& config);
    /// @throws common::AppException 数据库不可用
    bool find_refresh_token(const std::string& token_hash);

    /// @throws common::AppException 数据库不可用
    bool insert_refresh_token(const TokenRecord& token);

    /// @throws common::AppException 数据库不可用
    bool find_access_token(const std::string& token_hash);

    /// @throws common::AppException 数据库不可用
    bool insert_access_token(const TokenRecord& token);

    const std::string& hash_secret() const noexcept;

private:
    IDbConnectionProvider&    m_db_provider;     ///< 数据库连接提供者
    ICacheConnectionProvider& m_cache_provider;  ///< 缓存连接提供者
    std::string               m_refresh_token_table;
    std::string               m_hash_secret;
    int                       m_refresh_ttl_s;
    int                       m_access_ttl_s;
};
}  // namespace repository