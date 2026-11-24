# 业务模块详解

> 适用代码：`backend/`　最后核对：2026-11-24  
> 全局机制（响应、错误、鉴权、封禁）见 [architecture.md](./architecture.md)。  
> 每个模块按统一结构记录：职责 → 文件 → 路由 → API → 业务逻辑 → 实现细节 → 边界 → 已知问题 → 扩展点。

## 模块总览

| 模块 | 前缀 | 状态 | 章节 |
| --- | --- | --- | --- |
| 用户认证 | `/sdu_forum/api/v1/auth` | ✅ 可用 | [§1](#1-用户认证模块) |
| 管理员认证 | `/admin/api/v1/auth` | ✅ 可用（密钥配置有缺陷） | [§2](#2-管理员认证模块) |
| 管理员用户封禁 | `/admin/api/v1/users` | ⚠️ 写入可用，查询未闭环 | [§3](#3-管理员用户封禁模块) |
| 仓储层 | — | ✅ 写路径可用，查询有缺陷 | [§4](#4-仓储层) |
| 待新增模块 | — | — | [§5 模板](#5-新模块接入模板) |

---

## 1. 用户认证模块

### 1.1 职责

普通用户的注册、登录、刷新 Token、登出、注销账号。**不负责**管理员身份。

### 1.2 涉及文件

| 层 | 文件 |
| --- | --- |
| 路由 | `routes/routes.cpp`（`register_auth_routes`） |
| Controller | `controllers/auth_controller.{h,cpp}` |
| Service | `services/auth_service.{h,cpp}` |
| Repository | `repositories/account_repository.*`、`repositories/token_repository.*` |
| 依赖 | `BanService`、`TokenSigner`、`PasswordHasher`、`JwtConfig` |
| 模块装配 | `app/modules/auth_module.{h,cpp}` |

### 1.3 路由与中间件

| 方法 | 路径 | 中间件 |
| --- | --- | --- |
| POST | `/sdu_forum/api/v1/auth/login` | 无 |
| POST | `/sdu_forum/api/v1/auth/sign-up` | 无 |
| POST | `/sdu_forum/api/v1/auth/logout` | `AccessAuthMiddleware` + `UserRoleMiddleware` |
| POST | `/sdu_forum/api/v1/auth/refresh-token` | `RefreshAuthMiddleware` + `UserRoleMiddleware` |
| DELETE | `/sdu_forum/api/v1/auth/logoff` | `AccessAuthMiddleware` + `UserRoleMiddleware` |

### 1.4 API 定义

#### 1.4.1 登录 `POST /sdu_forum/api/v1/auth/login`

请求体：

```json
{ "user_id": "202400000001", "password": "******" }
```

- `user_id`、`password` 必须存在且为字符串；`user_id` 会 `trim`。
- 长度上限：`user_id ≤ 64`，`password ≤ 128`。

成功 `200`：

```json
{
  "data": {
    "public_id": "usr_xxxxxxxxxxxxxxxxxxxx",
    "token_type": "Bearer",
    "access_token": "...",
    "access_token_exp": 1730003600,
    "refresh_token": "...",
    "refresh_token_exp": 1730608400
  }
}
```

失败：

| 场景 | code | HTTP |
| --- | --- | --- |
| body 非 JSON / 缺字段 | `INVALID_JSON` | 400 |
| 字段类型错、空值、超长 | `INVALID_PARAM` | 400 |
| 账号不存在或密码错误 | `INVALID_CREDENTIALS` | 401 |
| 命中 `All` 或 `Login` 封禁 | `ACCOUNT_BANNED` | 403 |
| DB 异常 | `DB_ERROR` | 500 |

业务顺序（`AuthService::login`）：

1. 按 `user_id` 查账号（含 `id/public_id/password_hash/salt/iterations`）；
2. 校验 PBKDF2 密码；
3. 查 `BanType::All` 封禁 → 命中抛 `ACCOUNT_BANNED`；
4. 查 `BanType::Login` 封禁 → 命中抛 `ACCOUNT_BANNED`；
5. 用 `sub=public_id, role=sduforum_user` 签发 access + refresh；
6. 返回 token 与过期时间戳。

> 注意：封禁检查在**密码校验之后**，避免通过封禁错误码探测账号是否存在。

#### 1.4.2 注册 `POST /sdu_forum/api/v1/auth/sign-up`

请求体同登录。成功 `201`：

```json
{ "data": { "public_id": "usr_xxxxxxxxxxxxxxxxxxxx" } }
```

实现细节：

- `public_id = "usr_" + 20 位 Base62`，使用拒绝采样避免取模偏差。
- 密码用 PBKDF2 默认 **150000** 次迭代；哈希 64 hex，盐 32 hex。
- 先 `find_account_by_user_id` 查重，重复直接抛 `INVALID_PARAM`（**不是 409**，历史实现）。
- 再执行 `INSERT`；插入失败抛 `DB_ERROR`。
- 不自动登录，不返回 token。

#### 1.4.3 登出 `POST /sdu_forum/api/v1/auth/logout`

- Header：`Authorization: Bearer <access_token>`
- Body：`{ "refresh_token": "..." }`
- 成功：`204`，无 body。

`AuthService::logout`：

1. 用 refresh 哈希查吊销记录，已存在 → `UNAUTHORIZED`；
2. 验签 refresh（含 `iss/role/exp`）；
3. 旧 refresh 哈希写入 `refresh_tokens`（带过期时间）；
4. 旧 access 哈希写入 Redis（TTL = access TTL）；
5. 任一步失败 → `DB_ERROR`。

> 顺序说明：先查吊销再验签，意味着 Redis/DB 异常会以数据库错误返回，而不是「token 无效」。

#### 1.4.4 刷新 `POST /sdu_forum/api/v1/auth/refresh-token`

- Header：`Authorization: Bearer <refresh_token>`
- Body：

```json
{ "refreshing": true, "access_token": "<旧 access token>" }
```

- `refreshing` 必须为 bool，`access_token` 必须为字符串（可为空串，服务端仍会尝试吊销）。
- 成功 `200`：

```json
{
  "data": {
    "public_id": "usr_...",
    "refresh_token": "...",
    "refresh_token_exp": 1730608400,
    "access_token": "...",
    "access_token_exp": 1730003600
  }
}
```

注意：响应**没有** `token_type`（与登录不一致，客户端不要强依赖）。

`AuthService::refresh_token`：

1. 用 refresh signer 验签；
2. 按 `public_id` 查账号，不存在 → `UNAUTHORIZED`；
3. 只检查 `BanType::Login` 封禁；
4. `refreshing=true`：签发新 refresh，并把**旧 refresh** 哈希入库（exp 使用新 refresh 的过期时间）；
5. 始终签发新 access，并把传入的旧 access 哈希写 Redis（写失败忽略）；
6. 返回 `LoginResult`。

边界与已知问题：

- 服务层只查 `Login` 封禁，**不查 `All`**。HTTP 路由由 `RefreshAuthMiddleware` 兜底 `All`；若绕过中间件直接调用 Service，则缺少 `All` 校验（风险 R11）。
- `refreshing=false` 时**不签发新 refresh**，`LoginResult` 中 `refresh_token` 为空串、`refresh_token_exp` 为 0；响应仍带这两个字段，客户端必须判断。
- 旧 refresh / 旧 access 的写库/写缓存返回值均被忽略，吊销失败不会报错。
- 验签通过后直接使用 `account->id`，**未判断 `account.has_value()`**；账号已注销但 refresh 仍有效时会解引用空 optional（风险 R15）。

#### 1.4.5 注销 `DELETE /sdu_forum/api/v1/auth/logoff`

- Header：`Authorization: Bearer <access_token>`
- Body：`{ "refresh_token": "..." }`
- 成功：`204`。

`AuthService::logoff`：

1. refresh 已吊销 → `UNAUTHORIZED`；验签；
2. 按 `public_id` 删除账号；
3. 吊销 access + refresh；
4. 删除失败立即短路（不再吊销）。

> 由于 `blacklist` 对 `account` 有 `ON DELETE CASCADE`，注销会级联删除封禁历史。

### 1.5 边界与约束

- `user_id` 只做长度与空值校验，**不校验必须 12 位数字**（DB 注释与实现不一致）。
- Controller 允许 `user_id ≤ 64`，但表字段是 `CHAR(12)`；超过 12 位在 MySQL 严格模式下会插入失败并转成 `DB_ERROR`。
- 密码不做复杂度校验，只限长度。
- `public_id` 是唯一对外身份，接口不得返回内部自增 `id`。
- 登录失败不区分「账号不存在」与「密码错误」，统一 `INVALID_CREDENTIALS`。

### 1.6 扩展点

- 增加邮箱/手机号登录：在 `AuthService::login` 前增加账号解析，不要改 Controller 契约。
- 增加登录失败限流：应在 Controller 之前以中间件实现，避免污染 Service。
- 增加封禁类型检查：只加需要的具体维度，不要重复判断 `All`。

---

## 2. 管理员认证模块

### 2.1 职责

管理员登录、刷新、登出。管理员使用独立表与独立 JWT 密钥，**不复用用户账号体系**。

### 2.2 涉及文件

| 层 | 文件 |
| --- | --- |
| 路由 | `routes/routes.cpp`（`register_admin_auth_routes`） |
| Controller | `controllers/admin/auth_controller.{h,cpp}` |
| Service | `services/admin/auth_service.{h,cpp}` |
| Repository | `repositories/admin_account_repository.{h,cpp}`、`token_repository` |
| 模块装配 | `app/modules/admin/auth_module.{h,cpp}` |

### 2.3 路由与中间件

| 方法 | 路径 | 中间件 |
| --- | --- | --- |
| POST | `/admin/api/v1/auth/login` | 无 |
| POST | `/admin/api/v1/auth/logout` | `AdminAccessAuthMiddleware` + `AdminRoleMiddle` |
| POST | `/admin/api/v1/auth/refresh-token` | `AdminRefreshAuthMiddleware` + `AdminRoleMiddle` |

管理员**没有** `sign-up` 与 `logoff` 路由。

### 2.4 API 定义

#### 2.4.1 登录

请求：

```json
{ "admin_id": "admin001", "password": "******" }
```

- `admin_id` 与 `password` 长度上限均为 **8**（实现如此，不是笔误）。
- 成功 `200`：

```json
{
  "data": {
    "admin_id": "admin001",
    "token_type": "Bearer",
    "access_token": "...",
    "access_token_exp": 1730003600,
    "refresh_token": "...",
    "refresh_token_exp": 1730608400
  }
}
```

- 只校验密码，**不检查封禁**（管理员不在 `blacklist` 体系内）。
- 账号不存在 / 密码错误 → `INVALID_CREDENTIALS`。
- 签发 `sub=admin_id, role=sduforum_admin`，使用 `admin_access_secret` / `admin_refresh_secret`。

#### 2.4.2 登出

同用户侧：Header 放 access，body 放 `refresh_token`，成功 `204`。

#### 2.4.3 刷新

同用户侧：Header 放 refresh，body 放 `refreshing` + `access_token`，成功 `200`，data 首字段为 `admin_id`。

### 2.5 已知问题

| 问题 | 影响 |
| --- | --- |
| `config-dev.ini` 未配置 `admin_access_secret` / `admin_refresh_secret` | 管理员 token 以空密钥签名，任何人可伪造（风险 R5） |
| `validate()` 不校验管理员密钥 | 启动不会报错，问题被掩盖 |
| `admin_id` 上限 8 字符 | 业务若需要更长 ID，必须同时改 Controller 与 DB 字段 |
| `AdminAccountRepository::inisert_admin_account` 未绑定 `iterations` | 自定义迭代数无法落库（风险 R9） |
| `AdminAuthModule` 依赖 `PooledDbConnectionProvider` 具体类型 | 无法用 `IDbConnectionProvider` 替身单测管理员认证链路（R23） |
| `admin_account.admin_id` 无唯一索引 | 可能出现重复账号，登录取第一条（R22） |

### 2.6 扩展点

- 管理员重置密码：新增 `PUT /admin/api/v1/auth/password`，必须挂 AdminAccess + AdminRole。
- 管理员封禁：若需要，新增独立表，不要复用用户 `blacklist`。

---

## 3. 管理员用户封禁模块

### 3.1 职责

管理员对用户执行封禁、解除封禁、查询封禁历史与黑名单列表。

### 3.2 涉及文件

| 层 | 文件 |
| --- | --- |
| 路由 | `routes/routes.cpp`（`register_admin_user_routes`） |
| Controller | `controllers/admin/user_controller.{h,cpp}` |
| Service | `services/admin/user_service.{h,cpp}`、`services/admin/ban_service.{h,cpp}` |
| Repository | `repositories/blacklist_repository.{h,cpp}` |
| 模块装配 | `app/modules/admin/user_module.{h,cpp}` |

### 3.3 路由与中间件

全部要求 `AdminAccessAuthMiddleware` + `AdminRoleMiddle`：

| 方法 | 路径 | Handler |
| --- | --- | --- |
| POST | `/admin/api/v1/users/ban` | `ban` |
| POST | `/admin/api/v1/users/unban` | `unban` |
| GET | `/admin/api/v1/users/history` | `get_history` |
| GET | `/admin/api/v1/users/blacklist` | `get_blacklist` |

### 3.4 封禁 `POST /admin/api/v1/users/ban`

请求体：

```json
{
  "account_id": 123,
  "reason_code": 1001,
  "ban_type": 1,
  "time_type": 2,
  "time": 7,
  "reason": "可选，自定义原因",
  "source": "manual",
  "ref_id": 0
}
```

| 字段 | 必填 | 规则 |
| --- | --- | --- |
| `account_id` | 是 | 非负整数（数字或纯数字字符串），必须 > 0 |
| `reason_code` | 是 | 0–65535 且必须是合法 `ReasonCode` |
| `ban_type` | 是 | 0–7 且必须是合法 `BanType` |
| `time_type` | 是 | 0/1/2/0xFFFF；>0xFFFF 非法 |
| `time` | 条件 | `time_type != 0xFFFF` 时必填，1–0xFFFFFFFF；永久时忽略 |
| `reason` | 否 | ≤255；自定义原因码时必须非空 |
| `source` | **实际必填** | `manual` / `auto` / `report`；见下方缺陷 |
| `ref_id` | 否 | 非负整数，默认 0 |

成功：`204`，无 body。

错误：

| 场景 | code | HTTP |
| --- | --- | --- |
| 缺必填字段 / body 非 JSON | `INVALID_JSON` | 400 |
| 数值越界、枚举非法、时长非法、原因文本不满足一致性 | `INVALID_PARAM` | 400 |
| 已有 `All` 封禁 / 已有同维度封禁 | `CONFLICT` | 409 |
| 插入失败、连接不可用 | `DB_ERROR` | 500 |

`AdminUserService::ban` 流程：

1. 用 `BanMeta` 组装 `BanRecordInput`，`reason` 经 `resolve_reason_text` 处理：
   - 自定义码（`Unspecified`/`ManualCustom`）且**有原文** → 用原文；
   - 否则用标准中文文案兜底（含自定义码但原文为空的情况）。
   > 因此「自定义原因必须非空」这条规则在 Service 层被兜底文案绕过了，空原因不会报错（风险 R17）。
2. `ValidateBanInput`：原因码、维度、来源一致性、文本一致性、长度。
3. `exists_active(account_id, All)` → 有则 `CONFLICT`。
4. `exists_active(account_id, type)` → 有则 `CONFLICT`。
5. `resolve_expire_at`：
   - `0xFFFF` → `9999-12-30 23:59:59`；
   - `time_type > 2` 或 `time == 0` → `INVALID_PARAM`；
   - 否则 `now + time*(60|3600|86400)`，格式 `YYYY-MM-DD HH:MM:SS`。
6. 组装 `BlacklistEntry`（`banned_by` 存管理员 ID 字符串），`insert`。
7. 失败 → `DB_ERROR`；成功记 `LOGI`。

关键缺陷（R4）：Controller 写的是

```cpp
meta.source = ban::string_to_ban_source(optional_string(body, "source").value_or(""));
```

`value_or("")` 在缺省时得到空串，`string_to_ban_source("")` 抛 `INVALID_PARAM`。因此 `source` **实际必填**，与 `parse_ban_source` 的「缺省 manual」注释不一致（该辅助函数当前未被 `ban` 使用）。

### 3.5 解除封禁 `POST /admin/api/v1/users/unban`

请求：

```json
{ "account_id": 123, "ban_type": 1, "unban_reason": "申诉通过" }
```

- `account_id` 必填且 > 0；
- `ban_type` 可选，缺省 `All`；
- `unban_reason` 可选，≤255。

成功：`204`。

错误：无生效记录 → `NOT_FOUND`；DB 异常 → `DB_ERROR`。

`AdminUserService::unban`：

1. `has_active = exists_active(account_id, type) || is_blacklisted(account_id)`；
2. 无 → `NOT_FOUND`；
3. `repo.unban(...)`，受影响行数 ≤0 → `NOT_FOUND`；
4. 成功记 `LOGI`。

语义（来自代码注释）：

- `All` 封禁不能被单维度解除（更新 0 行 → `NOT_FOUND`）；
- 单维度（可多个）可以用 `All` 一次性全部解除；
- 解除写入 `unbanned_at=NOW()`、`unbanned_by`、`unban_reason`、`status=0`。

### 3.6 封禁历史 `GET /admin/api/v1/users/history?account_id=123`

- `account_id` 必填，纯数字，超出 `uint64` → `INVALID_PARAM`。
- 成功设计为 `200`，`data` 直接是记录数组：

```json
{ "data": [ { "id": 1, "account_id": 123, "...": "..." } ] }
```

记录字段（`entry_to_json`）：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | number | 记录主键 |
| `account_id` | number | 被封账号内部 ID |
| `reason` | string | 展示原因 |
| `reason_code` | number | 原因码 |
| `ban_type` | number | 维度 |
| `banned_by` | string | 操作人 `admin_id`；`banned_by` 为 `CHAR(8) NOT NULL`，不会是 null |
| `banned_at` | string | 封禁时间 |
| `expire_at` | string | 过期时间；永久封禁为 `9999-12-30 23:59:59` |
| `unbanned_at` | string | 未解封为 `1970-01-01 00:00:00`（不是 null） |
| `unbanned_by` | string | 解封人；未解封为空串 |
| `unban_reason` | string | 解封原因；未解封为空串 |
| `status` | number | 1/0/2 |
| `source` | string | manual/auto/report |
| `ref_id` | number | 关联工单 |

> **当前不可用**：`AdminUserService::get_ban_history` 函数体只有一行注释，没有 `return`（R2）；`BlacklistRepository::get_history` 即使查到数据也 `return {}`（R3）。接口会进入未定义行为。修复时必须让 Repository 返回 `PageResult{entries, total}`，Service 返回 `.item`。

### 3.7 黑名单列表 `GET /admin/api/v1/users/blacklist`

查询参数：

| 参数 | 默认 | 规则 |
| --- | --- | --- |
| `page` | 1 | 1–1000000 |
| `page_size` | 20 | 1–100 |
| `reason_code` | 无 | 0–65535 且合法 |
| `ban_type` | 无 | 0–7 且合法 |
| `status` | 无 | `active` / `expired` / `revoked` 或 `1` / `2` / `0` |
| `from` | 无 | `YYYY-MM-DD HH:MM:SS`，作用于 `banned_at` |
| `to` | 无 | 同上 |

成功设计为：

```json
{
  "data": {
    "page": 1,
    "page_size": 20,
    "items": [ /* 同 history 的记录结构 */ ]
  }
}
```

筛选规则（Controller 硬约束）：

- 有 `reason_code` → 按原因查询；
- 否则有 `ban_type` → 按类型查询；
- 都没有 → 只允许纯分页；若同时传了 `status`/`from`/`to` → `INVALID_PARAM`（`reason_code or ban_type is required when filtering`）。

> **当前不可用**：`list_blacklist`、`search_blacklist_by_reason`、`search_blacklist_by_type` 均无 `return`（R2）；`BlacklistRepository::search` 查完数据仍 `return {}`（R3），且 `account_id` 条件与绑定判断不一致，`nullopt` 时会解引用空 optional（R3b）。

### 3.8 封禁状态查询（未暴露接口）

`AdminUserService::get_user_status(account_id)` 设计为：

- 任一维度 Active → `Active`；
- 否则存在 Expired → `Expired`；
- 否则 `nullopt`。

当前函数无 `return`，且**没有任何路由调用**。若后续开放，建议路径 `GET /admin/api/v1/users/status`。

### 3.9 BanService（跨模块只读入口）

`services/admin/ban_service.h/.cpp`：

```cpp
std::optional<repository::BlacklistEntry>
is_banned(uint64_t account_id, ban::BanType type); // -> blacklist_repo.get_active(...)
```

- 供认证中间件与未来业务 Service 使用；返回 `nullopt` 表示未封禁。
- 判定统一为 `status=1 AND expire_at > NOW()`，不做缓存。
- **不要**在业务层复制这段 SQL，统一走 `BanService`。
- 设计注释：`Login` 只限制「新登录」，不主动失效已有会话，因此 refresh 接口必须单独校验 `Login`；`All` 由认证中间件拦截，实现「下一次请求立即生效」。

### 3.10 扩展点

- 新增封禁维度：只改 `BanType` + `FindBanTypeMeta`，旧数据无需迁移。
- 新增自动封禁：`source=auto`，`reason_code` 用 4xxx；必须满足来源一致性校验。
- 举报联动：`source=report`，`ref_id` 填举报单 ID。
- 定时过期：把 `expire_outdated()` 接入周期任务；不要在查询路径写状态。

---

## 4. 仓储层

### 4.1 连接抽象

`repositories/connection_provider.h`：

| 接口 | 方法 | 说明 |
| --- | --- | --- |
| `IDbConnection` | `prepare_statement` / `set_*` / `execute_query` / `execute_update` | 单条 SQL 句柄 |
| `IDbConnectionProvider` | `acquire()` | 返回 `PooledDbConnection`（RAII） |
| `ICacheConnection` | Redis 命令封装 | 供 TokenRepository 使用 |
| `ICacheConnectionProvider` | `acquire()` | 返回 `PooledCacheConnection` |

- `acquire()` 失败返回空守卫；Repository 必须判空并抛 `DB_ERROR`。
- 连接池单例只在 `AppContext` 中使用；Repository 只依赖接口，便于测试替身。

### 4.2 AccountRepository

| 方法 | SQL 要点 | 异常 |
| --- | --- | --- |
| `find_account_by_user_id` | `SELECT id, public_id, password_hash, salt, iterations ... WHERE user_id=?` | `DB_ERROR` |
| `find_account_by_public_id` | `SELECT id, user_id, password_hash, salt, iterations ... WHERE public_id=?` | `DB_ERROR` |
| `insert_account_by_user_id` | 5 参数插入 | `DB_ERROR` |
| `delete_account_by_public_id` | 按 `public_id` 删除 | `DB_ERROR` |

- 返回 `std::optional<AccountRecord>`；未命中为 `nullopt`，不是异常。
- 表名由构造参数注入。

### 4.3 TokenRepository

| 方法 | 行为 |
| --- | --- |
| `find_refresh_token(hash)` | 先 Redis；不可用/异常降级 DB；DB 命中 best-effort 回写 Redis |
| `insert_refresh_token(hash, expires_at)` | 写 MySQL |
| `find_access_token(hash)` | 只查 Redis；不可用/异常返回 `false`（fail-open） |
| `insert_access_token(hash, ttl)` | 写 Redis；失败返回 `false` |
| `hash_secret()` | 返回 `jwt.hash_secret`（硬编码默认值，不从 INI 读取，R20）；真正的哈希由调用方用 `utils::hmac_sha256_base64url(hash_secret, token)` 计算 |

- 缓存键与值均为 token 哈希；refresh 缓存 TTL 使用 refresh TTL。
- 该组件是所有登录态吊销的核心，改动必须同步 [architecture.md](./architecture.md) §8.2。

### 4.4 AdminAccountRepository

| 方法 | 说明 |
| --- | --- |
| `find_admin_account_by_admin_id` | `SELECT id, password_hash, salt, iterations` |
| `inisert_admin_account` | `INSERT(admin_id, password_hash, salt)`；**拼写错误且未绑定 `iterations`** |

### 4.5 BlacklistRepository

常量：

```cpp
PERMANENT_EXPIRE = "9999-12-30 23:59:59";
NOT_UNBANNED     = "1970-01-01 00:00:00";
```

| 方法 | 状态 | 说明 |
| --- | --- | --- |
| `insert(entry)` | ✅ | 9 列插入，`status` 显式写入；`banned_by` 按字符串绑定到 `CHAR(8)` |
| `exists_active(id, type)` | ✅ | 仅按维度匹配，**不包含 All 覆盖** |
| `unban(id, type, operator, reason)` | ✅ | `All` 解除全部；其他解除单维度；返回受影响行数；`operator` 超过 8 字符会失败 |
| `expire_outdated()` | ✅ | `status=1 AND expire_at<NOW()` → `status=2`，`unbanned_at=expire_at`；**无调用方**（R18） |
| `is_blacklisted(id)` | ✅ | 任一维度生效即 true |
| `get_active(id, type)` | ✅ | 单维度详情 |
| `get_history(id)` | ❌ | 查询后 `return {}`，丢弃 `entries`（R3） |
| `search(filter, page, size)` | ❌ | 查询后 `return {}`；`account_id` 绑定判断有 UB（R3/R3b） |

`search` 已实现的条件：

- `reason_code`、`ban_type`、`banned_at >= from`、`banned_at <= to`；
- `status`：
  - Active → `status=1 AND expire_at > NOW()`
  - Expired → `status=2 OR (status=1 AND expire_at <= NOW())`
  - Revoked → `status=0`
- `account_id`（存在缺陷）；
- `ORDER BY banned_at DESC LIMIT ? OFFSET ?`。

### 4.6 仓储层约束

1. 一个方法只做一条 SQL，不跨表事务编排。
2. 捕获 `sql::SQLException` → `AppException(DB_ERROR)`；`expire_outdated` 例外，记录日志并返回 0。
3. 查询未命中返回 `nullopt` / 空容器，不抛 `NOT_FOUND`（是否 404 由 Service 决定）。
4. 时间统一字符串 `YYYY-MM-DD HH:MM:SS`，由 Service 生成。
5. 禁止在 SQL 中拼接用户输入；一律参数绑定。

### 4.7 历史遗留与临时实现（改动前必读）

| 项 | 现状 | 处理建议 |
| --- | --- | --- |
| `SELECT *` | `get_active` / `search` / `get_history` 使用 `SELECT *`，依赖 `read_entry` 按列名映射 | 新增/删除列时同步映射；建议逐步改显式列名 |
| 查询返回类型不一致 | Repository `get_history` 返回 `PageResult`，Service 声明 `vector<BlacklistEntry>` | 统一为 `PageResult`，Service 取 `.item`（R24） |
| 条件查询未完成 | `search` 构造 SQL 后 `return {}`，`account_id` 绑定存在空 optional 解引用 | 修复 R3/R3b 后再开放接口 |
| 不存在的依赖 | `AdminUserService::list_blacklist` 注释调用 `list_active`，Repository 无此方法 | 用 `search`/`get_history` 实现，不要补一个重复方法 |
| 过期兜底无调用方 | `expire_outdated()` 完整实现但无定时任务 | 新增任务时注意多实例互斥（R18/E07） |
| 管理员仓库拼写 | `inisert_admin_account` 拼写错误且不写 `iterations` | 改名需同步调用方；补绑定或显式依赖默认值（R9） |
| 管理员模块依赖具体类 | `AdminAuthModule` 接收 `PooledDbConnectionProvider&` | 改为 `IDbConnectionProvider&` 以支持替身（R23） |
| 吊销表无清理 | `refresh_tokens` 只增不删 | 增加按 `expires_at` 的归档/清理任务 |
| 文档旧值 | 历史文档曾把 `banned_by` 写成 `BIGINT`、`admin_id` 写成 `VARCHAR(16)` | 以 `db_sqls.md` 的 `CHAR(8)` 为准 |

---

## 5. 新模块接入模板

> 新增模块时复制本节，替换 `Xxx`，并在 [README.md](./README.md) 状态看板登记。

### 5.1 模块名：`XxxModule`

**职责**：

- 一句话说明边界（只做什么、不做什么）。

**涉及文件**：

| 层 | 文件 |
| --- | --- |
| 路由 | `routes/routes.cpp` → `register_xxx_routes` |
| Controller | `controllers/xxx_controller.{h,cpp}` |
| Service | `services/xxx_service.{h,cpp}` |
| Repository | `repositories/xxx_repository.{h,cpp}` |
| 模块 | `app/modules/xxx_module.{h,cpp}` |

**路由与中间件**：

| 方法 | 路径 | 中间件 | 说明 |
| --- | --- | --- | --- |
| | | | |

**API 定义**：

| 接口 | 请求 | 成功响应 | 主要错误 |
| --- | --- | --- | --- |
| | | | |

**业务逻辑**：

1. 
2. 

**核心实现细节**：

- 

**边界与特殊处理**：

- 

**依赖的全局机制**：

- [ ] 统一响应（`api_response.h`）
- [ ] `AppException` + `ErrorCode`
- [ ] 鉴权 / 角色中间件
- [ ] `BanService` 维度检查
- [ ] 配置项 + `validate()`
- [ ] 表结构文档

**已知问题 / 临时实现**：

- 

**扩展点**：

- 

---

## 6. 模块扩展登记（预留）

> 新模块在动工前先在此登记；实现完成后再按 §5 模板补全独立章节。
> 登记后必须同步 [README.md](./README.md) §3 状态看板与 §7 扩展登记表。

| 编号 | 模块 | 状态 | 路由前缀 | 依赖的共享对象 | 新增表/配置 | 独立章节 |
| --- | --- | --- | --- | --- | --- | --- |
| M01 | 帖子 | 未开始 | `/api/v1/posts` | `BanService`、`AccountRepository` | 待定 | §7.1 |
| M02 | 评论 | 未开始 | `/api/v1/comments` | `BanService`、`PostService` | 待定 | §7.2 |
| M03 | 私信 | 未开始 | `/api/v1/messages` | `BanService`、`AccountRepository` | 待定 | §7.3 |
| M04 | 举报/工单 | 未开始 | `/api/v1/reports`、`/admin/api/v1/reports` | `BanService`、`blacklist.ref_id` | 待定 | §7.4 |
| M05 | 用户资料 | 未开始 | `/api/v1/users/me` | `AccountRepository` | `account` 扩展字段 | §7.5 |
| M06 | 文件上传 | 未开始 | `/api/v1/files` | 鉴权中间件 | 对象存储配置 | §7.6 |
| M07 | 封禁过期任务 | 未开始 | 无 HTTP | `BlacklistRepository::expire_outdated()` | 调度配置 | §7.7 |
| | | | | | | |

### 6.1 新增模块时的固定检查

1. 继承 `app::Module`，依赖接口而非具体连接池/Provider。
2. 在 `AppContext` 中构造并持有，在 `register_routes()` 调用。
3. 路由函数放入 `routes/routes.cpp`，不要在模块内散落 URL。
4. 写操作前按维度调用 `admin::service::BanService::is_banned()`；`All` 已由鉴权中间件兜底。
5. 新增表必须同时更新 [db_sqls.md](./db_sqls.md) 与 [database.md](./database.md)。
6. 新增配置项必须进入 `AppConfig::load()` 与 `validate()`，并补充开发 INI。
