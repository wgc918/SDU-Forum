# SDU-Forum 后端文档中心


> 最后与代码核对日期：2026-11-24
> 适用代码：`backend/` 当前工作区（C++17 + Drogon）
> 文档语言：中文


本文档集用于回答三个问题：


1. 现在**已经实现**了什么，实际行为是什么；
2. 新增功能时**应该改哪里**，哪些东西**不能动**；
3. 当前有哪些**未完成、临时实现或已知缺陷**。


---


## 0. 权威性声明


- **代码是唯一事实来源**。本文档与代码不一致时，以代码为准，并应立即修正文档。
- 文档中标注「已确认」的内容均来自对当前代码的直接阅读；标注「据代码推断」的内容表示代码依赖该行为但仓库内缺少完整定义。
- 所有「必须」「禁止」「不得」条目均为**硬约束**，违反会破坏现有鉴权、封禁或响应契约。
- 本目录是新增功能、Code Review、线上排障的第一入口；不要另起一份平行文档。


## 1. 文档地图


| 文件 | 内容 | 什么时候看 |
| --- | --- | --- |
| [README.md](./README.md) | 导航、项目状态、高风险清单、维护约定、接入 checklist | 第一次接触项目；准备提 MR |
| [architecture.md](./architecture.md) | 技术栈、分层、启动流程、配置、统一响应、错误码、JWT、Token 吊销、封禁体系、跨模块机制 | 改全局行为、加中间件、排查鉴权/封禁 |
| [modules.md](./modules.md) | 用户认证、管理员认证、管理员封禁管理、仓储层的 API 与实现细节；新模块模板 | 改具体业务接口 |
| [infrastructure.md](./infrastructure.md) | MySQL/Redis 连接池、INI、日志、配置、JWT 签名器、密码哈希、base64url、error 库、RabbitMQ、utils | 改基础设施或引入新依赖 |
| [database.md](./database.md) | 表结构、字段语义、状态值、时间约定、索引、历史差异、迁移规范 | 加表/加字段/写 SQL |
| [conventions.md](./conventions.md) | 分层职责、命名、异常与响应规范、仓储规范、配置规范、禁止事项 | 写代码前必读 |
| [testing.md](./testing.md) | doctest/CTest、测试替身、用例约定、已覆盖范围、待补测试 | 写或改单元测试 |
| [tests/docs/tests.md](../tests/docs/tests.md) | 逐用例目录、替身引用矩阵、错误映射、CMake 模板 | 改具体测试用例 |
| [db_sqls.md](./db_sqls.md) | 4 张表的可执行建表 SQL 与常用语句（与 database.md 双向同步） | 初始化数据库 |
| [sql_reference.md](./sql_reference.md) | 逐表 DDL、字段语义、业务 SQL、代码映射、历史差异存档 | 查某条 SQL 的具体实现与索引影响 |


## 2. 快速定位：我要做什么


| 需求 | 主要改动位置 | 必读 | 关键提醒 |
| --- | --- | --- | --- |
| 新增一个 HTTP 接口 | `routes/routes.cpp` → `controllers/` → `services/` → `repositories/` | [conventions.md](./conventions.md) 第 2 节 | Controller 只做协议校验；业务异常用 `AppException` |
| 新增一个业务模块 | `app/modules/` 新建 `xxx_module.{h,cpp}`，在 `AppContext` 装配 | [modules.md](./modules.md) §5 模板、§6 登记表 | 模块只负责注册路由，不持有连接池 |
| 改登录/注册逻辑 | `services/auth_service.cpp`、`controllers/auth_controller.cpp` | [modules.md](./modules.md) §1 | 不要绕过 `BanService` 的封禁检查 |
| 改 Token 规则 | `infrastructrue/auth/token_signer.*`、`infrastructrue/config/app_config.*` | [architecture.md](./architecture.md) §8 | access/refresh 密钥必须分离；改 claim 会破坏兼容 |
| 新增封禁维度 | `common/ban_enums.h`（只增不改） | [architecture.md](./architecture.md) §9.1 | 同步 `FindBanTypeMeta`、`IsBanTypeValid`、前端文案 |
| 新增封禁原因 | `common/ban_enums.h` 的 `ReasonCode` 与映射表 | [architecture.md](./architecture.md) §9.2 | 4xxx 仅允许 auto/report 来源 |
| 新增数据表/字段 | `docs/db_sqls.md`、`docs/database.md`、对应 Repository | [database.md](./database.md) | 表名必须来自配置，不得硬编码 |
| 新增中间件 | `middleware/` + `AppContext::register_middlewares` + 路由注册 | [architecture.md](./architecture.md) §6 | 回调分支必须 `return`，否则会双重回调 |
| 调整配置项 | `config/config-dev.ini`、`infrastructrue/config/app_config.{h,cpp}`、`validate()` | [architecture.md](./architecture.md) §5 | 新必填项必须加入 `validate()` |
| 写单元测试 | `tests/unit/`、`tests/support/test_doubles.h` | [testing.md](./testing.md) | 不得连接真实 MySQL/Redis/网络 |


## 3. 项目状态看板


> 状态含义：✅ 已实现并有测试/可运行；⚠️ 部分实现或存在阻断性缺陷；❌ 未开始。


| 模块 | 状态 | 说明 |
| --- | --- | --- |
| 用户注册 / 登录 / 刷新 / 登出 / 注销 | ✅ | 接口链路完整；`AuthService` 有 34 个单测 |
| 管理员登录 / 刷新 / 登出 | ⚠️ | 链路完整，但 `admin_access_secret`/`admin_refresh_secret` 未配置、以空密钥签名（R5），上线前必须修复 |
| 封禁写入（ban / unban） | ✅ | 业务校验、幂等、过期时间计算完整 |
| 封禁查询（history / blacklist） | ⚠️ | Controller 与路由已接好，但 Service/Repository 查询函数**未返回数据**，接口实际不可用（见风险 R2/R3） |
| 封禁自动过期兜底 | ⚠️ | `BlacklistRepository::expire_outdated()` 已实现，**尚未接入任何定时任务**（R18） |
| MySQL / Redis 连接池 | ✅ | 可运行；MySQL 健康检查与统计未完成 |
| 配置 / INI / 日志 / JWT / 密码哈希 | ✅ | 可运行；配置有若干键名缺陷（见风险 R5/R6） |
| error 库（`infrastructrue/error`） | ⚠️ | 完整实现但业务代码未使用，实际使用 `AppException` |
| RabbitMQ 连接池 | ⚠️ | 代码与示例存在，**未加入根 CMakeLists**，未集成 |
| 帖子 / 评论 / 私信 / 举报 / 上传 / 用户资料 | ❌ | 尚未开始；封禁维度已为其预留 |
| 定时任务 / 消息队列消费 | ❌ | 未开始 |


## 4. 当前已知高风险问题（先看这个）


下列问题均已通过阅读当前代码确认。修复前**不要**把对应能力描述为「已完成」。


| 编号 | 位置 | 问题 | 影响 |
| --- | --- | --- | --- |
| R1 | `middleware/auth_middleware.cpp` | 封禁分支调用 `mcb(...)` 后**缺少 `return`** | 会继续执行 `next_cb`，同一请求可能被处理两次 |
| R1b | `middleware/role_middleware.cpp` | 空角色 / 角色不匹配分支 `mcb(...)` 后**缺少 `return`** | 越权请求可能继续进入 Controller |
| R2 | `services/admin/user_service.cpp` | `get_ban_history`、`list_blacklist`、`search_blacklist_by_reason`、`search_blacklist_by_type`、`get_user_status` **函数体无 `return`**；`list_blacklist` 注释里的 `list_active` 在 Repository 中并不存在 | 返回路径未定义（UB），查询接口不可用 |
| R3 | `repositories/blacklist_repository.cpp` | `get_history()`、`search()` 查完数据后 `return {}`，丢弃 `entries` | 即使修复 R2 也查不到数据 |
| R3b | `repositories/blacklist_repository.cpp` `search` | 条件使用 `if (filter.account_id)`，绑定使用 `if (filter.account_id != 0)`；`nullopt` 时后者为真并解引用空 optional | 空 optional 解引用 → 崩溃 / UB |
| R4 | `controllers/admin/user_controller.cpp` `ban` | `source` 缺省时 `value_or("")` 传给 `string_to_ban_source` 会抛 `InvalidParam` | `source` 实际为**必填**，与注释「缺省 manual」不一致 |
| R5 | `config/config-dev.ini` | `[auth]` 缺少 `admin_access_secret`、`admin_refresh_secret`；`validate()` 也不校验 | 管理员 Token 使用空密钥签名，存在安全风险 |
| R5b | `config/config-dev.ini` | `access_jwt_secret` / `refresh_jwt_secret` 各重复两次，第二组疑似想写 admin 密钥 | 后值覆盖前值，用户与「预期 admin」密钥混用 |
| R6 | `infrastructrue/config/app_config.cpp` | Redis 密码读取键为 `"passward"`（拼写错误） | 标准键 `password` 永远读不到 |
| R6b | `infrastructrue/config/app_config.cpp` | `AuthConfig::refresh_token_table` 从未从 INI 读取 | 改配置无效，只能改代码 |
| R7 | `common/errors.h` | `RedisError` 的名称字符串为 `" REDIS_ERROR"`（前导空格） | 客户端按错误码匹配会失败 |
| R8 | `docs/db_sqls.md` | `password_hash` 注释写 PBKDF2-SHA256，实现为 **PBKDF2-HMAC-SHA512** | 文档误导；本版已修正注释 |
| R9 | `repositories/admin_account_repository.cpp` | `inisert_admin_account` 拼写错误；`INSERT` 未绑定 `iterations` | 依赖 DB 默认值 150000，自定义迭代数会丢失 |
| R10 | `blacklist` 表 vs `BanMeta` | `banned_by`/`unbanned_by` 为 `CHAR(8)`，存 `admin_id`；代码对 `operator_id` 无长度校验 | 管理员 ID 超过 8 字符时写入失败/截断（当前 Controller 限制 8，暂不触发） |
| R11 | `services/auth_service.cpp` `refresh_token` | 服务层只检查 `BanType::Login`，不检查 `All` | 正常 HTTP 路由由 `RefreshAuthMiddleware` 兜底 `All`；绕过中间件直调 Service 则缺少全站封禁校验 |
| R12 | `docs/db_sqls.md` | 历史版本缺少 `refresh_tokens`、`admin_account` 建表 SQL | 本版已补齐 |
| R13 | `infrastructrue/database-connection-pool` | `get_statistics()` 恒返回空结构；健康检查被注释 | 无法观测 MySQL 池状态 |
| R14 | `services/auth_service.cpp` `logout`/`logoff` | 先查 refresh 是否吊销再验签；Redis/DB 异常按数据库错误处理 | 依赖 Redis 可用性，异常语义偏重 |
| R15 | `services/auth_service.cpp` `refresh_token` | 验签后未判断 `account.has_value()` 就使用 `account->id` | refresh 有效但账号已注销时解引用空 optional |
| R16 | `services/auth_service.cpp` `logoff` | `delete_account || revoke_refresh || revoke_access` 短路 | 删除失败时不会吊销 token（符合现状设计，但需明确） |
| R17 | `services/admin/user_service.cpp` `resolve_reason_text` | 自定义原因码且 `reason` 为空时用兜底文案填充 | 绕过 `CheckReasonTextConsistency`，空自定义原因不会报错 |
| R18 | `repositories/blacklist_repository.cpp` `expire_outdated` | 已实现但**未接入任何定时任务/启动钩子** | 过期记录只能靠查询条件兜底，`status` 不会自动变 2 |
| R19 | `middleware/auth_middleware.cpp` | `find_account_by_public_id` 后未判断 `has_value()` 就使用 `account->id` | token 有效但账号已删除时解引用空 optional |
| R20 | `infrastructrue/config/app_config.h` | `JwtConfig::hash_secret` 硬编码为 `"dfacadgafeqwfqvab"`，不从配置读取 | 所有环境共用同一吊销哈希密钥；泄露后可伪造吊销键 |
| R21 | `utils/utils.cpp` vs `services/admin/user_service.cpp` | `timestamp_to_string` 用 UTC（`gmtime_r`），封禁过期时间用本地时区（`localtime_r`） | 两类时间混存；拿 `refresh_tokens.expires_at` 与 `NOW()` 比较会偏移 |
| R22 | `docs/db_sqls.md` / `admin_account` | `admin_id` 没有唯一索引 | 可能出现重复管理员账号，登录取第一条 |
| R23 | `app/modules/admin/auth_module.cpp` | 直接依赖 `PooledDbConnectionProvider` 具体类型而非 `IDbConnectionProvider` | 管理员认证模块无法用接口替身做单测 |
| R24 | `services/admin/user_service.cpp` | `get_ban_history` 返回 `vector`，Repository 返回 `PageResult`，签名不匹配 | 修复 R2/R3 时必须统一返回类型（Service 取 `.item`） |


## 5. 维护约定


1. **改代码必须同 PR 改文档**：行为、接口、配置、表结构、错误码任一变化，都要更新对应文档。
2. **新增模块必须在 [modules.md](./modules.md) 增加章节**：先在 §6 登记，再使用 §5 模板补全；不要另起文档。
3. **跨模块机制变化必须更新 [architecture.md](./architecture.md)**，尤其是鉴权、Token、封禁、响应格式。
4. **表结构变化同时更新 [db_sqls.md](./db_sqls.md) 与 [database.md](./database.md)**。
5. **新增/修改错误码必须同步 [architecture.md](./architecture.md) §7.2 的表格**。
6. 文档内引用代码使用相对路径（如 `../services/auth_service.cpp`），便于在仓库内跳转。
7. 已知缺陷修复后，在本文件 §4 与 [README 状态看板](#3-%E9%A1%B9%E7%9B%AE%E7%8A%B6%E6%80%81%E7%9C%8B%E6%9D%BF)同步移除或标记「已修复（日期）」。
8. 不要删除历史章节；被推翻的设计用「已废弃」标注并说明原因，保留决策背景。


## 6. 新增功能接入 checklist


复制到 MR 描述中逐项勾选：


- 在 `app/modules/` 新增模块类（实现 `app::Module`），并在 `AppContext` 构造、`register_routes` 中注册
- 路由集中在 `routes/routes.cpp`，标明 HTTP 方法与所需中间件
- Controller 完成 JSON / 查询参数校验，缺失字段用 `InvalidJson`，类型/范围错误用 `InvalidParam`
- Service 编排业务，所有可预期失败抛 `common::AppException`
- Repository 只做单条 SQL 与结果映射，捕获 `sql::SQLException` 转 `DB_ERROR`
- 新增错误码时同步 `ErrorCode`、`error_code_name`、`http_status` 与文档表格
- 需要登录的接口挂 `AccessAuthMiddleware`/`AdminAccessAuthMiddleware` + 对应 Role 中间件
- 业务动作前检查对应的 `BanType`（`All` 已由鉴权中间件兜底）
- 新增配置项写入 `config-dev.ini`，并在 `AppConfig::load` 与 `validate` 中处理
- 新增表/字段写入 `db_sqls.md` 与 `database.md`，表名走配置
- 为 Service / Repository 增加 doctest 单测（使用 `tests/support/test_doubles.h`）
- 更新 [modules.md](./modules.md) 对应章节；新发现的临时实现同时登记到本文件 §4 与 [modules.md](./modules.md) §4.7


## 7. 后续功能扩展登记（预留）


> 新功能开始实现前先在此登记一行，并在 [modules.md](./modules.md) §6 补全详情，避免多人并行时重复实现已有机制。
> 状态使用：未开始 / 设计中 / 开发中 / 已实现 / 已废弃。


| 编号 | 功能 | 状态 | 主要改动位置 | 必须复用的现有机制 | 备注 |
| --- | --- | --- | --- | --- | --- |
| E01 | 帖子（发布/编辑/删除/列表） | 未开始 | `controllers/post/`、`services/post/`、`repositories/post_repository` | `BanService::is_banned(id, Post)`、`AccessAuthMiddleware`、统一响应/分页 | 新增表需登记 database.md |
| E02 | 评论 | 未开始 | 同上 | `BanService::is_banned(id, Comment)` | 举报维度 `Report` 已预留 |
| E03 | 私信 | 未开始 | `controllers/message/` | `BanService::is_banned(id, Message)` | 实时通道未选型 |
| E04 | 举报/工单 | 未开始 | `services/admin/` | `ban.source=report`、`blacklist.ref_id` 关联工单 | 4xxx 原因码只允许 auto/report |
| E05 | 用户资料 | 未开始 | `controllers/user/` | `AccountRepository`、`public_id` 对外标识 | 不要暴露内部 `id` |
| E06 | 文件上传 | 未开始 | 待定 | 统一错误码与鉴权中间件 | 需要新增基础设施章节 |
| E07 | 封禁过期定时任务 | 未开始 | `app/app.cpp` 或独立 worker | `BlacklistRepository::expire_outdated()` | 修复 R18，需支持多实例互斥 |
| E08 | 管理员账号管理接口 | 未开始 | `controllers/admin/` | `AdminRoleMiddle`、`AdminAccountRepository` | 需先补 `admin_id` 唯一索引（R22） |
|  |  |  |  |  |  |


### 7.1 已预留、但尚未使用的扩展点


| 扩展点 | 现状 | 使用方式 |
| --- | --- | --- |
| `BanType` 的 `Comment/Post/Message/Report` | 枚举与元数据已定义，无业务代码 | 新模块写操作前调用 `admin::service::BanService::is_banned(id, 对应维度)` |
| `ReasonCode` 4xxx（`ILLEGAL_CONTENT` 起） | 仅允许 `auto/report` | 自动审核/举报流程使用；人工 ban 会被拒绝 |
| `blacklist.source/ref_id` | 已落库，无查询接口 | 举报工单关联时写入 |
| `BlacklistRepository::expire_outdated()` | 已实现，无调用方 | 定时任务周期调用 |
| `app::Module` 接口 | 3 个模块已实现 | 新模块继承并在 `AppContext` 装配 |
| `infrastructrue/error` | 完整但未使用 | 若迁移需全局统一，禁止与 `AppException` 混用 |
| RabbitMQ 连接池 | 未加入构建 | 接入前先补 `CMakeLists` 与配置校验 |
