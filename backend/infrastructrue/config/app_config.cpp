#include "config/app_config.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

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

        std::string expr  = raw.substr(start + 2, end - start - 2);
        std::string name  = expr;
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

const IniFile::Section& section_of(const std::string& name)
{
    static const IniFile::Section kEmpty;
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

int int_of(const IniFile::Section& section, const std::string& key, int fallback)
{
    std::string raw = string_of(section, key, "");
    if (raw.empty())
        return fallback;

    try
    {
        return std::stoi(raw);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("config item [" + key + "] is not a valid integer: " + raw);
    }
}

bool bool_of(const IniFile::Section& section, const std::string& key, bool fallback)
{
    std::string raw = string_of(section, key, "");
    if (raw.empty())
        return fallback;
    return raw == "true" || raw == "TRUE" || raw == "1";
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

    const auto& server = section_of("server");
    m_server.address    = string_of(server, "address", m_server.address);
    m_server.port       = static_cast<uint16_t>(int_of(server, "port", m_server.port));
    m_server.thread_num = static_cast<size_t>(int_of(server, "thread_num", 4));

    const auto& logging = section_of("log");
    m_logging.level      = string_of(logging, "level", m_logging.level);
    m_logging.file       = string_of(logging, "file", m_logging.file);
    m_logging.console    = bool_of(logging, "console", m_logging.console);
    m_logging.max_lines  = static_cast<size_t>(int_of(logging, "max_lines", 100000));

    const auto& database = section_of("database");
    m_database.host     = string_of(database, "host", m_database.host);
    m_database.port     = static_cast<uint16_t>(int_of(database, "port", m_database.port));
    m_database.user     = string_of(database, "user", "");
    m_database.password = string_of(database, "password", "");
    m_database.dbname   = string_of(database, "dbname", "");
    m_database.pool_min           = int_of(database, "pool_min", m_database.pool_min);
    m_database.pool_max           = int_of(database, "pool_max", m_database.pool_max);
    m_database.max_idle_time      = int_of(database, "max_idle_time", m_database.max_idle_time);
    m_database.connection_timeout = int_of(database, "connection_timeout_ms",
                                           m_database.connection_timeout);
    m_database.op_num             = int_of(database, "op_num", m_database.op_num);

    const auto& auth = section_of("auth");
    m_jwt.issuer               = string_of(auth, "issuer", "SDU_Forum");
    m_jwt.access_secret        = string_of(auth, "access_jwt_secret", "");
    m_jwt.refresh_secret       = string_of(auth, "refresh_jwt_secret", "");
    m_jwt.access_token_minutes = int_of(auth, "access_token_minutes", 60);
    m_jwt.refresh_token_hours  = int_of(auth, "refresh_token_hours", 168);
    m_auth.account_table       = string_of(auth, "account_table", m_auth.account_table);
    m_auth.blacklist_table     = string_of(auth, "blacklist_table", m_auth.blacklist_table);

    validate();
}

void AppConfig::validate() const
{
    if (m_server.port == 0)
        throw std::runtime_error("config item [server.port] is invalid");
    if (m_database.user.empty() || m_database.dbname.empty())
        throw std::runtime_error("config items [database.user] and [database.dbname] are required");
    if (m_database.pool_min <= 0 || m_database.pool_max < m_database.pool_min)
        throw std::runtime_error("config items [database.pool_min/pool_max] are invalid");
    if (m_jwt.access_secret.empty() || m_jwt.refresh_secret.empty())
        throw std::runtime_error("config items [auth.access_jwt_secret/refresh_jwt_secret] are "
                                 "required");
    if (m_jwt.access_token_minutes <= 0 || m_jwt.refresh_token_hours <= 0)
        throw std::runtime_error("config items [auth.access_token_minutes/refresh_token_hours] "
                                 "are invalid");
    if (m_auth.account_table.empty() || m_auth.blacklist_table.empty())
        throw std::runtime_error("config items [auth.account_table/blacklist_table] are required");
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

const AuthConfig& AppConfig::auth() const noexcept
{
    return m_auth;
}

}  // namespace infra
