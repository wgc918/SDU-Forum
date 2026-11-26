//-----------------------------------------------------------------------------
// 文件: it_support.h
// 描述: 集成测试公共设施：真实 MySQL 连接池、数据清理、造数、Redis 清理。
//-----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>

#include "common/ban_enums.h"
#include "repositories/connection_provider.h"
#include "repositories/token_repository.h"

namespace it
{

/// 获取集成测试数据库连接提供者（首次调用时初始化连接池）
/// @throws std::runtime_error 连接池初始化失败
repository::IDbConnectionProvider& db();

/// 基于当前 INI 配置构造 TokenStoreConfig
repository::TokenStoreConfig token_store_config();

/// 清空全部 it_ 表（按外键顺序），用于用例隔离
void reset_db();

/// 执行无参 SQL，返回受影响行数
int exec(const std::string& sql);

/// 执行单值查询，SQL 需用 AS v 别名（如 SELECT COUNT(*) AS v）
uint64_t query_u64(const std::string& sql);

/// 执行单值字符串查询，SQL 需用 AS v 别名
std::string query_string(const std::string& sql);

/// 插入一个测试账号，返回自增主键
uint64_t seed_account(const std::string& user_id, const std::string& password,
                      std::string* public_id = nullptr);

/// 插入一个测试管理员（低迭代次数，保证登录用例速度），返回自增主键
uint64_t seed_admin(const std::string& admin_id, const std::string& password);

/// 通过 user_id 查主键，不存在返回 0
uint64_t account_id_of(const std::string& user_id);

/// 直接插入一条封禁记录，便于构造历史/过期/已解除等状态
uint64_t seed_ban(uint64_t account_id, ban::BanType type, ban::BanStatus status,
                  ban::ReasonCode reason_code, const std::string& reason,
                  const std::string& banned_at, const std::string& expire_at,
                  ban::BanSource source = ban::BanSource::Manual,
                  const std::string& banned_by = "admin");

/// 生成满足长度约束的随机标识
std::string new_user_id();
std::string new_admin_id();
std::string new_public_id();

/// 本地时区的 "YYYY-MM-DD HH:MM:SS" 时间串（与 MySQL NOW() 对齐）
/// @param offset_seconds 相对当前时间的偏移，负数表示过去
std::string local_timestamp(int64_t offset_seconds = 0);

/// 清空 Redis 当前 db（仅 redis_available() 为真时调用）
void redis_flush_db();

}  // namespace it
