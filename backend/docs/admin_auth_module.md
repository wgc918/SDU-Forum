# 管理员认证模块 开发文档

> 版本：v1.0｜修订：2026-11-28

---

## 一、目标

本模块提供管理端认证能力：

- 登录：校验管理员账号密码，签发双 token；
- 刷新：校验 refresh token，原子轮换双 token；
- 登出：使当前双 token 失效；
- 中间件：提供管理员 access 认证、管理员 refresh 认证、管理员角色校验。

管理端与用户端在账号表、签名密钥、角色标识上完全隔离。管理员账号由运维统一开通，本模块不提供注册、注销、改密。

本模块只负责**管理员账号与凭证**。内容治理等能力由封禁等业务模块负责。

---

## 二、数据模型

### 2.1 `admin_account`（管理员账号表）

| 字段 | 类型 | 描述 |
| --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 主键自增；内部关联标识，不对外暴露 |
| `admin_id` | `CHAR(8)` | 登录账号，8 位；JWT `sub` |
| `password_hash` | `CHAR(64)` | PBKDF2-HMAC-SHA512 派生密钥十六进制 |
| `salt` | `CHAR(32)` | 16 字节随机盐十六进制 |
| `iterations` | `INT UNSIGNED` | 迭代次数，默认 `150000`，校验以存储值为准 |
| `created_at` | `DATETIME` | 默认 `CURRENT_TIMESTAMP` |
| `updated_at` | `DATETIME` | 默认 `CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP` |

索引：`PRIMARY KEY (id)`、`UNIQUE uk_admin_id (admin_id)`。

设计说明：

- 独立建表，不复用用户 `account` 表；管理员与用户账号的创建流程、生命周期、权限边界互不影响；
- 本期只有单一管理员角色，角色由 token claim 固定为 `sduforum_admin`，不建角色表；
- `iterations` 独立成列，后续提升迭代次数时旧账号仍可按原值校验，无需全量重置密码；
- 字符集 `utf8mb4_bin`，`admin_id` 二进制比较，大小写敏感；
- 管理员账号由运维流程直接写入，密码哈希规则必须与本模块一致。

### 2.2 `revoked_token`（吊销名单表）

与用户端共用同一张表。

| 字段 | 类型 | 描述 |
| --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 主键自增 |
| `token_hash` | `CHAR(64)` | 存储键：`base64url(HMAC-SHA256(hash_secret, token))` |
| `token_type` | `VARCHAR(8)` | `access` / `refresh` |
| `revoked_at` | `DATETIME` | 吊销时间，默认当前时间 |
| `expires_at` | `DATETIME` | 原 token 过期时间，仅用于清理，不参与吊销判定 |

索引：`PRIMARY KEY (id)`、`UNIQUE uk_token_hash (token_hash)`、`KEY idx_expires_at`。

设计说明：

- 用户端与管理端共用表；
- 只保存已吊销 token；有效 token 不落库，减少数据泄露面；
- 只存哈希不存明文，数据库泄露也无法直接复用 token；
- 清理任务按 `expires_at < NOW()` 批量删除，建议每分钟或每 5 分钟执行；

### 2.3 标识体系

| 标识 | 用途 | 对外 | 生成 |
| --- | --- | --- | --- |
| `id` | 内部主键、外键关联 | 否 | 数据库自增 |
| `admin_id` | 登录账号、JWT `sub` | 登录/登出等出现 | 运维开通，固定 8 位 |

### 2.4 JWT 结构

算法固定 `HS256`。

| 部分 | Claim | 值 |
| --- | --- | --- |
| header | `alg` | `HS256` |
| header | `typ` | `JWT` |
| payload | `iss` | 配置 issuer |
| payload | `aud` | `sduforum_admin_api` |
| payload | `iat` | 签发 Unix 秒 |
| payload | `exp` | 过期 Unix 秒 |
| payload | `sub` | `admin_id` |
| payload | `sid` | 会话ID |
| payload | `role` | `sduforum_admin` |
| payload | `token_type` | `access` / `refresh` |

设计说明：

- 管理员 access/refresh 使用独立签名密钥，与用户端完全隔离；用用户密钥签发的 token 无法通过管理员中间件验签；
- access 与 refresh 的 `sub`、`sid` 必须一致（绑定关系）；
- `sid` 应该在登录时生成，刷新时保持不变。
- 生命周期：access 短有效期用于业务访问；refresh 长有效期仅用于刷新，两者默认 TTL 由配置提供。

---

## 三、认证链/中间件

### 3.1 token 认证中间件（access / refresh 共用）

按以下顺序执行，任一步失败即终止：

1. 解析 `Authorization: Bearer <token>`；头缺失、非 Bearer、token 为空均返回 `401 UNAUTHORIZED`；
2. 格式与签名：三段结构完整、`alg=HS256`、重算签名并做恒定时间比较、`iss`/`aud` 匹配、`exp` 不早于服务端当前时间；
3. claims 完整性：`sub`、`role`、`sid`、`token_type` 非空；
4. `token_type` 必须与中间件一致（access 中间件只收 `access`，refresh 中间件只收 `refresh`）；
5. 查吊销名单（Redis → MySQL → 回写），命中即 `401`；
6. 按 `sub`（即 `admin_id`）查 `admin_account`，查不到即 `401`；
7. 注入请求属性 `admin_id`、`role`、`account_id`、`session_id`。

> 约定：受保护接口固定“认证在前、角色在后”；下游业务只读请求属性，不重复解析 token。管理员不参与用户封禁体系，鉴权不查询封禁表。

### 3.2 角色中间件

校验请求属性 `role == sduforum_admin`，失败返回 `403 FORBIDDEN`。

---

## 四、接口设计

### 4.1 接口清单

| 接口 | 方法 | 路径 | 认证 | 成功 |
| --- | --- | --- | --- | --- |
| 登录 | POST | `/auth/login` | 无 | `200` + 双 token |
| 刷新 | POST | `/auth/refresh_token` | refresh 认证 + 角色 | `200` + 新双 token（body 传旧 access） |
| 登出 | POST | `/auth/logout` | access 认证 + 角色 | `204`（body 传 refresh） |

以下路径均以 `/admin/api/v1` 为前缀。

### 4.2 登录

`POST /admin/api/v1/auth/login` ｜ 认证：无

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `admin_id` | string | 是 | 非空，固定 8 位 |
| `password` | string | 是 | 非空，固定 8 位 |

关键逻辑：

1. 校验 JSON 与字段格式；
2. 按 `admin_id` 查账号；账号不存在或密码错误统一返回 `401 INVALID_CREDENTIALS`；
3. 签发 access/refresh（`sub = admin_id`、`role = sduforum_admin`、共用同一 `sid`）；
4. 返回 `200` 与双 token 及过期时间。

```json
{
  "data": {
    "admin_id": "admin001",
    "token_type": "Bearer",
    "access_token": "<JWT>",
    "access_token_exp": 1730003600,
    "refresh_token": "<JWT>",
    "refresh_token_exp": 1730608400
  }
}
```

### 4.3 刷新令牌

`POST /admin/api/v1/auth/refresh_token` ｜ 认证：refresh 认证 + 角色

请求头 `Authorization: Bearer <refresh_token>`；

body：

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `access_token` | string | 是 | 当前 access token |

关键逻辑：

1. refresh 认证中间件 + 角色中间件校验通过；
2. 解析 body 中旧 access：双 token 要符合绑定关系；已吊销 → `401`；
3. 原子吊销旧 refresh：插入吊销表，唯一键冲突（已被抢占或已吊销）→ `401 UNAUTHORIZED`；
4. 旧 access 未过期且未吊销时写入吊销；
5. 签发新双 token；
6. 返回 `200` 与双 token 及过期时间。

> 说明：本期无后端刷新宽限期；并发刷新只有一个能抢占成功，其余 `401`，客户端需用单飞锁串行化刷新。

```json
{
  "data": {
    "admin_id": "admin001",
    "token_type": "Bearer",
    "access_token": "<new JWT>",
    "access_token_exp": 1730003600,
    "refresh_token": "<new JWT>",
    "refresh_token_exp": 1730608400
  }
}
```

### 4.4 登出

`POST /admin/api/v1/auth/logout` ｜ 认证：access 认证 + 角色

请求头 `Authorization: Bearer <access_token>`；

body：

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `refresh_token` | string | 是 | 非空 |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 解析 body 中 refresh：双 token 要符合绑定关系，否则 `401`；
3. 吊销 access 与 refresh；
4. 返回 `204`。

---

## 六、后续规划

- 细粒度 RBAC（菜单、按钮、数据权限）；
- 管理员操作审计日志增强。
