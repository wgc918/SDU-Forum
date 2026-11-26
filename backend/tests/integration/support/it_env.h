//-----------------------------------------------------------------------------
// 文件: it_env.h
// 描述: 集成测试环境配置读取、依赖可用性探测与安全护栏。
//-----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>

namespace it
{

/// 测试数据库连接参数
struct DbSettings
{
    std::string host = "127.0.0.1";
    uint16_t    port = 3306;
    std::string user;
    std::string password;
    std::string dbname;
};

/// 测试 Redis 连接参数
struct RedisSettings
{
    std::string host = "127.0.0.1";
    uint16_t    port = 6379;
    std::string password;
    int         db = 15;
};

/// 读取 SDU_IT_* 环境变量后的数据库参数
const DbSettings& db_settings();

/// 读取 SDU_IT_* 环境变量后的 Redis 参数
const RedisSettings& redis_settings();

/// 集成测试 INI 配置的绝对路径（SDU_FORUM_CONFIG 可覆盖）
std::string config_path();

/// 服务端可执行文件路径（CMake 编译期注入）
std::string server_binary();

/// HTTP 集成测试使用的端口（SDU_IT_SERVER_PORT，默认 18080）
int server_port();

/// 加载集成测试配置并执行安全护栏；失败抛出 std::runtime_error
/// @note 每次调用都会重新加载 INI，便于配置类用例修改后恢复
void load_config();

/// 安全护栏：表名必须 it_ 前缀、禁止系统库、Redis 禁止 db 0
/// @throws std::runtime_error 任一约束不满足
void check_safety();

/// Redis 是否可用（TCP PING，结果进程内缓存）
bool redis_available();

/// 生成本次测试唯一的短后缀（满足 user_id <= 12 / admin_id <= 8 约束）
std::string unique_suffix();

}  // namespace it
