//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 SDU-Forum 项目
//
// 文件: connection_provider.cpp
// 作者: wgc
// 创建日期: 2026年11月
// 最后修改: 2026年11月
//
// 描述:
//    实现数据库与缓存连接抽象的生产适配器。
//
// 实现说明:
//     - PooledDbConnection 持有 dcp::PooledConnGuard，析构时自动归还连接
//     - PooledCacheConnection 持有 rcp::RedisConnGuard，析构时自动归还连接
//     - 连接池返回空守卫时 acquire() 返回 nullptr
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人以处理软件的权利，
//     包括但不限于使用、复制、修改、合并、出版、分发、再许可和/或出售软件副本，
//     以及允许软件适用者这样做，须在下列条件下：
//
//     上述版权声明和本许可声明应包含在软件的所有副本或实质性部分中。
//
//     软件按"原样"提供，不提供任何形式的明示或暗示的保证，
//     包括但不限于对适销性、特定用途适用性和非侵权性的保证。
//     在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
//     无论是在合同诉讼、侵权诉讼或其他诉讼中，
//     由于软件或软件的使用或其他交易产生的。
//-----------------------------------------------------------------------------
#include "repositories/connection_provider.h"

#include <chrono>
#include <cstdint>
#include <utility>

#include "conn-pool/ConnectionPool.h"
#include "conn-pool/pooledConnGuard.h"
#include "database/IResultSet.h"
#include "redis-pool/RedisConnGuard.h"
#include "redis-pool/RedisConnectionPool.h"

namespace repository
{
namespace
{

/// @brief 基于连接守卫的数据库连接实现
class PooledDbConnection final : public IDbConnection
{
public:
    /// @brief 接管一个已借出的数据库连接
    /// @param guard 连接守卫
    explicit PooledDbConnection(dcp::PooledConnGuard guard) : m_guard(std::move(guard))
    {
    }

    void prepare_statement(const std::string& sql) override
    {
        m_guard->prepare_statement(sql);
    }

    void set_string(uint16_t index, const std::string& value) override
    {
        m_guard->set_string(index, value);
    }

    void set_int(uint16_t index, int value) override
    {
        m_guard->set_int(index, value);
    }

    void set_uint64(uint16_t index, uint64_t value) override
    {
        m_guard->set_uint64(index, value);
    }

    void set_datetime(uint16_t index, const std::string& value) override
    {
        m_guard->set_datetime(index, value);
    }

    std::unique_ptr<dcp::IResultSet> execute_query() override
    {
        return std::unique_ptr<dcp::IResultSet>(m_guard->execute_query());
    }

    int execute_update() override
    {
        return m_guard->execute_update();
    }

private:
    dcp::PooledConnGuard m_guard;  ///< 数据库连接守卫
};

/// @brief 基于连接守卫的缓存连接实现
class PooledCacheConnection final : public ICacheConnection
{
public:
    /// @brief 接管一个已借出的缓存连接
    /// @param guard 连接守卫
    explicit PooledCacheConnection(rcp::RedisConnGuard guard) : m_guard(std::move(guard))
    {
    }

    std::optional<std::string> get(const std::string& key) override
    {
        auto value = m_guard->get(key);
        if (!value)
            return std::nullopt;
        return std::string(*value);
    }

    bool set(const std::string& key, const std::string& value, int ttl_seconds) override
    {
        return m_guard->set(key, value,
                            std::chrono::milliseconds(static_cast<long long>(ttl_seconds) * 1000));
    }

private:
    rcp::RedisConnGuard m_guard;  ///< 缓存连接守卫
};

}  // namespace

PooledDbConnectionProvider::PooledDbConnectionProvider(dcp::ConnectionPool& pool) : m_pool(pool)
{
}

std::unique_ptr<IDbConnection> PooledDbConnectionProvider::acquire()
{
    auto guard = m_pool.get_connection();
    if (!guard)
        return nullptr;
    return std::make_unique<PooledDbConnection>(std::move(guard));
}

PooledCacheConnectionProvider::PooledCacheConnectionProvider(rcp::RedisConnectionPool& pool)
    : m_pool(pool)
{
}

std::unique_ptr<ICacheConnection> PooledCacheConnectionProvider::acquire()
{
    auto guard = m_pool.get_connection();
    if (!guard)
        return nullptr;
    return std::make_unique<PooledCacheConnection>(std::move(guard));
}

}  // namespace repository
