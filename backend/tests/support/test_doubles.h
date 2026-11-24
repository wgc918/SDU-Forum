//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 SDU-Forum 项目
//
// 文件: test_doubles.h
// 作者: wgc
// 创建日期: 2026年11月
// 最后修改: 2026年11月
//
// 描述:
//    提供 TokenRepository 单元测试所需的五类测试替身。
//
// 五类替身与断言对象:
//     - Dummy: 仅占位，不参与任何断言
//     - Stub:  提供固定返回值，断言被测对象的返回值
//     - Spy:   记录调用参数与顺序，断言交互是否发生
//     - Mock:  预置期望，verify() 断言交互契约
//     - Fake:  轻量真实实现（内存缓存），断言端到端状态
//
// 实现说明:
//     - 连接替身内部使用 shared_ptr 保存状态，每次 acquire 返回新的连接
//       句柄并共享同一份状态，模拟连接池“每次借出独立连接”的语义
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

#include <jdbc/cppconn/exception.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "database/IResultSet.h"
#include "doctest.h"
#include "repositories/connection_provider.h"

namespace test_support
{

/// @brief Stub 结果集按列名返回的行值配置
struct StubRowValues
{
    std::unordered_map<std::string, uint64_t>    uint64_columns;  ///< 无符号整型列
    std::unordered_map<std::string, int>         int_columns;     ///< 整型列
    std::unordered_map<std::string, std::string> string_columns;  ///< 字符串列
};

/// @brief 返回固定行数的结果集替身
class StubResultSet final : public dcp::IResultSet
{
public:
    /// @brief 构造结果集替身
    /// @param row_count 可读取的行数
    /// @param values 行值配置；为空时所有列返回 0 / 空串
    explicit StubResultSet(int row_count = 0, std::shared_ptr<const StubRowValues> values = nullptr)
        : m_row_count(row_count), m_seen(0), m_values(std::move(values))
    {
    }

    bool next() override
    {
        if (m_seen >= m_row_count)
            return false;
        ++m_seen;
        return true;
    }

    void close() override
    {
    }

    std::size_t get_column_count() override
    {
        return 0;
    }

    int get_int(const std::string& column_name) override
    {
        if (!m_values)
            return 0;
        auto it = m_values->int_columns.find(column_name);
        return it == m_values->int_columns.end() ? 0 : it->second;
    }

    uint32_t get_uint(const std::string&) override
    {
        return 0;
    }

    int64_t get_int64(const std::string&) override
    {
        return 0;
    }

    uint64_t get_uint64(const std::string& column_name) override
    {
        if (!m_values)
            return 0;
        auto it = m_values->uint64_columns.find(column_name);
        return it == m_values->uint64_columns.end() ? 0 : it->second;
    }

    double get_double(const std::string&) override
    {
        return 0;
    }

    float get_float(const std::string&) override
    {
        return 0;
    }

    std::string get_string(const std::string& column_name) override
    {
        if (!m_values)
            return "";
        auto it = m_values->string_columns.find(column_name);
        return it == m_values->string_columns.end() ? "" : it->second;
    }

    bool get_boolean(const std::string&) override
    {
        return false;
    }

    int get_int(uint16_t) override
    {
        return 0;
    }

    uint32_t get_uint(uint16_t) override
    {
        return 0;
    }

    int64_t get_int64(uint16_t) override
    {
        return 0;
    }

    uint64_t get_uint64(uint16_t) override
    {
        return 0;
    }

    double get_double(uint16_t) override
    {
        return 0;
    }

    float get_float(uint16_t) override
    {
        return 0;
    }

    std::string get_string(uint16_t) override
    {
        return "";
    }

    bool get_boolean(uint16_t) override
    {
        return false;
    }

    bool is_null(const std::string&) override
    {
        return false;
    }

    bool is_null(uint16_t) override
    {
        return false;
    }

private:
    int                                  m_row_count;  ///< 结果集行数
    int                                  m_seen;       ///< 已读取行数
    std::shared_ptr<const StubRowValues> m_values;     ///< 按列名返回的行值
};

/// @brief Stub 数据库连接的共享状态
struct StubDbState
{
    bool has_row                = false;  ///< 查询是否返回一行
    int  update_rows            = 1;      ///< 更新返回的受影响行数
    bool throw_on_query         = false;  ///< 查询时是否抛出 SQLException
    bool throw_on_update        = false;  ///< 更新时是否抛出 SQLException
    bool return_null_result_set = false;  ///< 查询是否返回空结果集
    bool throw_on_prepare       = false;  ///< 预处理时是否抛出 SQLException
    bool throw_on_set_string    = false;  ///< 绑定字符串时是否抛出 SQLException
    bool throw_on_set_int       = false;  ///< 绑定整型时是否抛出 SQLException
    bool throw_on_set_datetime  = false;  ///< 绑定时间时是否抛出 SQLException

    /// 查询返回行的列值
    std::shared_ptr<StubRowValues> row = std::make_shared<StubRowValues>();
};

/// @brief 提供固定查询与更新结果的数据库连接 Stub
class StubDbConnection final : public repository::IDbConnection
{
public:
    StubDbConnection() : m_state(std::make_shared<StubDbState>())
    {
    }

    /// @brief 使用共享状态构造连接句柄
    explicit StubDbConnection(std::shared_ptr<StubDbState> state) : m_state(std::move(state))
    {
    }

    /// @brief 返回共享状态
    StubDbState& state()
    {
        return *m_state;
    }

    /// @brief 返回共享状态指针，供提供者创建新句柄
    std::shared_ptr<StubDbState> state_ptr() const
    {
        return m_state;
    }

    void prepare_statement(const std::string&) override
    {
        if (m_state->throw_on_prepare)
            throw sql::SQLException("stub prepare failure");
    }

    void set_string(uint16_t, const std::string&) override
    {
        if (m_state->throw_on_set_string)
            throw sql::SQLException("stub string bind failure");
    }

    void set_int(uint16_t, int) override
    {
        if (m_state->throw_on_set_int)
            throw sql::SQLException("stub int bind failure");
    }

    void set_uint64(uint16_t, uint64_t) override
    {
    }

    void set_datetime(uint16_t, const std::string&) override
    {
        if (m_state->throw_on_set_datetime)
            throw sql::SQLException("stub datetime bind failure");
    }

    std::unique_ptr<dcp::IResultSet> execute_query() override
    {
        if (m_state->throw_on_query)
            throw sql::SQLException("stub query failure");
        if (m_state->return_null_result_set)
            return nullptr;
        return std::make_unique<StubResultSet>(m_state->has_row ? 1 : 0, m_state->row);
    }

    int execute_update() override
    {
        if (m_state->throw_on_update)
            throw sql::SQLException("stub update failure");
        return m_state->update_rows;
    }

private:
    std::shared_ptr<StubDbState> m_state;  ///< 共享状态
};

/// @brief Spy 数据库连接记录的调用信息
struct SpyDbState
{
    bool        has_row     = false;  ///< 查询是否返回一行
    int         update_rows = 1;      ///< 更新返回的受影响行数
    std::string prepared_sql;         ///< 最近一次预处理的 SQL

    std::vector<std::pair<uint16_t, std::string>> string_params;    ///< 字符串绑定参数
    std::vector<std::pair<uint16_t, int>>         int_params;       ///< 整型绑定参数
    std::vector<std::pair<uint16_t, uint64_t>>    uint64_params;    ///< 无符号整型绑定参数
    std::vector<std::pair<uint16_t, std::string>> datetime_params;  ///< 时间绑定参数

    int prepare_calls = 0;  ///< 预处理次数
    int query_calls   = 0;  ///< 查询次数
    int update_calls  = 0;  ///< 更新次数

    std::vector<std::string> events;  ///< 调用顺序

    /// 查询返回行的列值
    std::shared_ptr<StubRowValues> row = std::make_shared<StubRowValues>();
};

/// @brief 记录调用参数与次数的数据库连接 Spy
class SpyDbConnection final : public repository::IDbConnection
{
public:
    SpyDbConnection() : m_state(std::make_shared<SpyDbState>())
    {
    }

    /// @brief 使用共享状态构造连接句柄
    explicit SpyDbConnection(std::shared_ptr<SpyDbState> state) : m_state(std::move(state))
    {
    }

    /// @brief 返回共享状态
    SpyDbState& state()
    {
        return *m_state;
    }

    /// @brief 返回共享状态指针，供提供者创建新句柄
    std::shared_ptr<SpyDbState> state_ptr() const
    {
        return m_state;
    }

    void prepare_statement(const std::string& sql) override
    {
        m_state->events.push_back("prepare");
        m_state->prepared_sql = sql;
        ++m_state->prepare_calls;
    }

    void set_string(uint16_t index, const std::string& value) override
    {
        m_state->events.push_back("bind_string");
        m_state->string_params.emplace_back(index, value);
    }

    void set_int(uint16_t index, int value) override
    {
        m_state->events.push_back("bind_int");
        m_state->int_params.emplace_back(index, value);
    }

    void set_uint64(uint16_t index, uint64_t value) override
    {
        m_state->events.push_back("bind_uint64");
        m_state->uint64_params.emplace_back(index, value);
    }

    void set_datetime(uint16_t index, const std::string& value) override
    {
        m_state->events.push_back("bind_datetime");
        m_state->datetime_params.emplace_back(index, value);
    }

    std::unique_ptr<dcp::IResultSet> execute_query() override
    {
        m_state->events.push_back("query");
        ++m_state->query_calls;
        return std::make_unique<StubResultSet>(m_state->has_row ? 1 : 0, m_state->row);
    }

    int execute_update() override
    {
        m_state->events.push_back("update");
        ++m_state->update_calls;
        return m_state->update_rows;
    }

private:
    std::shared_ptr<SpyDbState> m_state;  ///< 共享状态
};

/// @brief Fake 缓存的数据状态
struct FakeCacheState
{
    std::unordered_map<std::string, std::string> store;  ///< 缓存数据
    std::unordered_map<std::string, int>         ttls;   ///< 缓存过期时间
};

/// @brief 内存缓存 Fake，按真实读写语义维护键值与过期时间
class FakeCacheConnection final : public repository::ICacheConnection
{
public:
    FakeCacheConnection() : m_state(std::make_shared<FakeCacheState>())
    {
    }

    /// @brief 使用共享状态构造连接句柄
    explicit FakeCacheConnection(std::shared_ptr<FakeCacheState> state) : m_state(std::move(state))
    {
    }

    /// @brief 返回共享状态指针，供提供者创建新句柄
    std::shared_ptr<FakeCacheState> state_ptr() const
    {
        return m_state;
    }

    /// @brief 预置缓存值
    /// @param key 缓存键
    /// @param value 缓存值
    /// @param ttl_seconds 过期时间
    void put(const std::string& key, const std::string& value, int ttl_seconds)
    {
        m_state->store[key] = value;
        m_state->ttls[key]  = ttl_seconds;
    }

    /// @brief 判断缓存键是否存在
    bool contains(const std::string& key) const
    {
        return m_state->store.find(key) != m_state->store.end();
    }

    /// @brief 返回缓存键对应的 TTL，不存在时返回 -1
    int ttl_of(const std::string& key) const
    {
        auto it = m_state->ttls.find(key);
        return it == m_state->ttls.end() ? -1 : it->second;
    }

    std::optional<std::string> get(const std::string& key) override
    {
        auto it = m_state->store.find(key);
        if (it == m_state->store.end())
            return std::nullopt;
        return it->second;
    }

    bool set(const std::string& key, const std::string& value, int ttl_seconds) override
    {
        m_state->store[key] = value;
        m_state->ttls[key]  = ttl_seconds;
        return true;
    }

private:
    std::shared_ptr<FakeCacheState> m_state;  ///< 共享状态
};

/// @brief Stub 缓存连接的共享状态
struct StubCacheState
{
    bool throw_on_get = false;  ///< get 时是否抛出运行时异常
    bool throw_on_set = false;  ///< set 时是否抛出运行时异常
    bool set_result   = true;   ///< set 固定返回值
};

/// @brief 提供固定读写结果的缓存连接 Stub
class StubCacheConnection final : public repository::ICacheConnection
{
public:
    StubCacheConnection() : m_state(std::make_shared<StubCacheState>())
    {
    }

    /// @brief 使用共享状态构造连接句柄
    explicit StubCacheConnection(std::shared_ptr<StubCacheState> state) : m_state(std::move(state))
    {
    }

    /// @brief 返回共享状态
    StubCacheState& state()
    {
        return *m_state;
    }

    /// @brief 返回共享状态指针，供提供者创建新句柄
    std::shared_ptr<StubCacheState> state_ptr() const
    {
        return m_state;
    }

    std::optional<std::string> get(const std::string&) override
    {
        if (m_state->throw_on_get)
            throw std::runtime_error("stub cache get failure");
        return std::nullopt;
    }

    bool set(const std::string&, const std::string&, int) override
    {
        if (m_state->throw_on_set)
            throw std::runtime_error("stub cache set failure");
        return m_state->set_result;
    }

private:
    std::shared_ptr<StubCacheState> m_state;  ///< 共享状态
};

/// @brief Mock 缓存连接记录的调用与预置期望
struct MockCacheState
{
    struct SetCall
    {
        std::string key;          ///< 缓存键
        std::string value;        ///< 缓存值
        int         ttl_seconds;  ///< 过期时间
    };

    struct ExpectedSet
    {
        std::string key;          ///< 期望的缓存键
        std::string value;        ///< 期望的缓存值
        int         ttl_seconds;  ///< 期望的过期时间
    };

    int                        expected_get_calls = 1;  ///< 期望的读取次数
    std::vector<ExpectedSet>   expected_sets;           ///< 期望的写入序列
    std::vector<SetCall>       set_calls;               ///< 实际写入序列
    int                        get_calls = 0;           ///< 实际读取次数
    std::optional<std::string> get_result;              ///< get 返回的缓存值
    bool                       set_result = true;       ///< set 返回结果
    bool                       throw_on_get = false;    ///< get 是否抛出异常
    bool                       throw_on_set = false;    ///< set 是否抛出异常
    std::string                last_get_key;            ///< 最近一次读取的 key
    std::vector<std::string>   events;                  ///< 调用顺序
};

/// @brief 预置缓存写回期望的 Mock
class MockCacheConnection final : public repository::ICacheConnection
{
public:
    MockCacheConnection() : m_state(std::make_shared<MockCacheState>())
    {
    }

    /// @brief 使用共享状态构造连接句柄
    explicit MockCacheConnection(std::shared_ptr<MockCacheState> state) : m_state(std::move(state))
    {
    }

    /// @brief 返回共享状态指针，供提供者创建新句柄
    std::shared_ptr<MockCacheState> state_ptr() const
    {
        return m_state;
    }

    /// @brief 返回共享状态
    MockCacheState& state()
    {
        return *m_state;
    }

    /// @brief 追加一条写入期望
    /// @param key 期望的缓存键
    /// @param value 期望的缓存值
    /// @param ttl_seconds 期望的过期时间
    void expect_set(const std::string& key, const std::string& value, int ttl_seconds)
    {
        m_state->expected_sets.push_back({key, value, ttl_seconds});
    }

    std::optional<std::string> get(const std::string& key) override
    {
        m_state->last_get_key = key;
        m_state->events.push_back("get");
        ++m_state->get_calls;
        if (m_state->throw_on_get)
            throw std::runtime_error("mock cache get failure");
        return m_state->get_result;
    }

    bool set(const std::string& key, const std::string& value, int ttl_seconds) override
    {
        m_state->events.push_back("set");
        m_state->set_calls.push_back({key, value, ttl_seconds});
        if (m_state->throw_on_set)
            throw std::runtime_error("mock cache set failure");
        return m_state->set_result;
    }

    /// @brief 校验交互契约是否满足
    void verify() const
    {
        CHECK(m_state->get_calls == m_state->expected_get_calls);
        REQUIRE(m_state->set_calls.size() == m_state->expected_sets.size());
        for (std::size_t i = 0; i < m_state->expected_sets.size(); ++i)
        {
            CHECK(m_state->set_calls[i].key == m_state->expected_sets[i].key);
            CHECK(m_state->set_calls[i].value == m_state->expected_sets[i].value);
            CHECK(m_state->set_calls[i].ttl_seconds == m_state->expected_sets[i].ttl_seconds);
        }
    }

private:
    std::shared_ptr<MockCacheState> m_state;  ///< 共享状态
};

/// @brief 占位用数据库提供者，不应被被测代码访问
class DummyDbProvider final : public repository::IDbConnectionProvider
{
public:
    std::unique_ptr<repository::IDbConnection> acquire() override
    {
        return nullptr;
    }
};

/// @brief 占位用缓存提供者，不应被被测代码访问
class DummyCacheProvider final : public repository::ICacheConnectionProvider
{
public:
    std::unique_ptr<repository::ICacheConnection> acquire() override
    {
        return nullptr;
    }
};

/// @brief 固定返回空连接的数据库提供者 Stub
class UnavailableDbProvider final : public repository::IDbConnectionProvider
{
public:
    std::unique_ptr<repository::IDbConnection> acquire() override
    {
        return nullptr;
    }
};

/// @brief 固定返回空连接的缓存提供者 Stub
class UnavailableCacheProvider final : public repository::ICacheConnectionProvider
{
public:
    std::unique_ptr<repository::ICacheConnection> acquire() override
    {
        return nullptr;
    }
};

/// @brief 暴露内部 Stub 状态的数据库提供者
class StubDbProvider final : public repository::IDbConnectionProvider
{
public:
    StubDbConnection& connection()
    {
        return m_prototype;
    }

    std::unique_ptr<repository::IDbConnection> acquire() override
    {
        return std::make_unique<StubDbConnection>(m_prototype.state_ptr());
    }

private:
    StubDbConnection m_prototype;  ///< 状态原型
};

/// @brief 暴露内部 Stub 状态的缓存提供者
class StubCacheProvider final : public repository::ICacheConnectionProvider
{
public:
    StubCacheConnection& connection()
    {
        return m_prototype;
    }

    std::unique_ptr<repository::ICacheConnection> acquire() override
    {
        return std::make_unique<StubCacheConnection>(m_prototype.state_ptr());
    }

private:
    StubCacheConnection m_prototype;  ///< 状态原型
};

/// @brief 暴露内部 Spy 状态的数据库提供者
class SpyDbProvider final : public repository::IDbConnectionProvider
{
public:
    SpyDbConnection& connection()
    {
        return m_prototype;
    }

    std::unique_ptr<repository::IDbConnection> acquire() override
    {
        return std::make_unique<SpyDbConnection>(m_prototype.state_ptr());
    }

private:
    SpyDbConnection m_prototype;  ///< 状态原型
};

/// @brief 暴露内部 Fake 状态的缓存提供者
class FakeCacheProvider final : public repository::ICacheConnectionProvider
{
public:
    FakeCacheConnection& fake()
    {
        return m_prototype;
    }

    std::unique_ptr<repository::ICacheConnection> acquire() override
    {
        return std::make_unique<FakeCacheConnection>(m_prototype.state_ptr());
    }

private:
    FakeCacheConnection m_prototype;  ///< 内存缓存状态原型
};

/// @brief 暴露内部 Mock 状态的缓存提供者
class MockCacheProvider final : public repository::ICacheConnectionProvider
{
public:
    MockCacheConnection& mock()
    {
        return m_prototype;
    }

    std::unique_ptr<repository::ICacheConnection> acquire() override
    {
        return std::make_unique<MockCacheConnection>(m_prototype.state_ptr());
    }

private:
    MockCacheConnection m_prototype;  ///< 带期望的状态原型
};

}  // namespace test_support
