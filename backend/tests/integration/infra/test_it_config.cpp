//-----------------------------------------------------------------------------
// 文件: test_it_config.cpp
// 描述: L4 组件层 —— 集成配置加载、环境变量展开与安全护栏。
//-----------------------------------------------------------------------------
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"

namespace
{

std::string write_temp_config(const std::string& name, const std::string& content)
{
    const std::string path = "/tmp/sdu_it_" + name + "_" + std::to_string(::getpid()) + ".ini";
    std::ofstream     out(path);
    REQUIRE(out.is_open());
    out << content;
    out.close();
    return path;
}

/// 构造最小可用配置；三个参数用于注入安全护栏应当拒绝的取值
std::string make_config(const std::string& account_table, const std::string& dbname,
                        int                redis_db)
{
    std::string content = R"ini(
[server]
address = 127.0.0.1
port = 18080
thread_num = 2

[log]
level = warn
file = /tmp/sdu_it_config.log
console = false
max_lines = 100

[database]
host = 127.0.0.1
port = 3306
user = wgc
password = 123456
dbname = )ini";
    content += dbname;
    content += R"ini(
pool_min = 1
pool_max = 4
max_idle_time = 30
connection_timeout_ms = 3000
op_num = 1

[jwt]
issuer = SDU_Forum_IT
access_jwt_secret = a
refresh_jwt_secret = b
admin_access_jwt_secret = c
admin_refresh_jwt_secret = d
hash_secret = e
access_token_ttl = 60
refresh_token_ttl = 168

[tables]
account_table = )ini";
    content += account_table;
    content += R"ini(
blacklist_table = it_blacklist
refresh_token_table = it_refresh_tokens
admin_account_table = it_admin_account

[redis]
host = 127.0.0.1
port = 6379
db = )ini";
    content += std::to_string(redis_db);
    content += "\n";
    return content;
}

void restore_config()
{
    infra::AppConfig::instance().load(it::config_path());
}

}  // namespace

TEST_CASE("config: 集成配置加载与关键字段")
{
    it::load_config();
    const auto& cfg = infra::AppConfig::instance();

    CHECK(cfg.jwt().issuer == "SDU_Forum_IT");
    CHECK(cfg.jwt().access_token_ttl_seconds() == 60 * 60);
    CHECK(cfg.jwt().refresh_token_ttl_seconds() == 168 * 3600);

    CHECK(cfg.table().account_table == "it_account");
    CHECK(cfg.table().blacklist_table == "it_blacklist");
    CHECK(cfg.table().refresh_token_table == "it_refresh_tokens");
    CHECK(cfg.table().admin_account_table == "it_admin_account");

    CHECK(cfg.database().dbname == "testdb");
    CHECK(cfg.database().pool_min >= 1);
    CHECK(cfg.database().pool_max >= cfg.database().pool_min);

    CHECK(cfg.redis().db >= 1);
    CHECK(cfg.server().port == it::server_port());
}

TEST_CASE("config: 安全护栏通过")
{
    it::load_config();
    CHECK_NOTHROW(it::check_safety());
}

TEST_CASE("config: 非 it_ 前缀表名被安全护栏拒绝")
{
    it::load_config();
    const std::string path = write_temp_config("bad_table", make_config("account", "testdb", 15));
    infra::AppConfig::instance().load(path);
    CHECK_THROWS_AS(it::check_safety(), std::runtime_error);

    restore_config();
    std::remove(path.c_str());
    CHECK_NOTHROW(it::check_safety());
}

TEST_CASE("config: 系统库被安全护栏拒绝")
{
    it::load_config();
    const std::string path = write_temp_config("bad_db", make_config("it_account", "mysql", 15));
    infra::AppConfig::instance().load(path);
    CHECK_THROWS_AS(it::check_safety(), std::runtime_error);

    restore_config();
    std::remove(path.c_str());
}

TEST_CASE("config: redis db 为 0 被安全护栏拒绝")
{
    it::load_config();
    const std::string path = write_temp_config("bad_redis", make_config("it_account", "testdb", 0));
    infra::AppConfig::instance().load(path);
    CHECK_THROWS_AS(it::check_safety(), std::runtime_error);

    restore_config();
    std::remove(path.c_str());
}
