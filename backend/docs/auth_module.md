# 用户认证模块 开发文档

> 版本：v1.0｜修订：2026-11-28

---

## 一、目标

本模块提供用户端认证能力：

- 注册：创建认证账号，返回对外 `public_id`，不签发 token；
- 登录：校验账号密码与封禁状态，签发双 token；
- 刷新：校验 refresh token，原子轮换双 token；
- 登出：使当前双 token 失效；
- 注销：注销账号并保留业务内容关联，吊销双 token；
- 改密：校验旧密码，更新密码，吊销双 token；
- 中间件：提供用户 access 认证、用户 refresh 认证、用户角色校验。

本模块只负责**认证账号与凭证**。用户详情、内容归属、封禁规则分别由用户模块、内容模块、管理员封禁模块负责。

---

## 二、数据模型

### 2.1 `account`（认证账号表）

| 字段 | 类型 | 描述 |
| --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 主键自增；内部关联标识，不对外暴露 |
| `public_id` | `CHAR(24)` | 对外公开 ID；`usr_` + 20 位 Base62；JWT `sub` |
| `user_id` | `CHAR(12)` | 登录账号，12 位数字； |
| `password_hash` | `CHAR(64)` | PBKDF2-HMAC-SHA512 派生密钥十六进制 |
| `salt` | `CHAR(32)` | 16 字节随机盐十六进制 |
| `iterations` | `INT UNSIGNED` | 迭代次数，默认 `150000`，校验以存储值为准 |
| `deleted_at` | `DATETIME NULL` | 注销时间；非空即表示账号已被注销，禁止登录与认证 |
| `created_at`    | `DATETIME`        | 默认 `CURRENT_TIMESTAMP`                             |
| `updated_at`    | `DATETIME`        | 默认 `CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP` |

索引：`PRIMARY KEY (id)`、`UNIQUE uk_public_id (public_id)`、`UNIQUE uk_user_id (user_id)`。

设计说明：

- 字符集 `utf8mb4_bin`，`user_id`、`public_id` 二进制比较，大小写敏感；
- 注销为软删除：账号信息保留，仅更新``deleted_at``；
- 登录查询必须带 `deleted_at IS NULL`；中间件按 `public_id` 查到非空即按未认证处理；

### 2.2 `revoked_token`（吊销名单表）

| 字段 | 类型 | 描述 |
| --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 主键自增 |
| `token_hash` | `CHAR(64)` | （存储键） |
| `token_type` | `VARCHAR(8)` | `access` / `refresh` |
| `revoked_at` | `DATETIME` | 吊销时间，默认当前时间 |
| `expires_at` | `DATETIME` | 原 token 过期时间，仅用于清理，不参与吊销判定 |

索引：`PRIMARY KEY (id)`、`UNIQUE uk_token_hash (token_hash)`、`KEY idx_expires_at`。

设计说明：

- 用户端与管理端共用表；
- 清理任务按 `expires_at < NOW()` 批量删除，建议每分钟或每 5 分钟执行；
- 只保存已吊销 token；有效 token 不落库；

### 2.3 标识体系

| 标识 | 用途 | 对外 | 生成 |
| --- | --- | --- | --- |
| `id` | 内部主键、外键关联 | 否 | 数据库自增 |
| `user_id` | 登录账号 | 仅注册/登录时出现 | 用户输入，12 位数字 |
| `public_id` | 对外标识、JWT `sub` | 是 | `usr_` + 20 位 Base62（密码学安全随机 + 拒绝采样），唯一索引兜底 |

### 2.4 JWT 结构

算法固定 `HS256`。

| 部分    | Claim        | 值                   |
| ------- | ------------ | -------------------- |
| header  | `alg`        | `HS256`              |
| header  | `typ`        | `JWT`                |
| payload | `iss`        | 配置 issuer          |
| payload | `aud`        | `sduforum_user_api`  |
| payload | `iat`        | 签发 Unix 秒         |
| payload | `exp`        | 过期 Unix 秒         |
| payload | `sub`        | `public_id`          |
| payload | `sid`        | 会话ID               |
| payload | `role`       | `sduforum_user`      |
| payload | `token_type` | `access` / `refresh` |

设计说明：

- access 与 refresh 的 `sub`、`sid` 必须一致（绑定关系）；
- `sid` 应该在登录时生成，刷新时保持不变。
- 生命周期：access 短有效期用于业务访问；refresh 长有效期仅用于刷新，两者默认 TTL 由配置提供。

### 2.5 token吊销方案

#### 方案①：基于版本链的控制

`account` 表存 `version`，Token payload 存 `ver`。验签后比对两者必须一致。登出、注销、改密时 `version + 1`，旧双 Token 全部失效。

**问题**：刷新接口并发时，“读版本—比版本—写版本—签新 Token”不是原子操作。请求 1、2 都通过版本比对后，串行自增并各自签发新 Token；后签发的会让先签发的 Access Token 变成旧版本，导致刚拿到的 Token 又被拒，形成循环失败。

**结论**：适合不会并发的登出、改密、注销；不适合刷新接口。

#### 方案②：黑名单表

登出、注销、改密、刷新后，把仍未到期的旧 Token 的写入黑名单表（附 `exp`，TTL 对齐过期时间）。验签后查表，存在即拒绝。

**并发刷新**：旧 Refresh Token  加唯一约束。请求 1、2 都查表无记录，但插入串行化——先插入者成功并签发新 Token，后插入者因唯一约束失败，直接返回 401、不签发新 Token。保证同时只有一个刷新成功，不会循环失败。

**问题**：但是会导致登出、注销、改密接口需要在请求体中携带`refresh_token`,刷新接口的请求体要携带`access_token`,设计不是很好。

**结论**：适合刷新接口；登出/改密也可用，但用户级全端撤销逐条拉黑较重，可拆分用版本控制。



所以折中的方案是，登出、注销、改密这类不会并发的接口使用版本控制，刷新接口使用黑名单表。但是为了开发方便，目前统一使用黑名单表。

---

## 三、认证链/中间件

### 3.1 token 认证中间件（access / refresh 共用）

 **access 与 refresh 绑定关系**：`sub`与`sid`要保持一致

按以下顺序执行，任一步失败即终止：

1. 解析 `Authorization: Bearer <token>`；头缺失、非 Bearer、token 为空均返回 `401 UNAUTHORIZED`；
2. 格式与签名：三段结构完整、`alg=HS256`、重算签名并做恒定时间比较、`iss`/`aud` 匹配、`exp` 不早于服务端当前时间；
3. claims 完整性：`sub`、`role`、`sid`、`token_type`非空；
4. `token_type` 必须与中间件一致（access 中间件只收 `access`，refresh 中间件只收 `refresh`）；
5. 查吊销名单（Redis → MySQL → 回写），命中即 `401`；
6. 按 `sub`（即 `public_id`）查账号信息 ；
7. `All` 封禁判断，命中即 `403 ACCOUNT_BANNED`；
8. 注入请求属性 `public_id`、`role`、`user_id`、`account_id`、`session_id`。

> 约定：受保护接口固定“认证在前、角色在后”；下游业务只读请求属性，不重复解析 token。且业务编排中统一使用主键id标识用户

### 3.2 角色中间件

校验请求属性 `role == sduforum_user`，失败返回 `403 FORBIDDEN`。

---

## 四、接口设计

### 4.1 接口清单

| 接口 | 方法 | 路径 | 认证 | 成功 |
| --- | --- | --- | --- | --- |
| 注册 | POST | `/auth/sign_up` | 无 | `201` + `public_id`，不发 token |
| 登录 | POST | `/auth/login` | 无 | `200` + 双 token |
| 刷新 | POST | `/auth/refresh_token` | refresh 认证 + 角色 | `200` + 新双 token（body 传旧 access） |
| 登出 | POST | `/auth/logout` | access 认证 + 角色 | `204`（body 传 refresh） |
| 注销 | POST | `/auth/logoff` | access 认证 + 角色 | `204`（body 传 refresh，软删除） |
| 改密 | PATCH | `/auth/password` | access 认证 + 角色 | `204`（body 传旧、新密码） |

以下路径均以 `/sdu_forum/api/v1` 为前缀。

### 4.2 封禁模块联动

| 维度    | 检查位置     | 时机                     | 结果                 |
| ------- | ------------ | ------------------------ | -------------------- |
| `All`   | 认证中间件   | 每次 access/refresh 鉴权 | `403 ACCOUNT_BANNED` |
| `Login` | 登录 Service | 密码校验通过后           | `403 ACCOUNT_BANNED` |
| `Login` | 刷新 Service | refresh 验签通过后       | `403 ACCOUNT_BANNED` |

封禁模块需提供内部查询接口：按 `account_id` + 维度返回是否生效及 `expire_at`。

### 4.2 注册

`POST /sdu_forum/api/v1/auth/sign_up` ｜ 认证：无

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `user_id` | string | 是 | 非空，12 位数字 |
| `password` | string | 是 | 非空，固定 8 位 |

关键逻辑：

1. 校验 JSON 与字段格式；
2. 按 `user_id` 查重；
3. 生成 `public_id`，计算 PBKDF2 哈希；
4. 写入 `account`表；
5. 返回 `201` 与 `public_id`，**不签发 token**。

```json
{
  "data": { "public_id": "usr_7Qm2xK9pLr4Tz8Wn1VbC" }
}
```

> 说明：当前默认public_id不会冲突，用户数量预估在10万级

### 4.3 登录

`POST /sdu_forum/api/v1/auth/login` ｜ 认证：无

请求 body 同注册（`user_id`、`password`）。

关键逻辑：

1. 校验 JSON 与字段格式；
2. 按 `user_id` 查 `deleted_at IS NULL` 的账号；账号不存在或密码错误统一返回 `401 INVALID_CREDENTIALS`；
3. 密码校验通过后检查 `All`、`Login` 封禁，命中返回 `403 ACCOUNT_BANNED`；
4. 签发 access/refresh；
7. 返回 `200` 与双 token 及过期时间。

```json
{
  "data": {
    "public_id": "usr_7Qm2xK9pLr4Tz8Wn1VbC",
    "token_type": "Bearer",
    "access_token": "<JWT>",
    "access_token_exp": 1730003600,
    "refresh_token": "<JWT>",
    "refresh_token_exp": 1730608400
  }
}
```

### 4.4 刷新令牌

`POST /sdu_forum/api/v1/auth/refresh_token` ｜ 认证：refresh 认证 + 角色

请求头 `Authorization: Bearer <refresh_token>`；

body：

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `access_token` | string | 是 | 当前 access token |

关键逻辑：

1. refresh 认证中间件+角色中间件校验通过；
2. 检查 `Login` 封禁，命中 `403 ACCOUNT_BANNED`；
3. 解析 body 中旧 access：双token要符合绑定关系；已吊销 → `401`；
4. 原子吊销旧 refresh：插入吊销表，唯一键冲突（已被抢占或已吊销）→ `401 UNAUTHORIZED`；
5. 旧 access 未过期且未吊销时写入吊销；
6.  签发新双 token；
7. 返回 `200`与双 token 及过期时间。

	```json
	{
	  "data": {
	    "public_id": "usr_7Qm2xK9pLr4Tz8Wn1VbC",
	    "token_type": "Bearer",
	    "access_token": "<new JWT>",
	    "access_token_exp": 1730003600,
	    "refresh_token": "<new JWT>",
	    "refresh_token_exp": 1730608400
	  }
	}
	```

说明：本期无后端刷新宽限期；并发刷新只有一个能抢占成功，其余 `401`，客户端需用单飞锁串行化刷新。

### 4.5 登出

`POST /sdu_forum/api/v1/auth/logout` ｜ 认证：access 认证 + 角色

请求头 `Authorization: Bearer <access_token>`；

body：

| 字段            | 类型   | 必填 | 约束 |
| --------------- | ------ | ---- | ---- |
| `refresh_token` | string | 是   | 非空 |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 解析 body 中 refresh：双token要符合绑定关系，否则 `401`；
3. 吊销 access 与 refresh；
4. 返回 `204`。

### 4.6 注销

`POST /sdu_forum/api/v1/auth/logoff` ｜ 认证：access 认证 + 角色

请求头 `Authorization: Bearer <access_token>`；

body：

| 字段            | 类型   | 必填 | 约束 |
| --------------- | ------ | ---- | ---- |
| `refresh_token` | string | 是   | 非空 |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 解析 refresh：双token要符合绑定关系，否则 `401`；
3. 更新 `account`：`deleted_at = NOW()`；
4. 吊销 access 与 refresh；
5. 返回 `204`。

一致性要点：

- 软删除：账号信息保留，内容、举报、审计等外键不级联删除；对外展示由用户模块按“已注销用户”处理；
- 封禁记录保留，便于治理审计；

### 4.7 改密

`PATCH /sdu_forum/api/v1/auth/password` ｜ 认证：access 认证 + 角色

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `old_password` | string | 是 | 非空，固定 8 位 |
| `new_password` | string | 是 | 非空，固定 8 位，不得与旧密码相同 |
| `refresh_token` | string | 是 | 非空 |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 解析 refresh：双token要符合绑定关系，否则 `401`；
3. 按 `account_id` 查账号，校验 `old_password`，失败返回 `401 INVALID_CREDENTIALS`；
4. 新旧密码相同返回 `400 INVALID_PARAM`；
5. 重算 `password_hash`、`salt`、`iterations`；
6. 吊销双token
7. 不签发 token，返回 `204`；客户端需重新登录。

---

## 六、后续规划

- 多设备会话管理、会话列表与“踢下线”；
- 邮箱/手机号验证、验证码、找回密码；
- 第三方登录、学校统一认证；
- 密码强度策略增强；
