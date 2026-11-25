#include "config/app_config.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

#include "ini.h"

namespace infra
{
namespace
{

/// 展开 ${VAR} 与 ${VAR:-default} 形式的环境变量引用
std::string expand_env(const std::string& raw)
{
    std::string out;
    std::size_t pos = 0;

    while (pos < raw.size())
    {
        std::size_t start = raw.find("${", pos);
        if (start == std::string::npos)
        {
            out += raw.substr(pos);
            break;
        }

        out += raw.substr(pos, start - pos);

        std::size_t end = raw.find('}', start + 2);
        if (end == std::string::npos)
        {
            out += raw.substr(start);
            break;
        }

        std::string expr = raw.substr(start + 2, end - start - 2);
        std::string name = expr;
        std::string value_fallback;

        std::size_t sep = expr.find(":-");
        if (sep != std::string::npos)
        {
            name           = expr.substr(0, sep);
            value_fallback = expr.substr(sep + 2);
        }

        const char* value = std::getenv(name.c_str());
        out += (value != nullptr && *value != '\0') ? value : value_fallback;
        pos = end + 1;
    }

    return out;
}

IniFile::Section& section_of(const std::string& name)
{
    static IniFile::Section kEmpty;
    try
    {
        return IniFile::instance()[name];
    }
    catch (const std::out_of_range&)
    {
        return kEmpty;
    }
}

std::string string_of(const IniFile::Section& section, const std::string& key,
                      const std::string& fallback)
{
    auto it = section.find(key);
    if (it == section.end())
        return fallback;
    return expand_env(it->second.val());
}

}  // namespace

AppConfig& AppConfig::instance()
{
    static AppConfig config;
    return config;
}

void AppConfig::load(const std::string& path)
{
    std::ifstream probe(path);
    if (!probe.is_open())
        throw std::runtime_error("config file not found: " + path);
    probe.close();

    IniFile::instance().load(path);

    auto& server        = section_of("server");
    m_server.address    = std::string(server["address"]);
    m_server.port       = static_cast<uint16_t>(static_cast<int>(server["port"]));
    m_server.thread_num = static_cast<size_t>(static_cast<int>(server["thread_num"]));

    auto& logging       = section_of("log");
    m_logging.level     = std::string(logging["level"]);
    m_logging.file      = std::string(logging["file"]);
    m_logging.console   = logging["console"];
    m_logging.max_lines = static_cast<size_t>(static_cast<int>(logging["max_lines"]));

    auto& database                = section_of("database");
    m_database.host               = std::string(database["host"]);
    m_database.port               = static_cast<uint16_t>(static_cast<int>(database["port"]));
    m_database.user               = std::string(database["user"]);
    m_database.password           = string_of(database, "password", "");
    m_database.dbname             = std::string(database["dbname"]);
    m_database.pool_min           = database["pool_min"];
    m_database.pool_max           = database["pool_max"];
    m_database.max_idle_time      = database["max_idle_time"];
    m_database.connection_timeout = database["connection_timeout_ms"];
    m_database.op_num             = database["op_num"];

    auto& jwt                  = section_of("jwt");
    m_jwt.issuer               = std::string(jwt["issuer"]);
    m_jwt.access_secret        = string_of(jwt, "access_jwt_secret", "");
    m_jwt.refresh_secret       = string_of(jwt, "refresh_jwt_secret", "");
    m_jwt.admin_access_secret  = string_of(jwt, "admin_access_jwt_secret", "");
    m_jwt.admin_refresh_secret = string_of(jwt, "admin_refresh_jwt_secret", "");
    m_jwt.hash_secret          = string_of(jwt, "hash_secret", "");
    m_jwt.access_token_ttl_m   = jwt["access_token_ttl"];
    m_jwt.refresh_token_ttl_h  = jwt["refresh_token_ttl"];

    auto& table                 = section_of("tables");
    m_table.account_table       = std::string(table["account_table"]);
    m_table.blacklist_table     = std::string(table["blacklist_table"]);
    m_table.refresh_token_table = std::string(table["refresh_token_table"]);
    m_table.admin_account_table = std::string(table["admin_account_table"]);

    auto& redis      = section_of("redis");
    m_redis.host     = std::string(redis["host"]);
    m_redis.port     = static_cast<uint16_t>(static_cast<int>(redis["port"]));
    m_redis.password = string_of(redis, "password", "");
    m_redis.db       = redis["db"];

    validate();
}

void AppConfig::validate() const
{
    if (m_server.address.empty())
        throw std::runtime_error("config item [server.address] is required");
    if (m_server.port == 0)
        throw std::runtime_error("config item [server.port] is invalid");
    if (m_server.thread_num == 0)
        throw std::runtime_error("config item [server.thread_num] is invalid");

    if (m_logging.level.empty())
        throw std::runtime_error("config item [log.level] is required");
    if (!m_logging.console && m_logging.file.empty())
        throw std::runtime_error("config items [log.console/log.file] are invalid");

    if (m_database.host.empty())
        throw std::runtime_error("config item [database.host] is required");
    if (m_database.port == 0)
        throw std::runtime_error("config item [database.port] is invalid");
    if (m_database.user.empty() || m_database.dbname.empty())
        throw std::runtime_error("config items [database.user] and [database.dbname] are required");
    if (m_database.pool_min <= 0 || m_database.pool_max < m_database.pool_min)
        throw std::runtime_error("config items [database.pool_min/pool_max] are invalid");
    if (m_database.max_idle_time <= 0)
        throw std::runtime_error("config item [database.max_idle_time] is invalid");
    if (m_database.connection_timeout <= 0)
        throw std::runtime_error("config item [database.connection_timeout_ms] is invalid");
    if (m_database.op_num <= 0)
        throw std::runtime_error("config item [database.op_num] is invalid");

    if (m_jwt.issuer.empty())
        throw std::runtime_error("config item [jwt.issuer] is required");
    if (m_jwt.access_secret.empty() || m_jwt.refresh_secret.empty())
        throw std::runtime_error(
            "config items [jwt.access_jwt_secret/refresh_jwt_secret] are "
            "required");
    if (m_jwt.admin_access_secret.empty() || m_jwt.admin_refresh_secret.empty())
        throw std::runtime_error(
            "config items "
            "[jwt.admin_access_jwt_secret/admin_refresh_jwt_secret] are "
            "required");
    if (m_jwt.hash_secret.empty())
        throw std::runtime_error("config item [jwt.hash_secret] is required");
    if (m_jwt.access_token_ttl_m <= 0 || m_jwt.refresh_token_ttl_h <= 0)
        throw std::runtime_error(
            "config items [jwt.access_token_ttl/refresh_token_ttl] "
            "are invalid");

    if (m_table.account_table.empty())
        throw std::runtime_error("config item [tables.account_table] is required");
    if (m_table.blacklist_table.empty())
        throw std::runtime_error("config item [tables.blacklist_table] is required");
    if (m_table.refresh_token_table.empty())
        throw std::runtime_error("config item [tables.refresh_token_table] is required");
    if (m_table.admin_account_table.empty())
        throw std::runtime_error("config item [tables.admin_account_table] is required");

    if (m_redis.host.empty())
        throw std::runtime_error("config item [redis.host] is required");
    if (m_redis.port == 0)
        throw std::runtime_error("config item [redis.port] is invalid");
    if (m_redis.password.empty())
        throw std::runtime_error("config item [redis.password] is required");
    if (m_redis.db < 0)
        throw std::runtime_error("config item [redis.db] is invalid");
}

const ServerConfig& AppConfig::server() const noexcept
{
    return m_server;
}

const LoggingConfig& AppConfig::logging() const noexcept
{
    return m_logging;
}

const DatabaseConfig& AppConfig::database() const noexcept
{
    return m_database;
}

const JwtConfig& AppConfig::jwt() const noexcept
{
    return m_jwt;
}

const TableNameConfig& AppConfig::table() const noexcept
{
    return m_table;
}

const RedisConfig& AppConfig::redis() const noexcept
{
    return m_redis;
}

}  // namespace infra
