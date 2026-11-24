# 基础设施与公共组件

> 适用代码：`backend/infrastructrue/`（目录名拼写如此，不要"修正"）  
> 最后核对：2026-11-24

本文件记录各基础设施库的**实际行为**、默认参数、已知缺陷与接入方式。修改这些组件会影响所有业务模块，必须先读 [architecture.md](./architecture.md) 的全局约束。

## 1. MySQL 连接池 `database-connection-pool`

命名空间 `dcp`。核心类型：

| 类型 | 作用 |
| --- | --- |
| `ConnectionPool` | 按 `DatabaseType` 隔离的单例池 |
| `IDatabaseConnection` | 连接抽象（prepare / bind / query / update） |
| `IResultSet` | 结果集抽象（按列名或列号读取） |
| `IConnectionFactory` / `MySQLFactory` | 连接工厂；当前只实现 MySQL |
| `PooledConnGuard` | RAII 守卫，析构时归还连接 |

### 1.1 单例模型

```cpp
ConnectionPool& pool = dcp::ConnectionPool::instance(DatabaseType::MySQL);
```

- 每种 `DatabaseType` 一个实例，存于静态 `unordered_map`，由互斥锁保护。
- `destroy_instance(type)` 会 `shutdown()` 并 `delete`。
- Oracle / PostgreSQL / SQLite / SQLServer 分支**没有工厂**，实例化后 `m_factory` 为空，`get_connection()` 会崩溃；当前项目只用 MySQL。

### 1.2 初始化与默认参数

```cpp
pool.init(cfg);                                  // 使用默认池参数
pool.init(cfg, min, max, idle_time, timeout, op_num);
```

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `min_size` | `std::thread::hardware_concurrency()` | 初始化时预建 |
| `max_size` | `2 * min_size` | 上限 |
| `max_idle_time` | `180` 秒 | 回收阈值 |
| `timeout` | `10` ms | 等待可用连接的最长时间 |
| `op_num` | `4` | 每次扩容创建数 |

项目实际通过 `AppConfig::database()` 传入 `pool_min/pool_max/max_idle_time/connection_timeout_ms/op_num`。

### 1.3 取还连接

`get_connection()`：

1. 未初始化 → 打印错误并返回空守卫；
2. 队列为空且 `current_size < max_size` → `expand_connections()`（一次最多 `op_num` 个）；
3. 仍为空 → `wait_for(timeout)`；
4. 超时且为空 → 返回空守卫（**不抛异常**，由 Repository 判空转 `DB_ERROR`）；
5. 成功 → 弹出队首，返回 `PooledConnGuard`。

`return_connection()`：

- 连接有效 → 放回队尾并 `notify_one`；
- 连接无效 → `current_size--` 并 `delete`；
- **MySQL 侧归还时不做健康检查**（健康检查器被注释）。

`shutdown()`：停止回收线程 → 清空队列并 `delete` 所有连接 → 计数归零。

### 1.4 回收线程

- 每 **30 秒**执行一次 `recycle_idle_connections()`。
- 保留至少 `min_size` 个；仅回收空闲时间 ≥ `max_idle_time` 的连接。
- 未超时的连接会被移到队尾，避免饿死。

### 1.5 已知缺陷

| 问题 | 影响 |
| --- | --- |
| `get_statistics()` 恒返回 `PoolStatistics{}` | 无法观测池状态（R13） |
| `MySQLHealthChecker` 被注释 | 归还时不校验连接可用性 |
| 非 MySQL 类型无工厂 | 使用即崩溃 |
| 扩容失败无回滚/日志 | 连接创建失败时计数仍可能增加 |
| `timeout` 默认 10ms | 高并发下容易超时，实际由配置 `connection_timeout_ms=3000` 覆盖 |

## 2. Redis 连接池 `redis-connection-pool`

命名空间 `rcp`，基于 redis-plus-plus。

### 2.1 单例与初始化

```cpp
rcp::RedisConnectionPool& pool = rcp::RedisConnectionPool::instance();
pool.init(cfg); // 或 init(cfg, min, max, idle_time, timeout, op_num)
```

| 参数 | 默认值 |
| --- | --- |
| `min_size` | `hardware_concurrency()` |
| `max_size` | `2 * min_size` |
| `max_idle_time` | `180` 秒 |
| `timeout` | **5000** ms |
| `op_num` | `4` |

配置来自 `[redis] host/port/db`；密码读取键是 `passward`（见 R6），不是 `password`。

### 2.2 取还连接

- `get_connection()`：空队列且未达上限 → 扩容；等待超时 → 记录 `total_timeout_errors` 并返回空守卫。
- 统计字段：`total_borrowed/released/created/destroyed/timeout_errors/reconnect_count`、`avg_wait_time_ms`、`max_wait_time_ms`、`pending_waiters`。
- `return_connection()`：先 `PING` 健康检查；
  - 健康 → 放回队列；
  - 不健康 → 销毁；若低于 `min_size` 立即补建一个；
  - 计入 `total_reconnect_count`。

### 2.3 回收与析构

- 回收线程同样每 30 秒运行，保留 `min_size`。
- 析构函数调用 `shutdown()`，停止线程并释放连接。

### 2.4 业务使用方式

Repository **不直接**持有 `RedisConnectionPool`，而是通过 `repository::ICacheConnectionProvider` 获取 RAII 守卫。新增缓存操作时：

1. 在 `ICacheConnection` 增加方法；
2. 在 `connection_provider.cpp` 实现；
3. 测试替身同步扩展（`tests/support/test_doubles.h`）。

## 3. INI 解析 `ini`

- 单例 `IniFile::instance()`；`load(path)` 会**覆盖**已加载内容。
- 数据结构：`Section = std::unordered_map<std::string, Value>`。
- 访问不存在的节会抛 `std::out_of_range`；`AppConfig` 用 `section_of()` 捕获并返回空节。
- `Value` 的布尔解析只认 `"true"`（大小写敏感），与 `AppConfig::bool_of` 的 `true/TRUE/1` 不同。**业务层不要直接依赖 `Value` 的 bool**。
- 支持 `#` / `;` 注释与节名 `[section]`；具体边界见 `README.md` 与 `src/ini.cpp`。

使用约束：

- 只在启动阶段加载；运行期不重新加载（`AppConfig` 是只读快照）。
- 新增配置节/键时，优先在 `AppConfig` 中封装为强类型字段，不要让业务直接读 `IniFile`。

## 4. 日志 `log`

- 单例 `Logger`，宏：`LOGD` / `LOGI` / `LOGW` / `LOGE` / `LOGF`。
- 同步写；单条日志使用 **4KB 栈缓冲**，超长会被截断。
- 按行轮转：达到 `max_lines` 后切换文件。
- `LogConfig`：`level`、`file`、`console`、`max_lines`。
- 启动时由 `AppConfig::logging()` 初始化。

约束：

- 禁止打印密码、token、密钥、完整 `password_hash`。
- 错误日志应包含可定位的上下文（`account_id`、`ban_type`、`operator_id`），但不要打印完整 SQL 参数。
- `LOGW` 用于可预期业务失败，`LOGE` 用于系统异常；当前 Service 大量使用 `LOGW` 记录封禁失败。

## 5. 配置 `config`

实现见 [architecture.md](./architecture.md) §5。此处补充实现细节：

- `expand_env()` 支持一个值内多个 `${}`；未闭合的 `${` 原样保留。
- `int_of()` 解析失败抛 `std::runtime_error`，启动直接失败。
- `bool_of()` 接受 `true/TRUE/1`，其余为 false。
- `load()` 先探测文件是否存在，不存在抛 `runtime_error("config file not found: ...")`。
- `validate()` 只校验：`server.port != 0`、`database.user/dbname` 非空、`pool_min>0 && pool_max>=pool_min`、用户 access/refresh 密钥非空、TTL > 0、`account_table/blacklist_table` 非空。
- **不校验**：管理员密钥、Redis 密码、`admin_account_table`、`refresh_token_table`。
- `refresh_token_table` 在 `load()` 中**没有被读取**，永远是默认 `refresh_tokens`。
- `JwtConfig::hash_secret` 直接在结构体中硬编码为 `"dfacadgafeqwfqvab"`，`load()` 不读取同名键（R20）。

## 6. JWT 签名器 `auth/token_signer`

- 算法固定 HS256；Base64URL 无 padding。
- Header：`{"alg":"HS256","typ":"JWT"}`。
- Payload：`iss/iat/exp/sub/role`。
- `sign(sub, role, ttl_seconds)` 返回 `SignedToken{token, expires_at}`。
- `verify(token)`：
  1. 必须恰好两段 `.`；
  2. 三段均非空；
  3. **先验签**（`CRYPTO_memcmp` 常量时间比较）；
  4. 校验 `alg == HS256`；
  5. 校验 `iss` 等于构造时的 issuer；
  6. `sub`、`role` 非空；
  7. `exp` 为数字且未过期；
  8. 返回 `TokenPayload{sub, issuer, role, issued_at, expires_at}`。
- 任何失败返回 `std::nullopt`，**不抛异常**。

约束：

- 用户/管理员、access/refresh 必须使用不同 `TokenSigner` 实例与不同密钥。
- 不要修改 claim 名称或算法；客户端与中间件都依赖现有结构。
- 新增 claim 必须保证旧 token 仍可验证（`verify` 只强制校验上述字段）。

## 7. 密码哈希 `security/password_hasher`

- 算法：**PBKDF2-HMAC-SHA512**，派生 32 字节，输出 64 位 hex；盐 32 位 hex。
- 默认迭代 **150000**。
- `PasswordHash{hash, salt, iterations}`。
- `hash_password(password, iterations = 150000)`。
- `verify_password(password, hash, salt, iterations)`：格式非法直接返回 `false`，不抛异常。

已知不一致：

- `docs/db_sqls.md` 注释写 PBKDF2-SHA256（R8），以代码 SHA512 为准。
- 修改算法会导致所有存量密码失效，**不得随意更改**；如需升级，必须做「登录时渐进重哈希」。

## 8. Base64URL `codec/base64url`

- 按 RFC 4648 §5，无 `=` padding。
- 提供 `base64_url_encode(input, out)` / `base64_url_decode(input, out)`，返回 `bool`。
- JWT 三段编码/解码唯一入口；不要在业务里自行实现。

## 9. 错误库 `error`（未接入）

- 提供 `Result<T, E>`、`Error`、错误码与 X-Macro 定义，示例完整。
- **当前业务代码未使用**，统一使用 `common::AppException` + `ErrorCode`。
- 新增代码不要混用两套错误模型；若未来迁移，必须整体替换并更新 [conventions.md](./conventions.md)。

## 10. RabbitMQ 连接池 `rabbitMQ-pool`（未集成）

- 已实现 `Broker`、`Connection`、`ChannelPool`、`Producer`、`Consumer`、`Message` 等，附 direct/fanout/topic/work 示例。
- **未加入根 `CMakeLists.txt`**，没有链接进 `sdu_app` 或 `sdu_forum_server`，业务代码未引用。
- 依赖 rabbitmq-c；启用前需要：
  1. 在根 CMake 增加 `add_subdirectory` 与链接；
  2. 增加配置节与 `AppConfig` 字段；
  3. 在 `AppContext` 初始化/关闭；
  4. 补充线程安全与重连策略测试。
- 在正式接入前，不要假设任何异步能力存在。

## 11. utils

| 函数 | 行为 |
| --- | --- |
| `parse_bearer(header, &err)` | 要求 `Bearer ` 前缀；返回去前缀并 `trim` 的 token；失败写 `BearerError` |
| `bearer_error_message(err)` | 返回对应中文/英文提示，用于 401 message |
| `trim(str)` | 去除首尾空白 |
| `hmac_sha256_base64url(secret, data)` | HMAC-SHA256 后 Base64URL，供哈希类场景使用 |
| `timestamp_to_string(ts)` | Unix 时间戳转可读字符串；实现用 `gmtime_r`，即 **UTC**，不要与 DB `NOW()`（本地时区）比较（R21） |

`BearerError`：`Missing` / `BadScheme` / `EmptyToken`。

约束：

- Controller 解析 Header 只用于 logout/refresh/logoff 的**显式 token**；
- 已认证接口的用户身份必须用 `middleware::current_public_id(req)`，不要重复解析。

## 12. 基础设施改动 checklist

- [ ] 是否影响 `AppContext` 的构造/析构顺序
- [ ] 是否需要新增配置项并加入 `validate()`
- [ ] 是否影响测试替身（`IDbConnection` / `ICacheConnection`）
- [ ] 是否破坏 JWT / 密码哈希的向后兼容
- [ ] 是否需要在 `docs/database.md` 记录存储变化
- [ ] 是否更新本文件与 [architecture.md](./architecture.md)
