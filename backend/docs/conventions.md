# 开发规范与硬约束

> 最后核对：2026-11-24  
> 本文件是代码评审的检查清单。标「必须」「禁止」「不得」的条目不可协商。

## 1. 分层职责

| 层 | 可以做 | 不可以做 |
| --- | --- | --- |
| `app` | 装配依赖、加载配置、注册中间件/路由、启停连接池 | 写业务分支、拼 SQL |
| `routes` | URL、HTTP 方法、中间件链绑定 | 参数校验、业务判断 |
| `controllers` | 解析 JSON/查询参数、校验、调用 Service、构造响应 | 直接访问 Repository/连接池；拼 SQL；自行拼错误 JSON |
| `services` | 业务编排、封禁检查、时间计算、抛 `AppException` | 构造 HTTP 响应；解析 Header |
| `repositories` | 单条 SQL、结果映射、异常转换 | 业务规则（如重复封禁判断）；拼 HTTP 错误 |
| `middleware` | 认证、角色、异常转换 | 业务逻辑；直接写数据库（应经 Service/Repository） |
| `common` | 无状态常量、枚举、响应、错误码 | 持有连接、读写文件、依赖 Drogon 请求对象 |
| `infrastructrue` | 通用能力 | 依赖 `services`/`controllers`（反向依赖） |

依赖方向只能向下：`app → routes → controllers → services → repositories → infrastructrue`。`common` 可被任意层引用。

## 2. 新增接口的标准流程

1. `routes/routes.cpp` 注册 handler，声明方法与中间件顺序（认证在前、角色在后）。
2. Controller：
   - `req->getJsonObject()` 为空 → `InvalidJson`；
   - 缺字段 → `InvalidJson("missing param: xxx")`；
   - 类型/范围/枚举错 → `InvalidParam`；
   - 身份从 `middleware::current_public_id(req)` / `current_admin_id(req)` 读取；
   - 成功用 `data_response` / `created_response` / `no_content_response`。
3. Service 编排，失败抛 `AppException`。
4. Repository 执行单条 SQL，`sql::SQLException` → `DB_ERROR`。
5. 补单测（Service 优先，Repository 用替身）。
6. 更新 [modules.md](./modules.md) 与 [README.md](./README.md) 状态。

## 3. 错误与异常

### 3.1 必须

- 业务失败抛 `common::AppException(ErrorCode, message, details?)`。
- 错误码只用 `common::ErrorCode` 中的枚举，**禁止**新增字符串字面量。
- `message` 面向客户端，简洁、不含 SQL、堆栈、内部路径。
- `details` 必须是 JSON 对象或 null。
- Repository 的连接获取失败抛 `DB_ERROR("database connection unavailable")`。

### 3.2 禁止

- `return` 自定义错误 JSON。
- 在 Controller 里 `try/catch` 后手写状态码（交给全局异常中间件）。
- 把 `std::exception::what()` 原样返回给客户端。
- 吞掉异常不记日志（`expire_outdated` 是已登记的例外）。

### 3.3 错误码选择速查

| 情况 | 错误码 |
| --- | --- |
| body 不是 JSON、缺字段 | `InvalidJson` |
| 字段类型错、范围错、枚举非法 | `InvalidParam` |
| 缺 token、token 无效/过期/已吊销 | `Unauthorized` |
| 用户名或密码错误 | `InvalidCredentials` |
| 登录命中封禁 | `AccountBanned` |
| 角色不匹配 | `Forbidden` |
| 账号/封禁记录不存在 | `NotFound` |
| 重复封禁 | `Conflict` |
| DB/Redis 故障 | `DatabaseError` / `RedisError` |
| 未预期异常 | `InternalError` |

## 4. 响应格式

必须使用 `common/api_response.h`：

```cpp
callback(common::data_response(200, data));
callback(common::created_response(data));   // 201
callback(common::no_content_response());    // 204
```

- 成功响应不得把业务字段放在顶层，必须包在 `data` 内。
- 204 不得带 body。
- 字段命名为 `snake_case`（现有契约），禁止改成 camelCase。
- 金额/ID 等大整数使用 `Json::UInt64` / `Json::Int64`，避免精度丢失。

## 5. 鉴权与身份

1. 受保护接口必须挂认证中间件 + 对应角色中间件。
2. 用户身份只能通过 `middleware::current_public_id(req)` 获取。
3. 管理员身份只能通过 `middleware::current_admin_id(req)` 获取。
4. 不要在 Controller 重新解析 `Authorization`，除非该接口本身就是 token 操作（logout/refresh/logoff）。
5. 中间件属性键固定：`public_id`、`admin_id`、`role`。**不得重命名**。
6. `All` 封禁由认证中间件统一处理；业务层只检查具体维度。
7. 中间件中任何已经写出响应的分支，**必须立即 `return`**（当前存在缺失，见 R1/R1b）。

## 6. Token 与密码

- access / refresh / admin access / admin refresh 四套密钥不得混用。
- token 只存哈希，不存明文；哈希统一用 `utils::hmac_sha256_base64url(TokenRepository::hash_secret(), token)`。
- 吊销检查必须在验签之前。
- 密码只存 PBKDF2 派生值 + 盐 + 迭代次数；禁止存明文或可逆加密。
- 禁止在日志、响应、异常 message 中出现 password/token/secret。
- 修改哈希算法必须提供渐进迁移方案，不能直接全量失效。
- 吊销哈希密钥 `JwtConfig::hash_secret` 目前**硬编码**，不从 INI 读取；不要假设改配置就能轮换（R20）。

## 7. 封禁相关

1. 枚举 `BanType` / `ReasonCode` **只增不改**；废弃项保留数值并标记 `deprecated`。
2. 生效判定统一 `status=1 AND expire_at > NOW()`，禁止只看 `status`。
3. 永久封禁使用 `PERMANENT_EXPIRE`，不要写 NULL。
4. 新增原因码必须同时更新 `FindReasonMeta` 映射表，否则 `IsReasonCodeValid` 判为非法。
5. 4xxx / `auto_only` 原因必须 `source ∈ {auto, report}`。
6. 自定义原因码必须提供非空 `reason`。
7. 幂等判断在 Service 层完成，Repository 不做业务判重。
8. 不要在业务层重复判断 `All`。
9. `banned_by` / `unbanned_by` 存管理员 `admin_id` 字符串（`CHAR(8)`），不是管理员表 `id`；超过 8 字符会写入失败。
10. 解除封禁只处理「生效中」记录（`status=1 AND expire_at > NOW()`）；`All` 表示解除全部维度。

## 8. 仓储与 SQL

- Repository 构造函数接收表名，SQL 中拼接的是**配置值**，不是用户输入。
- 所有值用 `?` 绑定；禁止把参数拼进 SQL。
- 连接必须通过 `IDbConnectionProvider` / `ICacheConnectionProvider` 获取，用 RAII 守卫。
- 取不到连接返回空守卫时，Repository 必须抛 `DB_ERROR`。
- 查询未命中返回 `nullopt`/空容器；是否 404 由 Service 决定。
- 时间字符串统一 `YYYY-MM-DD HH:MM:SS`；与 `NOW()` 比较的值必须用本地时区生成，UTC 时间只用于审计展示（R21）。
- 修改 `SELECT *` 时改为显式列名，避免表结构变化导致映射错位。

## 9. 配置

- 新增配置项必须同时改：`config-dev.ini`、`AppConfig` 字段、`load()`、`validate()`、本文档/architecture 配置表。
- 必填项必须在 `validate()` 中校验，启动即失败优于运行期空值。
- 敏感值支持 `${ENV:-default}`，生产必须用环境变量覆盖默认值。
- 表名、TTL、池大小等不得在代码中硬编码。
- 注意现有键名缺陷：Redis 密码键是 `passward`；`refresh_token_table` 未从 INI 读取；admin 密钥未配置。

## 10. 日志

- 使用 `LOGD/LOGI/LOGW/LOGE`，不要使用 `std::cout`（连接池内部历史代码除外）。
- 业务可预期失败用 `LOGW`，系统异常用 `LOGE`。
- 日志必须能定位对象：至少包含 `account_id` / `admin_id` / `ban_type` 之一。
- 禁止打印密码、token、密钥、完整哈希、完整请求体。
- 高频路径避免打印大对象；必要时先 `if (logger.level() <= DEBUG)`（按现有日志库能力）。

## 11. 命名与文件

- 命名空间：`app`、`routes`、`controller`、`admin::controller`、`service`、`admin::service`、`repository`、`middleware`、`ban`、`common`、`infra`、`utils`、`dcp`、`rcp`。
- 文件名 `snake_case`；类名 `PascalCase`；函数/变量 `snake_case`；常量 `kPascalCase` 或 `UPPER_SNAKE`（按所在目录既有风格）。
- 头文件用 `#pragma once`。
- 新增 `.h` 或修改函数接口时，按项目既有注释风格补充文件头与 `@brief/@param/@throws`。
- 格式化使用仓库根 `.clang-format`；提交前运行 `clang-format`。

## 12. 禁止事项清单（评审直接打回）

1. 在 Controller 中写 SQL 或访问连接池。
2. 跳过中间件直接信任请求头中的用户 ID。
3. 新增一套与 `AppException` 并行的错误返回机制。
4. 在 `common` 中引入 Drogon 请求/响应类型。
5. 把 `All` 封禁判断复制到各业务 Service。
6. 修改 `BanType` / `ReasonCode` 已有数值。
7. 用 `status` 单独判断封禁是否生效。
8. 硬编码表名、密钥、TTL。
9. 在日志中输出敏感信息。
10. 未配置 `validate()` 就新增必填配置。
11. 让单元测试连接真实 MySQL/Redis/网络。
12. 删除或重命名 `public_id` / `admin_id` / `role` 中间件属性。
13. 在中间件已写响应后继续执行（缺 `return`）。
14. 直接修改 `docs/db_sqls.md` 而不更新 [database.md](./database.md)。
15. 让 `admin_id` / `banned_by` / `unbanned_by` 超过 8 字符（`CHAR(8)`）。
16. 假设 `hash_secret` 可以通过 INI 配置（当前硬编码，R20）。
17. 用 UTC 时间与 DB `NOW()` 比较（R21）。

## 13. 历史核心设计（不得破坏）

| 设计 | 现状 | 为什么不能改 |
| --- | --- | --- |
| `All` 封禁在认证中间件拦截 | 已实现 | 保证下一次请求立即生效，避免每个业务重复判断 |
| 具体维度在业务动作前判断 | 接口预留 | 避免 All 与单维度的联合判断复杂度 |
| 过期判定用 `expire_at > NOW()` | 已实现 | 查询时无需先改状态，天然正确 |
| `expire_outdated()` 仅做兜底 | 已实现、未定时调度 | 不能把状态维护放到读路径 |
| refresh 存 MySQL、access 存 Redis | 已实现 | refresh 生命周期长需持久化；access 短命适合缓存 |
| 先查吊销再验签 | 已实现 | 覆盖「签名有效但已登出」 |
| Provider 接口隔离连接池 | 已实现 | 单测替身依赖该边界 |
| 模块只注册路由 | 已实现 | 依赖装配集中在 `AppContext` |
| 管理员独立表与独立密钥 | 部分实现 | 避免用户体系提权；当前密钥配置缺失待修 |

## 14. 测试规范（摘要）

详见 [testing.md](./testing.md)：

- 框架 doctest，CTest 标签 `fast;unit`。
- 不得访问真实 MySQL/Redis/网络。
- 测试替身五类：Dummy / Stub / Spy / Mock / Fake。
- 异常断言必须校验 `AppException::code()`，不只校验是否抛出。
- Mock 必须在用例结束调用 `verify()`。
- 新增 Service 公共方法必须有成功路径 + 至少一个失败路径。

## 15. 提交前 checklist

- [ ] 代码通过编译，`ctest` 全绿
- [ ] 新增/修改接口已更新 [modules.md](./modules.md)
- [ ] 全局行为变化已更新 [architecture.md](./architecture.md)
- [ ] 表结构变化已更新 [database.md](./database.md) 与 [db_sqls.md](./db_sqls.md)
- [ ] 新增配置已更新 [architecture.md](./architecture.md) §5 与 `validate()`
- [ ] 已检查没有引入新的「已写响应但未 return」分支
- [ ] 已确认没有把未完成函数描述为已完成
- [ ] 已在 [README.md](./README.md) §4 或 [modules.md](./modules.md) §4.7 登记新发现的缺陷或临时实现
