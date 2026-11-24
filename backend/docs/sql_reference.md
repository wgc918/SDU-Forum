# SDU-Forum 数据库 SQL 参考手册

> 适用代码：`backend/repositories/`、`backend/docs/db_sqls.md`、`backend/common/ban_enums.h`  
> 数据库：MySQL 8，字符集 `utf8mb4`  
> 最后核对：2026-11-24  
> 文档定位：按业务模块组织全部建表 DDL 与业务 DML，供开发、评审、排障时快速查阅。

---

## 目录

- [0. 文档说明](#0-文档说明)
  - [0.1 数据来源与权威性](#01-数据来源与权威性)
  - [0.2 模块总览](#02-模块总览)
  - [0.3 通用约定](#03-通用约定)
- [1. 用户账号模块](#1-用户账号模块)
- [2. 管理员账号模块](#2-管理员账号模块)
- [3. 账号封禁模块](#3-账号封禁模块)
- [4. 令牌（Token）模块](#4-令牌token模块)
- [5. SQL 与代码映射总表](#5-sql-与代码映射总表)
- [附录 A：设计稿与实现差异](#附录-设计稿与实现差异)
- [附录 B：db_sqls.md 原始 SQL 存档](#附录-bdb_sqlsmd-原始-sql-存档)

---

## 0. 文档说明

### 0.1 数据来源与权威性

| 来源 | 路径 | 在本文中的用途 |
| --- | --- | --- |
| 建表 DDL / 参考 SQL | [db_sqls.md](./db_sqls.md) | 表结构定义；已于 2026-11-24 校正业务 SQL 语法 |
| 实现 SQL | `backend/repositories/*.cpp` | 运行时**实际执行**的参数化 SQL |
| 封禁枚举 | `backend/common/ban_enums.h` | `ban_type`、`status`、`source`、`reason_code` 的取值语义 |

约定：

1. **主文档中的业务 SQL 以 Repository 实现为准**，因为那是线上实际执行的语句。
2. `db_sqls.md` 已按实现校正；校正前的原文仅保留在[附录 B](#附录-bdb_sqlsmd-原始-sql-存档)，仅用于追溯。
3. 两份来源存在差异时，统一在[附录 A](#附录-设计稿与实现差异)中列明，主文档不静默修改原文。

### 0.2 模块总览

| 编号 | 业务模块 | 数据表 | Repository | 主要操作 | 章节 |
| --- | --- | --- | --- | --- | --- |
| 1 | 用户账号 | `account` | `AccountRepository` | 注册写入、登录查询、注销删除 | [§1](#1-用户账号模块) |
| 2 | 管理员账号 | `admin_account` | `AdminAccountRepository` | 管理员凭据查询、账号写入 | [§2](#2-管理员账号模块) |
| 3 | 账号封禁 | `blacklist` | `BlacklistRepository` | 封禁、解封、过期兜底、多条件查询 | [§3](#3-账号封禁模块) |
| 4 | 令牌 | `refresh_tokens` + Redis | `TokenRepository` | refresh token 持久化与校验；access token 缓存 | [§4](#4-令牌token模块) |

### 0.3 通用约定

- **表名不硬编码**：`account_table`、`blacklist_table`、`admin_account_table`、`refresh_token_table` 均由配置注入 Repository。本文使用默认表名展示，实际执行时由 Repository 拼接配置值。
- **参数绑定**：所有业务 SQL 均使用 `?` 预处理占位符，参数下标从 1 开始，禁止拼接用户输入。
- **时间格式**：统一 `YYYY-MM-DD HH:MM:SS`，字段类型为 `DATETIME`（不带时区）。
- **生效判定**：封禁记录统一以 `status = 1 AND expire_at > NOW()` 判定为生效中。
- **异常语义**：数据库不可用抛 `AppException(DatabaseError)`；查询未命中的业务语义由上层决定。

---

## 1. 用户账号模块

### 1.1 模块职责

负责普通用户的注册、登录凭据读取与账号注销。该表只保存认证所需的最小信息，不承载用户资料、帖子等业务数据。

### 1.2 表结构（`account`）

```sql
CREATE TABLE `account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '内部主键，外键关联用',
  `public_id`     CHAR(24)        NOT NULL                COMMENT '对外公开ID，不可枚举',
  `user_id`       CHAR(12)        NOT NULL                COMMENT '登录账号，用户输入12位数字',
  `password_hash` CHAR(64)        NOT NULL                COMMENT 'PBKDF2-SHA256 hex',
  `salt`          CHAR(32)        NOT NULL                COMMENT '随机盐 hex',
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000   COMMENT 'PBKDF2迭代次数',
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_public_id` (`public_id`),
  UNIQUE KEY `uk_user_id`   (`user_id`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_bin
  COMMENT='用户账号表';
```

**字段说明**

| 字段 | 类型 | 语义 | 代码使用 |
| --- | --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 内部主键，供外键关联 | `AccountRecord.id`；`blacklist.account_id` 引用 |
| `public_id` | `CHAR(24)` | 对外公开 ID，不可枚举 | JWT `sub`、接口返回、注销条件 |
| `user_id` | `CHAR(12)` | 登录账号（12 位数字） | 登录查询键；唯一索引 |
| `password_hash` | `CHAR(64)` | PBKDF2 派生值 hex | 密码校验 |
| `salt` | `CHAR(32)` | 随机盐 hex | 密码校验 |
| `iterations` | `INT UNSIGNED` | PBKDF2 迭代次数，默认 150000 | 校验时读取，兼容历史值 |
| `created_at` | `DATETIME` | 创建时间 | 审计 |
| `updated_at` | `DATETIME` | 更新时间，自动维护 | 审计 |

### 1.3 业务操作 SQL

#### 1.3.1 按登录账号查询用户凭据

**用途**：用户登录时，根据输入的登录账号读取密码校验所需的全部字段。  
**实现**：`AccountRepository::find_account_by_user_id`

```sql
-- 用途：按登录账号查询用户凭据；user_id 唯一，最多返回 1 条
SELECT id, public_id, password_hash, salt, iterations
FROM account
WHERE user_id = ?;
```

**参数**：`?1 = user_id`，用户输入的登录账号。  
**核心字段**：`id`、`public_id`、`password_hash`、`salt`、`iterations`。  
**返回**：命中返回 `AccountRecord`；未命中返回 `std::nullopt`。

#### 1.3.2 按公开 ID 查询用户凭据

**用途**：已持有 `public_id`（如 JWT `sub`）时反查账号信息，用于刷新令牌或注销前的身份确认。  
**实现**：`AccountRepository::find_account_by_public_id`

```sql
-- 用途：按对外公开 ID 查询用户凭据；public_id 唯一，最多返回 1 条
SELECT id, user_id, password_hash, salt, iterations
FROM account
WHERE public_id = ?;
```

**参数**：`?1 = public_id`，对外公开 ID。  
**核心字段**：`id`、`user_id`、`password_hash`、`salt`、`iterations`。  
**返回**：命中返回 `AccountRecord`；未命中返回 `std::nullopt`。

#### 1.3.3 创建用户账号

**用途**：注册成功后写入账号凭据。  
**实现**：`AccountRepository::insert_account_by_user_id`

```sql
-- 用途：写入一条用户账号记录；账号查重由业务层负责
INSERT INTO account (public_id, user_id, password_hash, salt, iterations)
VALUES (?, ?, ?, ?, ?);
```

**参数**

| 序号 | 占位符 | 字段 | 含义 |
| --- | --- | --- | --- |
| 1 | `?` | `public_id` | 对外公开 ID |
| 2 | `?` | `user_id` | 登录账号 |
| 3 | `?` | `password_hash` | PBKDF2 派生值 hex |
| 4 | `?` | `salt` | 随机盐 hex |
| 5 | `?` | `iterations` | PBKDF2 迭代次数 |

**核心字段**：`public_id`、`user_id` 的唯一性由表上的 `uk_public_id`、`uk_user_id` 保证。  
**返回**：受影响行数大于 0 返回 `true`。  
**注意**：Repository 注释明确「查重逻辑交由业务层负责」，本语句不包含 `INSERT ... ON DUPLICATE KEY` 等去重逻辑。

#### 1.3.4 按公开 ID 删除用户账号

**用途**：账号注销时删除账号记录。  
**实现**：`AccountRepository::delete_account_by_public_id`

```sql
-- 用途：按对外公开 ID 注销账号；依赖外键级联清理封禁记录
DELETE FROM account
WHERE public_id = ?;
```

**参数**：`?1 = public_id`，待注销账号的对外公开 ID。  
**核心字段**：`public_id`（唯一索引定位）。  
**返回**：实际删除行数大于 0 返回 `true`。  
**注意**：`blacklist.account_id` 对 `account.id` 设置了 `ON DELETE CASCADE`，删除账号会同步删除其全部封禁历史。

### 1.4 模块注意事项

- `public_id` 长度 `CHAR(24)`，需与生成规则（如 `usr_` + 20 位 Base62）保持一致。
- 排序规则为 `utf8mb4_bin`，`user_id` 比较**大小写敏感**。
- `db_sqls.md` 中该表的 `INSERT` 语句存在字段名拼写差异，详见[附录 A](#附录-设计稿与实现差异)。

---

## 2. 管理员账号模块

### 2.1 模块职责

保存管理员登录凭据，供后台管理端认证使用。业务上的封禁操作人 `banned_by` / `unbanned_by` 记录的是管理员账号字符串（`admin_id`），而非本表主键。

### 2.2 表结构（`admin_account`）

```sql
CREATE TABLE `admin_account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '内部主键，外键关联用',
  `admin_id`      CHAR(8)         NOT NULL                COMMENT '登录账号，admin001',
  `password_hash` CHAR(64)        NOT NULL                COMMENT 'PBKDF2-SHA256 hex',
  `salt`          CHAR(32)        NOT NULL                COMMENT '随机盐 hex',
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000   COMMENT 'PBKDF2迭代次数',
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_bin
  COMMENT='管理员账号表';
```

**字段说明**

| 字段 | 类型 | 语义 | 代码使用 |
| --- | --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 内部主键 | `AdminAccountRecord.id` |
| `admin_id` | `CHAR(8)` | 管理员登录账号，如 `admin001` | 登录查询键 |
| `password_hash` | `CHAR(64)` | PBKDF2 派生值 hex | 密码校验 |
| `salt` | `CHAR(32)` | 随机盐 hex | 密码校验 |
| `iterations` | `INT UNSIGNED` | PBKDF2 迭代次数，默认 150000 | 校验时读取 |
| `created_at` | `DATETIME` | 创建时间 | 审计 |
| `updated_at` | `DATETIME` | 更新时间，自动维护 | 审计 |

### 2.3 业务操作 SQL

#### 2.3.1 按管理员账号查询凭据

**用途**：管理员登录时读取密码校验字段。  
**实现**：`AdminAccountRepository::find_admin_account_by_admin_id`

```sql
-- 用途：按管理员登录账号查询凭据；admin_id 用于定位唯一管理员
SELECT id, password_hash, salt, iterations
FROM admin_account
WHERE admin_id = ?;
```

**参数**：`?1 = admin_id`，管理员登录账号。  
**核心字段**：`id`、`password_hash`、`salt`、`iterations`。  
**返回**：命中返回 `AdminAccountRecord`；未命中返回 `std::nullopt`。

#### 2.3.2 创建管理员账号

**用途**：初始化或新增管理员账号。  
**实现**：`AdminAccountRepository::inisert_admin_account`

```sql
-- 用途：写入管理员账号；iterations 未显式绑定，使用表默认值 150000
INSERT INTO admin_account (admin_id, password_hash, salt)
VALUES (?, ?, ?);
```

**参数**

| 序号 | 占位符 | 字段 | 含义 |
| --- | --- | --- | --- |
| 1 | `?` | `admin_id` | 管理员登录账号 |
| 2 | `?` | `password_hash` | PBKDF2 派生值 hex |
| 3 | `?` | `salt` | 随机盐 hex |

**核心字段**：`admin_id`、`password_hash`、`salt`。  
**返回**：受影响行数大于 0 返回 `true`。  
**注意**：语句未绑定 `iterations`，依赖建表默认值 `150000`。

### 2.4 模块注意事项

- 表结构**未对 `admin_id` 建立唯一索引**，但查询语义默认其唯一；新增管理员前需由业务层保证不重复，或后续补充 `UNIQUE KEY uk_admin_id (admin_id)`。
- `admin_id` 定义为 `CHAR(8)`，与上层「管理员账号不超过 8 位」的限制一致。
- 代码方法名 `inisert_admin_account` 为现有拼写，文档按原样引用。

---

## 3. 账号封禁模块

### 3.1 模块职责

维护账号在**不同维度**（全站、评论、发帖、私信等）的封禁记录，支持封禁、解封、自动过期兜底，以及按原因、类型、时间、状态、账号等条件的分页查询。

### 3.2 表结构（`blacklist`）

```sql
CREATE TABLE `blacklist` (
  `id`             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '封禁记录主键',
  `account_id`     BIGINT UNSIGNED NOT NULL                COMMENT '被封账号，引用 account.id',
  `reason`         VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT '封禁原因（对外展示）',
  `reason_code`    SMALLINT UNSIGNED NOT NULL DEFAULT 0    COMMENT '原因分类码，便于统计/自动化',
  `ban_type`       TINYINT UNSIGNED NOT NULL DEFAULT 0     COMMENT '0=全站 1=禁评论 2=禁发帖 ...',
  `banned_by`      CHAR(8)         NOT NULL DEFAULT 'auto'  COMMENT '操作人 admin_id，auto=系统自动',
  `banned_at`      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '封禁时间',
  `expire_at`      DATETIME        NOT NULL DEFAULT '9999-12-30 23:59:59' COMMENT '解封时间，9999-12-30=永久封禁',
  `unbanned_at`    DATETIME        NOT NULL DEFAULT '1970-01-01 00:00:00' COMMENT '实际解封时间，1970-01-01=未解封',
  `unbanned_by`    CHAR(8)         NOT NULL DEFAULT ''     COMMENT '解封操作人',
  `unban_reason`   VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT '解封原因/备注',
  `status`         TINYINT         NOT NULL DEFAULT 1      COMMENT '1=生效中 0=已解除 2=已过期',
  `source`         VARCHAR(32)     NOT NULL DEFAULT 'manual' COMMENT '来源：manual/auto/report',
  `ref_id`         BIGINT UNSIGNED NOT NULL DEFAULT 0      COMMENT '关联工单/举报记录ID',
  PRIMARY KEY (`id`),
  KEY `idx_account_status` (`account_id`, `status`),
  KEY `idx_expire` (`status`, `expire_at`),
  CONSTRAINT `fk_ban_account`
    FOREIGN KEY (`account_id`) REFERENCES `account`(`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_0900_ai_ci
  COMMENT='账号封禁记录';
```

### 3.3 字段说明

| 字段 | 类型 | 语义 | 约定 |
| --- | --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 封禁记录主键 | 自增 |
| `account_id` | `BIGINT UNSIGNED` | 被封账号，引用 `account.id` | 外键，级联删除 |
| `reason` | `VARCHAR(255)` | 封禁原因，对外展示 | 自定义原因码时必填 |
| `reason_code` | `SMALLINT UNSIGNED` | 原因分类码 | 见 [§3.4.4](#344-reasoncode-原因码) |
| `ban_type` | `TINYINT UNSIGNED` | 封禁维度 | 见 [§3.4.1](#341-bantype-封禁维度) |
| `banned_by` | `CHAR(8)` | 操作人 `admin_id`；`auto` 表示系统自动 | 默认 `auto` |
| `banned_at` | `DATETIME` | 封禁时间 | 默认当前时间 |
| `expire_at` | `DATETIME` | 解封时间；`9999-12-30 23:59:59` 表示永久 | 不是 `NULL` |
| `unbanned_at` | `DATETIME` | 实际解封时间；`1970-01-01 00:00:00` 表示未解封 | 不是 `NULL` |
| `unbanned_by` | `CHAR(8)` | 解封操作人；自动过期写 `system` | 默认空串 |
| `unban_reason` | `VARCHAR(255)` | 解封原因/备注 | 默认空串 |
| `status` | `TINYINT` | 记录状态 | 见 [§3.4.2](#342-banstatus-状态) |
| `source` | `VARCHAR(32)` | 来源 `manual/auto/report` | 见 [§3.4.3](#343-bansource-来源) |
| `ref_id` | `BIGINT UNSIGNED` | 关联工单/举报记录 ID | `0` 表示无关联 |

### 3.4 枚举与状态机

#### 3.4.1 `ban_type` 封禁维度

| 值 | 枚举 | 含义 |
| --- | --- | --- |
| 0 | `All` | 全站封禁 |
| 1 | `Comment` | 禁止评论 |
| 2 | `Post` | 禁止发帖 |
| 3 | `PrivateMessage` | 禁止私信 |
| 4 | `Login` | 禁止登录 |
| 5 | `ProfileEdit` | 禁止修改资料 |
| 6 | `Upload` | 禁止上传附件/图片 |
| 7 | `ReadOnly` | 只读模式 |

#### 3.4.2 `status` 状态

| 值 | 枚举 | 含义 | 判定 |
| --- | --- | --- | --- |
| 1 | `Active` | 生效中 | `status=1 AND expire_at > NOW()` |
| 0 | `Revoked` | 已解除 | 管理员执行解封 |
| 2 | `Expired` | 已过期 | 到期且被过期兜底扫描 |

#### 3.4.3 `source` 来源

| 值 | 枚举 | 含义 |
| --- | --- | --- |
| `manual` | `Manual` | 管理员手动封禁 |
| `auto` | `Auto` | 系统自动封禁 |
| `report` | `Report` | 举报触发封禁 |

#### 3.4.4 `reason_code` 原因码

<details>
<summary>展开完整原因码映射（1xxx 内容违规 / 2xxx 行为违规 / 3xxx 账号安全 / 4xxx 系统自动 / 5xxx 其他）</summary>

| 值 | 枚举 | 默认文案 | 备注 |
| --- | --- | --- | --- |
| 0 | `Unspecified` | （自定义原因） | 展示 `reason` 原文 |
| 1001 | `SpamAd` | 发布广告或垃圾信息 | |
| 1002 | `IllegalContent` | 发布违法违规内容 | |
| 1003 | `Porn` | 发布色情低俗内容 | |
| 1004 | `Violence` | 发布暴力或血腥内容 | |
| 1005 | `Political` | 发布不当政治言论 | |
| 1006 | `Copyright` | 侵犯他人知识产权 | |
| 1007 | `Privacy` | 泄露他人隐私信息 | |
| 1008 | `Misinformation` | 传播虚假信息/谣言 | |
| 2001 | `PersonalAttack` | 人身攻击或辱骂 | |
| 2002 | `Harassment` | 骚扰他人 | |
| 2003 | `HateSpeech` | 仇恨言论 | |
| 2004 | `Impersonation` | 冒充他人或官方 | |
| 2005 | `RepeatViolation` | 多次违规屡教不改 | |
| 2006 | `MaliciousReport` | 恶意举报/滥用举报 | |
| 2007 | `Trolling` | 恶意引战/灌水刷屏 | |
| 3001 | `AccountTheft` | 账号被盗风险 | |
| 3002 | `SharedAccount` | 账号共享/买卖 | |
| 3003 | `BotActivity` | 疑似机器人/脚本行为 | |
| 3004 | `SecurityRisk` | 账号存在安全风险 | |
| 4001 | `AutoRuleHit` | 触发系统自动风控规则 | 仅允许 `auto` / `report` |
| 4002 | `ReportThreshold` | 被多人举报且核实 | 仅允许 `auto` / `report` |
| 5001 | `UserRequest` | 用户主动申请 | |
| 5002 | `Inactive` | 长期未登录清理 | |
| 5003 | `Other` | 其他违规行为 | |
| 5999 | `ManualCustom` | （管理员自定义） | 展示 `reason` 原文 |

</details>

#### 3.4.5 状态机

```
INSERT（status=1，expire_at=未来时间或 9999-12-30）
  │
  ├─ 到期后被 expire_outdated() 扫描 → status=2（已过期），unbanned_at=expire_at
  ├─ 管理员执行 unban()             → status=0（已解除），unbanned_at=NOW()
  └─ 未到期                         → 保持 status=1（生效中）
```

关键常量：

- 永久封禁：`expire_at = '9999-12-30 23:59:59'`（`PERMANENT_EXPIRE`）。
- 未解封：`unbanned_at = '1970-01-01 00:00:00'`（`NOT_UNBANNED`）。

### 3.5 业务操作 SQL

#### 3.5.1 新增封禁记录

**用途**：写入一条封禁记录，`status` 显式置为 1（生效中）。  
**实现**：`BlacklistRepository::insert`

```sql
-- 用途：新增封禁记录；banned_at 使用默认当前时间，status 显式写入 1
INSERT INTO blacklist
  (account_id, reason, reason_code, ban_type, banned_by, expire_at, `status`, source, ref_id)
VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
```

**参数**

| 序号 | 占位符 | 字段 | 含义 |
| --- | --- | --- | --- |
| 1 | `?` | `account_id` | 被封账号内部主键 |
| 2 | `?` | `reason` | 封禁原因原文 |
| 3 | `?` | `reason_code` | 原因分类码 |
| 4 | `?` | `ban_type` | 封禁维度 |
| 5 | `?` | `banned_by` | 操作人 `admin_id` 或 `auto` |
| 6 | `?` | `expire_at` | 解封时间 |
| 7 | `?` | `status` | 固定为 `1` |
| 8 | `?` | `source` | `manual` / `auto` / `report` |
| 9 | `?` | `ref_id` | 关联工单/举报 ID，无关联写 `0` |

**核心字段**：`account_id`、`ban_type`、`expire_at`、`status`、`source`。  
**返回**：受影响行数大于 0 返回 `true`。  
**业务约束**：

- 同一账号同一维度重复封禁的幂等判断由业务层完成（Repository 中已注释：应由业务层判断）。
- `reason_code` 与 `source` 的一致性、自定义原因的 `reason` 非空校验由 `ban::ValidateBanInput` 完成。

#### 3.5.2 幂等检查：某维度是否存在生效记录

**用途**：判断账号在指定维度是否已有生效中的封禁，避免重复写入。  
**实现**：`BlacklistRepository::exists_active`

```sql
-- 用途：检查某账号在某维度是否存在生效记录（status=1 且未到期）
SELECT id
FROM blacklist
WHERE account_id = ? AND status = ? AND expire_at > NOW() AND ban_type = ?;
```

**参数**：`?1 = account_id`；`?2 = 1`（`Active`）；`?3 = ban_type`。  
**核心字段**：`account_id`、`status`、`expire_at`、`ban_type`。  
**返回**：存在生效记录返回 `true`。  
**注意**：该检查**仅按维度匹配**，不包含 `ban_type=0`（全站封禁）对其他维度的覆盖；全站封禁的拦截由鉴权中间件单独处理。

#### 3.5.3 解除封禁（全站，`type = All`）

**用途**：解除该账号所有生效中的封禁记录。  
**实现**：`BlacklistRepository::unban`（`type == ban::BanType::All` 分支）

```sql
-- 用途：解除账号全部生效中的封禁记录
UPDATE blacklist
SET unbanned_at = NOW(), unbanned_by = ?, unban_reason = ?, `status` = ?
WHERE account_id = ? AND status = ? AND expire_at > NOW();
```

**参数**

| 序号 | 占位符 | 绑定值 |
| --- | --- | --- |
| 1 | `?` | 解封操作人 `operator_id` |
| 2 | `?` | 解封原因 `unban_reason` |
| 3 | `?` | `0`（`Revoked`） |
| 4 | `?` | `account_id` |
| 5 | `?` | `1`（`Active`） |

**核心字段**：`unbanned_at`、`unbanned_by`、`unban_reason`、`status`。  
**返回**：实际更新的行数。

#### 3.5.4 解除封禁（指定维度）

**用途**：仅解除账号在指定 `ban_type` 上的生效封禁。  
**实现**：`BlacklistRepository::unban`（`type != ban::BanType::All` 分支）

```sql
-- 用途：解除账号在指定维度上的生效封禁
UPDATE blacklist
SET unbanned_at = NOW(), unbanned_by = ?, unban_reason = ?, `status` = ?
WHERE account_id = ? AND ban_type = ? AND status = ? AND expire_at > NOW();
```

**参数**

| 序号 | 占位符 | 绑定值 |
| --- | --- | --- |
| 1 | `?` | 解封操作人 `operator_id` |
| 2 | `?` | 解封原因 `unban_reason` |
| 3 | `?` | `0`（`Revoked`） |
| 4 | `?` | `account_id` |
| 5 | `?` | `ban_type` |
| 6 | `?` | `1`（`Active`） |

**核心字段**：`ban_type`、`unbanned_at`、`unbanned_by`、`unban_reason`、`status`。  
**返回**：实际更新的行数。  
**注意**：只处理「生效中且未到期」的记录，已过期记录不会被改写为已解除。

#### 3.5.5 过期兜底：批量置为已过期

**用途**：将已到期但仍为 `status=1` 的记录批量置为 `status=2`（已过期），并记录自动解封信息。  
**实现**：`BlacklistRepository::expire_outdated`

```sql
-- 用途：扫描已到期但仍生效的记录，批量置为已过期
UPDATE blacklist
SET unbanned_at = expire_at, `status` = ?, unbanned_by = ?, unban_reason = ?
WHERE `status` = ? AND expire_at < NOW();
```

**参数**

| 序号 | 占位符 | 绑定值 |
| --- | --- | --- |
| 1 | `?` | `2`（`Expired`） |
| 2 | `?` | `"system"` |
| 3 | `?` | `"到期自动解除"` |
| 4 | `?` | `1`（`Active`） |

**核心字段**：`expire_at`、`unbanned_at`、`unbanned_by`、`unban_reason`、`status`。  
**返回**：受影响行数；数据库异常时记录日志并返回 `0`。  
**注意**：该语句尚未接入定时任务，需由调度器周期性调用。

#### 3.5.6 查询账号是否处于封禁状态

**用途**：判断账号是否存在任一维度的生效封禁。  
**实现**：`BlacklistRepository::is_blacklisted`

```sql
-- 用途：判断账号是否存在任一维度的生效封禁
SELECT id
FROM blacklist
WHERE account_id = ? AND status = ? AND expire_at > NOW();
```

**参数**：`?1 = account_id`；`?2 = 1`（`Active`）。  
**核心字段**：`account_id`、`status`、`expire_at`。  
**返回**：存在任一生效记录返回 `true`。

#### 3.5.7 查询某维度的生效详情

**用途**：读取账号在指定维度上的生效封禁记录，用于展示原因、到期时间等。  
**实现**：`BlacklistRepository::get_active`

```sql
-- 用途：查询账号在某维度上的生效封禁详情
SELECT *
FROM blacklist
WHERE account_id = ? AND status = ? AND expire_at > NOW() AND ban_type = ?;
```

**参数**：`?1 = account_id`；`?2 = 1`（`Active`）；`?3 = ban_type`。  
**核心字段**：全部字段，映射为 `BlacklistEntry`。  
**返回**：命中返回单条 `BlacklistEntry`；未命中返回 `std::nullopt`。  
**注意**：使用 `SELECT *`，字段变更时需同步 `read_entry` 的映射逻辑。

#### 3.5.8 查询账号封禁历史

**用途**：查询账号全部封禁记录（含已解除、已过期），按封禁时间倒序。  
**实现**：`BlacklistRepository::get_history`

```sql
-- 用途：查询账号全部封禁历史，按封禁时间倒序
SELECT *
FROM blacklist
WHERE account_id = ?
ORDER BY banned_at DESC;
```

**参数**：`?1 = account_id`。  
**核心字段**：`account_id`、`banned_at`、`status`。  
**返回**：`BlacklistEntry` 列表。  
**注意**：当前实现查询后返回空结果对象，已读取的数据未被返回；使用该接口前需参考 [database.md](./database.md) 中的已知缺陷说明。

#### 3.5.9 条件分页查询

**用途**：后台按原因码、封禁维度、时间范围、状态、账号等条件组合查询，分页返回。  
**实现**：`BlacklistRepository::search`

```sql
-- 用途：按可选条件组合查询封禁记录，分页并按封禁时间倒序
SELECT *
FROM blacklist
[WHERE <条件 1> AND <条件 2> ...]
ORDER BY banned_at DESC
LIMIT ? OFFSET ?;
```

条件片段按传入顺序拼接：

| 过滤字段 | 条件片段 | 说明 |
| --- | --- | --- |
| `filter.reason` | `reason_code=?` | 原因码精确匹配 |
| `filter.type` | `ban_type=?` | 封禁维度精确匹配 |
| `filter.from` | `banned_at>=?` | 封禁时间下界 |
| `filter.to` | `banned_at<=?` | 封禁时间上界 |
| `filter.status = Active` | `` `status`=1 AND expire_at > NOW() `` | 生效中 |
| `filter.status = Expired` | ``(`status`=2 OR (`status`=1 AND expire_at <= NOW()))`` | 已过期（含未扫描的到期记录） |
| `filter.status = Revoked` | `` `status`=0 `` | 已解除 |
| `filter.account_id` | `account_id =?` | 指定账号 |

参数绑定顺序：

| 顺序 | 占位符 | 绑定值 |
| --- | --- | --- |
| 1 | `?` | `filter.reason` |
| 2 | `?` | `filter.type` |
| 3 | `?` | `filter.from` |
| 4 | `?` | `filter.to` |
| 5 | `?` | `filter.account_id` |
| 6 | `?` | `page_size`（`LIMIT`） |
| 7 | `?` | `(page - 1) * page_size`（`OFFSET`） |

**核心字段**：`reason_code`、`ban_type`、`banned_at`、`status`、`expire_at`、`account_id`。  
**返回**：`PageResult`（记录列表 + 总数）。  
**实现注意**：

- 条件拼接使用 `if (filter.account_id)`，而参数绑定使用 `if (filter.account_id != 0)`；当 `account_id` 为 `nullopt` 时后者仍成立，会解引用空 optional，属于已知缺陷。
- 与 `get_history` 相同，当前查询结果未被返回。

### 3.6 索引与查询匹配

| 查询场景 | 使用索引 | 说明 |
| --- | --- | --- |
| `account_id + status + expire_at + ban_type` | `idx_account_status` | `ban_type` 需回表过滤 |
| `status + expire_at`（过期扫描） | `idx_expire` | 覆盖过期兜底 |
| `banned_at` 范围 + 排序 | 无专用索引 | 数据量大时建议新增 `idx_banned_at` |
| `reason_code` / `ban_type` 过滤 | 无专用索引 | 按需增加 |

### 3.7 与历史草稿的差异（已消除）

`db_sqls.md` 的早期版本存在语法错误，已于 2026-11-24 按 Repository 实现校正：

1. 条件关键字 `WHILE` → `WHERE`。
2. 指定维度解封补齐 `status=1 AND expire_at > NOW()`。
3. 过期兜底补写 `unbanned_by='system'` / `unban_reason='到期自动解除'`。
4. `refresh_tokens` 插入列名 `expire_at` → `expires_at`。
5. `passward_hash` → `password_hash`。

校正前的原文见[附录 B](#附录-bdb_sqlsmd-原始-sql-存档)，差异汇总见[附录 A](#附录-设计稿与实现差异)。

---

## 4. 令牌（Token）模块

### 4.1 模块职责

- `refresh_token`：**只在吊销时**把哈希写入 MySQL，用于拒绝旧 token；有效 token 不落库。
- `access_token`：仅写入 Redis，设置 TTL，不落库。
- 读取 refresh token 时采用 Cache-Aside：先查 Redis，未命中再查数据库并回写缓存。

### 4.2 表结构（`refresh_tokens`）

```sql
CREATE TABLE `refresh_tokens`(
`id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'token记录主键',
`token_hash`    CHAR(64)        NOT NULL                COMMENT 'HMAC-SHA256 + Base64URL（43字符）',
`expires_at`    DATETIME        NOT NULL                COMMENT '被吊销 token 的原过期时间',
PRIMARY KEY (`id`),
UNIQUE KEY (`token_hash`)
)ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_0900_ai_ci
  COMMENT='记录未过期的旧refresh_token';
```

**字段说明**

| 字段 | 类型 | 语义 | 代码使用 |
| --- | --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 记录主键 | 自增 |
| `token_hash` | `CHAR(64)` | refresh token 的吊销哈希：HMAC-SHA256 后 Base64URL，43 字符 | 唯一索引；查询键 |
| `expires_at` | `DATETIME` | 被吊销 token 的原过期时间 | 由 `exp_at` 经 `timestamp_to_string` 转换（UTC） |

### 4.3 业务操作 SQL

#### 4.3.1 查询 refresh token 是否存在

**用途**：判断 refresh token 是否**已被吊销**（存在即拒绝）。  
**实现**：`TokenRepository::find_refresh_token`

```sql
-- 用途：按哈希值查询 refresh token 是否存在
SELECT id
FROM refresh_tokens
WHERE token_hash = ?;
```

**参数**：`?1 = token_hash`，refresh token 的哈希值。  
**核心字段**：`token_hash`（唯一索引）。  
**返回**：存在返回 `true`。  
**缓存策略**：

1. 先以 `token_hash` 为 key 查询 Redis；命中直接返回 `true`。
2. Redis 不可用或未命中时降级查询数据库。
3. 数据库命中后，以 best-effort 方式回写 Redis，TTL 为 `refresh_ttl_s`。
4. 缓存写入失败不影响本次数据库命中结果。

**注意**：该查询**只判断记录是否存在，不校验 `expires_at`**；过期判定由上层令牌校验逻辑负责。

#### 4.3.2 持久化 refresh token

**用途**：**吊销** refresh token 时写入哈希与原过期时间，后续查询命中即拒绝。  
**实现**：`TokenRepository::insert_refresh_token`

```sql
-- 用途：持久化 refresh token 的哈希与过期时间
INSERT INTO refresh_tokens (token_hash, expires_at)
VALUES (?, ?);
```

**参数**

| 序号 | 占位符 | 字段 | 含义 |
| --- | --- | --- | --- |
| 1 | `?` | `token_hash` | token 哈希值 |
| 2 | `?` | `expires_at` | 过期时间，由 `utils::timestamp_to_string` 转换 |

**核心字段**：`token_hash`、`expires_at`。  
**返回**：受影响行数大于 0 返回 `true`。  
**注意**：当前没有配套的过期记录清理 SQL；过期记录会保留在表中，需另行归档或清理。

#### 4.3.3 access token 缓存操作（非 SQL）

`TokenRepository::find_access_token` / `insert_access_token` 只操作 Redis，不执行 SQL：

| 操作 | Redis 键 | 值 | TTL |
| --- | --- | --- | --- |
| 写入 | `token_hash` | `token_hash` | `access_ttl_s` |
| 查询 | `token_hash` | 是否存在 | 由写入时设置 |

**返回**：Redis 不可用或操作异常时返回 `false`，不抛数据库异常。

### 4.4 模块注意事项

- DDL 列名为 `expires_at`，代码按此列写入；`db_sqls.md` 已于 2026-11-24 校正，历史差异见[附录 A](#附录-设计稿与实现差异)。
- `token_hash` 上有唯一索引；同一 token 重复吊销会触发唯一键冲突（当前代码未单独处理）。
- 只有被吊销的 `refresh_token` 落库；`access_token` 完全依赖 Redis，Redis 故障会导致 access token 吊销检查 fail-open。

---

## 5. SQL 与代码映射总表

| 序号 | 业务操作 | Repository 方法 | SQL 类型 | 表 | 章节 |
| --- | --- | --- | --- | --- | --- |
| 1 | 按登录账号查用户 | `AccountRepository::find_account_by_user_id` | `SELECT` | `account` | [§1.3.1](#131-按登录账号查询用户凭据) |
| 2 | 按公开 ID 查用户 | `AccountRepository::find_account_by_public_id` | `SELECT` | `account` | [§1.3.2](#132-按公开-id-查询用户凭据) |
| 3 | 创建用户 | `AccountRepository::insert_account_by_user_id` | `INSERT` | `account` | [§1.3.3](#133-创建用户账号) |
| 4 | 注销用户 | `AccountRepository::delete_account_by_public_id` | `DELETE` | `account` | [§1.3.4](#134-按公开-id-删除用户账号) |
| 5 | 查管理员凭据 | `AdminAccountRepository::find_admin_account_by_admin_id` | `SELECT` | `admin_account` | [§2.3.1](#231-按管理员账号查询凭据) |
| 6 | 创建管理员 | `AdminAccountRepository::inisert_admin_account` | `INSERT` | `admin_account` | [§2.3.2](#232-创建管理员账号) |
| 7 | 新增封禁 | `BlacklistRepository::insert` | `INSERT` | `blacklist` | [§3.5.1](#351-新增封禁记录) |
| 8 | 幂等检查 | `BlacklistRepository::exists_active` | `SELECT` | `blacklist` | [§3.5.2](#352-幂等检查某维度是否存在生效记录) |
| 9 | 全站解封 | `BlacklistRepository::unban`（All） | `UPDATE` | `blacklist` | [§3.5.3](#353-解除封禁全站type--all) |
| 10 | 指定维度解封 | `BlacklistRepository::unban`（非 All） | `UPDATE` | `blacklist` | [§3.5.4](#354-解除封禁指定维度) |
| 11 | 过期兜底 | `BlacklistRepository::expire_outdated` | `UPDATE` | `blacklist` | [§3.5.5](#355-过期兜底批量置为已过期) |
| 12 | 是否被封禁 | `BlacklistRepository::is_blacklisted` | `SELECT` | `blacklist` | [§3.5.6](#356-查询账号是否处于封禁状态) |
| 13 | 维度生效详情 | `BlacklistRepository::get_active` | `SELECT` | `blacklist` | [§3.5.7](#357-查询某维度的生效详情) |
| 14 | 封禁历史 | `BlacklistRepository::get_history` | `SELECT` | `blacklist` | [§3.5.8](#358-查询账号封禁历史) |
| 15 | 条件分页查询 | `BlacklistRepository::search` | `SELECT` | `blacklist` | [§3.5.9](#359-条件分页查询) |
| 16 | 校验 refresh token | `TokenRepository::find_refresh_token` | `SELECT` | `refresh_tokens` | [§4.3.1](#431-查询-refresh-token-是否存在) |
| 17 | 写入 refresh token | `TokenRepository::insert_refresh_token` | `INSERT` | `refresh_tokens` | [§4.3.2](#432-持久化-refresh-token) |
| 18 | access token 缓存 | `TokenRepository::find_access_token` / `insert_access_token` | Redis | — | [§4.3.3](#433-access-token-缓存操作非-sql) |

---

## 附录 A：设计稿与实现差异

> 原则：主文档业务 SQL 以 Repository 实现为准；以下差异已于 2026-11-24 在 `db_sqls.md` 中修正，保留记录用于追溯。

| 编号 | 位置 | 校正前原文 | Repository 实现 | 处理状态 |
| --- | --- | --- | --- | --- |
| D1 | `account` 插入 | `INSERT INTO account (public_id,user_id,passward_hash,salt,iterations) ...` | `password_hash` | ✅ 已修正 `db_sqls.md` |
| D2 | 封禁幂等查询 | `... WHILE account_id=? AND status=1 ...` | `... WHERE account_id=? AND status=? ...` | ✅ 已修正 |
| D3 | 指定维度解封 | `... WHILE account_id=? AND ban_type=?;` | 增加 `status=? AND expire_at > NOW()` | ✅ 已修正 |
| D4 | 过期兜底 | `SET unbanned_at=expire_at, status=2 ...` | 额外写入 `unbanned_by='system'`、`unban_reason='到期自动解除'` | ✅ 已修正 |
| D5 | 全站解封 | `... WHILE account_id=? AND ban_type=?;` | `WHERE account_id=? AND status=? AND expire_at > NOW()` | ✅ 已修正 |
| D6 | `refresh_tokens` 插入 | `INSERT INTO refresh_tokens (token_hash,expire_at) ...` | `INSERT INTO refresh_tokens (token_hash,expires_at) ...` | ✅ 已修正 |

---

## 附录 B：db_sqls.md 原始 SQL 存档

> 以下内容为 `db_sqls.md` **校正前**的原始业务 SQL 存档，未做语法修正，仅用于追溯。当前可执行版本以 [db_sqls.md](./db_sqls.md) 为准。

```sql
-- ========== 用户账号 ==========
DELETE FROM account WHERE public_id=?;

INSERT INTO account (public_id,user_id,passward_hash,salt,iterations) VALUES(?,?,?,?,?);

-- ========== 账号封禁 ==========
INSERT INTO blacklist (account_id, reason, reason_code, ban_type, banned_by, expire_at,`status`,source,ref_id) VALUES(?,?,?,?,?,?,?,?,?);

SELECT id FROM blacklist WHILE account_id=? AND status=1 AND  expire_at > NOW() AND ban_type=?;

UPDATE blacklist SET unbanned_at=NOW(),unbanned_by=?,unban_reason=?,`status`=0 WHILE account_id=? AND ban_type=?;

UPDATE blacklist SET unbanned_at=expire_at,`status`=2 WHILE `status`=1 AND expire_at < NOW();

-- ========== 令牌 ==========
SELECT id FROM refresh_tokens WHERE token_hash=?;

INSERT INTO refresh_tokens (token_hash,expire_at) VALUES (?,?);
```

---

## 维护约定

1. 新增或修改表结构时，必须同步更新：`docs/db_sqls.md`、本文对应模块的 DDL 与字段表、Repository 的 SELECT/INSERT 列表、记录结构体。
2. 新增业务 SQL 时，按「模块 → 操作」追加到对应章节，并在[§5 映射总表](#5-sql-与代码映射总表)登记。
3. 若实现与 `db_sqls.md` 再次出现差异，追加到[附录 A](#附录-设计稿与实现差异)，不得直接改写附录 B 的原始存档。
