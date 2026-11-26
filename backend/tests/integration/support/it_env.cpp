//-----------------------------------------------------------------------------
// 文件: it_env.cpp
// 描述: 集成测试环境配置读取、依赖探测与安全护栏实现。
//-----------------------------------------------------------------------------
#include "it_env.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <stdexcept>

#include "config/app_config.h"

namespace it
{
namespace
{

std::string env_or(const char* name, const std::string& fallback)
{
    const char* value = std::getenv(name);
    return (value && *value) ? std::string(value) : fallback;
}

DbSettings make_db_settings()
{
    DbSettings s;
    s.host     = env_or("SDU_IT_DB_HOST", "127.0.0.1");
    s.port     = static_cast<uint16_t>(std::stoi(env_or("SDU_IT_DB_PORT", "3306")));
    s.user     = env_or("SDU_IT_DB_USER", "wgc");
    s.password = env_or("SDU_IT_DB_PASSWORD", "123456");
    s.dbname   = env_or("SDU_IT_DB_NAME", "testdb");
    return s;
}

RedisSettings make_redis_settings()
{
    RedisSettings s;
    s.host     = env_or("SDU_IT_REDIS_HOST", "127.0.0.1");
    s.port     = static_cast<uint16_t>(std::stoi(env_or("SDU_IT_REDIS_PORT", "6379")));
    s.password = env_or("SDU_IT_REDIS_PASSWORD", "");
    s.db       = std::stoi(env_or("SDU_IT_REDIS_DB", "15"));
    return s;
}

/// 带超时的 TCP 连接；成功返回 fd，失败返回 -1
int tcp_connect(const std::string& host, uint16_t port, int timeout_ms)
{
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo*        result = nullptr;
    const std::string service = std::to_string(port);
    if (::getaddrinfo(host.c_str(), service.c_str(), &hints, &result) != 0)
        return -1;

    int fd = -1;
    for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next)
    {
        fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;

        timeval tv{};
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;

        ::close(fd);
        fd = -1;
    }

    ::freeaddrinfo(result);
    return fd;
}

bool tcp_probe_redis(const std::string& host, uint16_t port)
{
    const int fd = tcp_connect(host, port, 200);
    if (fd < 0)
        return false;

    const char ping[] = "PING\r\n";
    const bool sent   = ::send(fd, ping, sizeof(ping) - 1, 0) == static_cast<ssize_t>(sizeof(ping) - 1);
    char       buf[32] = {0};
    const ssize_t n    = sent ? ::recv(fd, buf, sizeof(buf) - 1, 0) : -1;
    ::close(fd);

    return n > 0 && std::strncmp(buf, "+PONG", 5) == 0;
}

}  // namespace

const DbSettings& db_settings()
{
    static const DbSettings s = make_db_settings();
    return s;
}

const RedisSettings& redis_settings()
{
    static const RedisSettings s = make_redis_settings();
    return s;
}

std::string config_path()
{
    const std::string from_env = env_or("SDU_FORUM_CONFIG", "");
    if (!from_env.empty())
        return from_env;
    return std::string(SDU_IT_SOURCE_DIR) + "/tests/integration/config/config-int.ini";
}

std::string server_binary()
{
    return std::string(SDU_IT_SERVER_BIN);
}

int server_port()
{
    return infra::AppConfig::instance().server().port;
}

void load_config()
{
    infra::AppConfig::instance().load(config_path());
}

void check_safety()
{
    const auto& cfg = infra::AppConfig::instance();

    const std::string tables[] = {cfg.table().account_table, cfg.table().blacklist_table,
                                  cfg.table().refresh_token_table, cfg.table().admin_account_table};
    for (const auto& table : tables)
    {
        if (table.rfind("it_", 0) != 0)
            throw std::runtime_error("integration guard: table '" + table +
                                     "' must start with 'it_' (refuse to touch real tables)");
    }

    const std::string db = cfg.database().dbname;
    if (db == "mysql" || db == "information_schema" || db == "performance_schema" || db == "sys")
        throw std::runtime_error("integration guard: refusing system database '" + db + "'");

    if (cfg.redis().db < 1)
        throw std::runtime_error("integration guard: redis db must be >= 1 (got " +
                                 std::to_string(cfg.redis().db) + ")");
}

bool redis_available()
{
    static const bool available = tcp_probe_redis(redis_settings().host, redis_settings().port);
    return available;
}

std::string unique_suffix()
{
    static std::atomic<uint32_t> counter{0};
    const uint32_t value = counter.fetch_add(1);
    char           buf[16];
    std::snprintf(buf, sizeof(buf), "%06u%03u", static_cast<unsigned>(::getpid() % 1000000),
                  static_cast<unsigned>(value % 1000));
    return buf;
}

}  // namespace it
