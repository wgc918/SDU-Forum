#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace infra
{

struct ServerConfig
{
    std::string address    = "0.0.0.0";
    uint16_t    port       = 8080;
    size_t      thread_num = 4;
};

struct LoggingConfig
{
    std::string level     = "INFO";
    std::string file      = "logs/server.log";
    bool        console   = true;
    size_t      max_lines = 100000;
};

struct DatabaseConfig
{
    std::string host = "127.0.0.1";
    uint16_t    port = 3306;
    std::string user;
    std::string password;
    std::string dbname;
    int         pool_min           = 5;
    int         pool_max           = 30;
    int         max_idle_time      = 300;
    int         connection_timeout = 3000;
    int         op_num             = 4;
};

struct RedisConfig
{
    std::string host = "127.0.0.1";
    uint16_t    port = 6379;
    std::string password;  // 密码（默认为空，无密码）
    int         db = 0;
};

struct JwtConfig

{
    std::string issuer;
    std::string access_secret;
    std::string refresh_secret;
    std::string hash_secret          = "dfacadgafeqwfqvab";  // token存储时转hash使用
    int         access_token_minutes = 60;
    int         refresh_token_hours  = 168;

    int access_token_ttl_seconds() const
    {
        return access_token_minutes * 60;
    }

    int refresh_token_ttl_seconds() const
    {
        return refresh_token_hours * 3600;
    }
};

struct AuthConfig
{
    std::string account_table       = "account";
    std::string blacklist_table     = "blacklist";
    std::string refresh_token_table = "refresh_tokens";
};

/// 进程级配置，启动时从 INI 文件加载一次
class AppConfig
{
public:
    static AppConfig& instance();

    AppConfig(const AppConfig&)            = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    /// @throws std::runtime_error 配置文件不存在或配置项非法
    void load(const std::string& path);

    const ServerConfig& server() const noexcept;
    const LoggingConfig& logging() const noexcept;
    const DatabaseConfig& database() const noexcept;
    const JwtConfig& jwt() const noexcept;
    const AuthConfig& auth() const noexcept;
    const RedisConfig& redis() const noexcept;

private:
    AppConfig() = default;

    void validate() const;

    ServerConfig   m_server;
    LoggingConfig  m_logging;
    DatabaseConfig m_database;
    JwtConfig      m_jwt;
    AuthConfig     m_auth;
    RedisConfig    m_redis;
};

}  // namespace infra
