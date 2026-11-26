//-----------------------------------------------------------------------------
// 文件: it_support.cpp
// 描述: 集成测试公共设施实现。
//-----------------------------------------------------------------------------
#include "it_support.h"

#include <cstdio>
#include <ctime>
#include <random>
#include <stdexcept>

#include "config/app_config.h"
#include "conn-pool/ConnectionPool.h"
#include "database/IDatabaseConnection.h"
#include "it_env.h"
#include "redis-pool/RedisConnGuard.h"
#include "redis-pool/RedisConnectionPool.h"
#include "repositories/blacklist_repository.h"
#include "security/password_hasher.h"

namespace it
{
namespace
{

/// 集成测试统一使用的密码迭代次数（远低于生产的 150000，缩短用例耗时）
constexpr int kTestIterations = 1000;

std::string table(const std::string& name)
{
    return "`" + name + "`";
}

std::mt19937& rng()
{
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

std::string random_hex(size_t length)
{
    static const char* digits = "0123456789abcdef";
    std::uniform_int_distribution<int> dist(0, 15);
    std::string out;
    out.reserve(length);
    for (size_t i = 0; i < length; ++i)
        out.push_back(digits[dist(rng())]);
    return out;
}

const std::string& account_table()
{
    static const std::string name = infra::AppConfig::instance().table().account_table;
    return name;
}

const std::string& admin_table()
{
    static const std::string name = infra::AppConfig::instance().table().admin_account_table;
    return name;
}

const std::string& blacklist_table()
{
    static const std::string name = infra::AppConfig::instance().table().blacklist_table;
    return name;
}

const std::string& refresh_table()
{
    static const std::string name = infra::AppConfig::instance().table().refresh_token_table;
    return name;
}

}  // namespace

repository::IDbConnectionProvider& db()
{
    static bool initialized = false;
    static repository::PooledDbConnectionProvider* provider = nullptr;

    if (!initialized)
    {
        load_config();
        check_safety();

        const auto& cfg = infra::AppConfig::instance().database();
        dcp::ConnectionConfig conn_cfg;
        conn_cfg.db_name   = cfg.dbname;
        conn_cfg.user_name = cfg.user;
        conn_cfg.password  = cfg.password;
        conn_cfg.ip        = cfg.host;
        conn_cfg.port      = cfg.port;

        try
        {
            auto& pool = dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL);
            pool.init(conn_cfg, cfg.pool_min, cfg.pool_max, cfg.max_idle_time,
                      cfg.connection_timeout, cfg.op_num);
            provider = new repository::PooledDbConnectionProvider(pool);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("integration: MySQL pool init failed: ") + e.what() +
                                     " (check SDU_IT_DB_* and whether MySQL is running)");
        }
        initialized = true;
    }

    return *provider;
}

repository::TokenStoreConfig token_store_config()
{
    const auto& cfg = infra::AppConfig::instance();
    repository::TokenStoreConfig out;
    out.refresh_token_table = cfg.table().refresh_token_table;
    out.hash_secret         = cfg.jwt().hash_secret;
    out.refresh_ttl_s       = cfg.jwt().refresh_token_ttl_seconds();
    out.access_ttl_s        = cfg.jwt().access_token_ttl_seconds();
    return out;
}

int exec(const std::string& sql)
{
    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");
    conn->prepare_statement(sql);
    return conn->execute_update();
}

uint64_t query_u64(const std::string& sql)
{
    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");
    conn->prepare_statement(sql);
    auto result = conn->execute_query();
    if (!result || !result->next())
        throw std::runtime_error("integration: query returned no row: " + sql);
    return result->get_uint64("v");
}

std::string query_string(const std::string& sql)
{
    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");
    conn->prepare_statement(sql);
    auto result = conn->execute_query();
    if (!result || !result->next())
        throw std::runtime_error("integration: query returned no row: " + sql);
    return result->get_string("v");
}

void reset_db()
{
    exec("DELETE FROM " + table(blacklist_table()));
    exec("DELETE FROM " + table(refresh_table()));
    exec("DELETE FROM " + table(account_table()));
    exec("DELETE FROM " + table(admin_table()));
}

uint64_t seed_account(const std::string& user_id, const std::string& password,
                      std::string* public_id)
{
    const auto hash  = infra::hash_password(password, kTestIterations);
    const auto pub   = new_public_id();

    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");

    conn->prepare_statement("INSERT INTO " + table(account_table()) +
                            " (public_id, user_id, password_hash, salt, iterations)"
                            " VALUES (?, ?, ?, ?, ?)");
    conn->set_string(1, pub);
    conn->set_string(2, user_id);
    conn->set_string(3, hash.hash);
    conn->set_string(4, hash.salt);
    conn->set_int(5, kTestIterations);
    conn->execute_update();

    conn->prepare_statement("SELECT LAST_INSERT_ID() AS v");
    auto result = conn->execute_query();
    if (!result || !result->next())
        throw std::runtime_error("integration: failed to read insert id");
    const uint64_t id = result->get_uint64("v");

    if (public_id)
        *public_id = pub;
    return id;
}

uint64_t seed_admin(const std::string& admin_id, const std::string& password)
{
    const auto hash = infra::hash_password(password, kTestIterations);

    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");

    conn->prepare_statement("INSERT INTO " + table(admin_table()) +
                            " (admin_id, password_hash, salt, iterations) VALUES (?, ?, ?, ?)");
    conn->set_string(1, admin_id);
    conn->set_string(2, hash.hash);
    conn->set_string(3, hash.salt);
    conn->set_int(4, kTestIterations);
    conn->execute_update();

    conn->prepare_statement("SELECT LAST_INSERT_ID() AS v");
    auto result = conn->execute_query();
    if (!result || !result->next())
        throw std::runtime_error("integration: failed to read insert id");
    return result->get_uint64("v");
}

uint64_t account_id_of(const std::string& user_id)
{
    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");

    conn->prepare_statement("SELECT id AS v FROM " + table(account_table()) +
                            " WHERE user_id = ? LIMIT 1");
    conn->set_string(1, user_id);
    auto result = conn->execute_query();
    if (!result || !result->next())
        return 0;
    return result->get_uint64("v");
}

uint64_t seed_ban(uint64_t account_id, ban::BanType type, ban::BanStatus status,
                  ban::ReasonCode reason_code, const std::string& reason,
                  const std::string& banned_at, const std::string& expire_at,
                  ban::BanSource source, const std::string& banned_by)
{
    auto conn = db().acquire();
    if (!conn)
        throw std::runtime_error("integration: no database connection available");

    conn->prepare_statement(
        "INSERT INTO " + table(blacklist_table()) +
        " (account_id, reason, reason_code, ban_type, banned_by, banned_at, expire_at,"
        "  unbanned_at, unbanned_by, unban_reason, status, source, ref_id)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, '', '', ?, ?, 0)");
    conn->set_uint64(1, account_id);
    conn->set_string(2, reason);
    conn->set_int(3, static_cast<int>(reason_code));
    conn->set_int(4, static_cast<int>(type));
    conn->set_string(5, banned_by);
    conn->set_datetime(6, banned_at);
    conn->set_datetime(7, expire_at);
    conn->set_datetime(8, repository::NOT_UNBANNED);
    conn->set_int(9, static_cast<int>(status));
    conn->set_string(10, ban::ban_source_to_string(source));
    conn->execute_update();

    conn->prepare_statement("SELECT LAST_INSERT_ID() AS v");
    auto result = conn->execute_query();
    if (!result || !result->next())
        throw std::runtime_error("integration: failed to read insert id");
    return result->get_uint64("v");
}

std::string new_user_id()
{
    return "it" + random_hex(10);
}

std::string new_admin_id()
{
    return "ad" + random_hex(6);
}

std::string new_public_id()
{
    return "itpub" + random_hex(19);
}

std::string local_timestamp(int64_t offset_seconds)
{
    const std::time_t now = std::time(nullptr) + static_cast<std::time_t>(offset_seconds);
    std::tm           local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", local.tm_year + 1900,
                  local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    return std::string(buf);
}

void redis_flush_db()
{
    if (!redis_available())
        return;

    static bool initialized = false;
    if (!initialized)
    {
        const auto& cfg = infra::AppConfig::instance().redis();
        rcp::RedisConnectionConfig redis_cfg;
        redis_cfg.host     = cfg.host;
        redis_cfg.port     = cfg.port;
        redis_cfg.password = cfg.password;
        redis_cfg.db       = cfg.db;

        rcp::RedisConnectionPool::instance().init(redis_cfg, 1, 4, 30, 1000, 1);
        initialized = true;
    }

    auto guard = rcp::RedisConnectionPool::instance().get_connection();
    if (guard)
        guard->flushdb();
}

}  // namespace it
