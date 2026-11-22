//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 SDU-Forum 项目
//
// 文件: connection_provider.h
// 作者: wgc
// 创建日期: 2026年11月
// 最后修改: 2026年11月
//
// 描述:
//    定义仓库层所需的最小数据库与缓存连接抽象，以及基于连接池的生产实现。
//
// 功能特性:
//     - 以纯虚接口隔离 dcp::ConnectionPool 与 rcp::RedisConnectionPool
//     - 生产适配器内部持有 RAII 守卫，析构时自动归还连接
//     - 连接不可用时返回空指针，由仓库层转换为业务异常
//
// 使用场景:
//     - 生产环境由 AuthModule 装配 Pooled*Provider
//     - 单元测试中注入 Fake / Stub / Spy / Mock 替身
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
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace dcp
{
class ConnectionPool;
class IResultSet;
}  // namespace dcp

namespace rcp
{
class RedisConnectionPool;
}  // namespace rcp

namespace repository
{

/// @brief 仓库层所需的最小数据库连接能力
/// @details
///     仅暴露 Repository 实际使用的方法，避免仓库直接依赖具体连接池。
class IDbConnection
{
public:
    virtual ~IDbConnection() = default;

    /// @brief 预处理 SQL 语句
    /// @param sql 待执行的 SQL
    virtual void prepare_statement(const std::string& sql) = 0;

    /// @brief 绑定字符串参数
    /// @param index 参数下标，从 1 开始
    /// @param value 参数值
    virtual void set_string(uint16_t index, const std::string& value) = 0;

    /// @brief 绑定整型参数
    /// @param index 参数下标，从 1 开始
    /// @param value 参数值
    virtual void set_int(uint16_t index, int value) = 0;

    virtual void set_uint64(uint16_t index, uint64_t value) = 0;

    /// @brief 绑定日期时间参数
    /// @param index 参数下标，从 1 开始
    /// @param value 格式为 YYYY-MM-DD HH:MM:SS 的时间字符串
    virtual void set_datetime(uint16_t index, const std::string& value) = 0;

    /// @brief 执行预处理查询
    /// @return 结果集，调用方持有所有权
    virtual std::unique_ptr<dcp::IResultSet> execute_query() = 0;

    /// @brief 执行预处理更新
    /// @return 受影响行数
    virtual int execute_update() = 0;
};

/// @brief 数据库连接提供者
class IDbConnectionProvider
{
public:
    virtual ~IDbConnectionProvider() = default;

    /// @brief 获取一个数据库连接
    /// @return 连接对象；无可用连接时返回 nullptr
    virtual std::unique_ptr<IDbConnection> acquire() = 0;
};

/// @brief 仓库层所需的最小缓存连接能力
class ICacheConnection
{
public:
    virtual ~ICacheConnection() = default;

    /// @brief 读取缓存值
    /// @param key 缓存键
    /// @return 命中时返回对应值，未命中返回 std::nullopt
    virtual std::optional<std::string> get(const std::string& key) = 0;

    /// @brief 写入缓存值
    /// @param key 缓存键
    /// @param value 缓存值
    /// @param ttl_seconds 过期时间，单位秒
    /// @return 写入是否成功
    virtual bool set(const std::string& key, const std::string& value, int ttl_seconds) = 0;
};

/// @brief 缓存连接提供者
class ICacheConnectionProvider
{
public:
    virtual ~ICacheConnectionProvider() = default;

    /// @brief 获取一个缓存连接
    /// @return 连接对象；无可用连接时返回 nullptr
    virtual std::unique_ptr<ICacheConnection> acquire() = 0;
};

/// @brief 基于 dcp::ConnectionPool 的数据库连接提供者
class PooledDbConnectionProvider final : public IDbConnectionProvider
{
public:
    /// @brief 构造数据库连接提供者
    /// @param pool 数据库连接池，调用方保证其生命周期长于本对象
    explicit PooledDbConnectionProvider(dcp::ConnectionPool& pool);

    std::unique_ptr<IDbConnection> acquire() override;

private:
    dcp::ConnectionPool& m_pool;  ///< 数据库连接池
};

/// @brief 基于 rcp::RedisConnectionPool 的缓存连接提供者
class PooledCacheConnectionProvider final : public ICacheConnectionProvider
{
public:
    /// @brief 构造缓存连接提供者
    /// @param pool Redis 连接池，调用方保证其生命周期长于本对象
    explicit PooledCacheConnectionProvider(rcp::RedisConnectionPool& pool);

    std::unique_ptr<ICacheConnection> acquire() override;

private:
    rcp::RedisConnectionPool& m_pool;  ///< Redis 连接池
};

}  // namespace repository
