# 数据库设计与 SQL 规范

> 适用代码：`backend/repositories/`、`docs/db_sqls.md`
> 最后核对：2026-11-24

## 1. 数据库概览

- 引擎：MySQL 8（`utf8mb4`）。
- 连接：`sdu_infra` 的 `dcp::ConnectionPool`，参数来自 `[database]`。
- 表名：全部通过 `AppConfig::auth()` 注入 Repository，**禁止在 SQL 中硬编码**。
- 当前仓库内已有 4 张表的建表 SQL（见 [db_sqls.md](./db_sqls.md)）：

| 表 | 用途 | 表名配置键 | 默认值 |
| --- | --- | --- | --- |
| `account` | 普通用户账号 | `auth.account_table` | `account` |
| `admin_account` | 管理员账号 | `auth.admin_account_table` | `admin_account` |
| `blacklist` | 封禁记录 | `auth.blacklist_table` | `blacklist` |
| `refresh_tokens` | 已吊销 refresh token | `auth.refresh_token_table` | `refresh_tokens` |

## 2. `account` 用户账号表

```sql
CREATE TABLE `account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `public_id`     CHAR(24)        NOT NULL,
  `user_id`       CHAR(12)        NOT NULL,
  `password_hash` CHAR(64)        NOT NULL,
  `salt`          CHAR(32)        NOT NULL,
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000,
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_public_id` (`public_id`),
  UNIQUE KEY `uk_user_id`   (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;
```

| 字段 | 语义 | 代码使用 |
| --- | --- | --- |
| `id` | 内部主键，封禁表外键 | 中间件/Service 的封禁检查、`BlacklistEntry.account_id` |
| `public_id` | 对外唯一 ID，`usr_` + 20 位 Base62 | JWT `sub`、接口返回、注销条件 |
| `user_id` | 登录账号 | `login` / `sign-up` 查询键 |
| `password_hash` | PBKDF2-HMAC-SHA512 派生值 hex（64） | `verify_password`；实际派生 32 字节后转 hex，故长度恰为 64 |
| `salt` | 随机盐 hex（32） | `verify_password` |
| `iterations` | 迭代次数 | 验证时读取，兼容历史值 |
| `created_at` / `updated_at` | 审计时间 | 当前业务未读取 |

注意：

- `CHAR(24)` 与 `usr_` + 20 位一致；修改 `public_id` 生成规则必须同步字段长度。
- `CHAR(12)` 的「12 位数字」只是注释/业务约定，代码只校验非空和 ≤64；超长会在 MySQL 严格模式下插入失败并转成 `DB_ERROR`。
- 注释/历史文档写 PBKDF2-SHA256，实际实现为 **PBKDF2-HMAC-SHA512**（见 [infrastructure.md](./infrastructure.md) §7）。
- 排序规则 `utf8mb4_bin`：`user_id`、`public_id` 大小写敏感。

## 3. `admin_account` 管理员账号表

```sql
CREATE TABLE `admin_account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `admin_id`      CHAR(8)         NOT NULL,
  `password_hash` CHAR(64)        NOT NULL,
  `salt`          CHAR(32)        NOT NULL,
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000,
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;
```

字段语义与 `account` 相同，差异：

- `admin_id` 是 **CHAR(8)**，与 `AdminAuthController` 的 8 字符上限一致。业务需要更长 ID 时必须同时改 Controller 与 DDL。
- 当前 DDL **没有 `admin_id` 唯一索引**，代码依赖其唯一性；建议补 `UNIQUE KEY uk_admin_id (admin_id)`。
- `AdminAccountRepository::inisert_admin_account` 的 INSERT 只写 `admin_id/password_hash/salt`，**不绑定 `iterations`**，因此依赖 DB 默认值 150000（风险 R9）。
- `blacklist.banned_by` / `unbanned_by` 存的是 `admin_id` 字符串，不是本表 `id`。字段注释若写「admin.id」属于历史遗留，以代码为准。
- 管理员**没有**注册、注销接口，账号只能由外部脚本/人工写入（仓库内有 `scripts/create_admin_account.py`）。

## 4. `blacklist` 封禁记录表

```sql
CREATE TABLE `blacklist` (
  `id`             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `account_id`     BIGINT UNSIGNED NOT NULL,
  `reason`         VARCHAR(255)    NOT NULL DEFAULT '',
  `reason_code`    SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `ban_type`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `banned_by`      CHAR(8)         NOT NULL DEFAULT 'auto',
  `banned_at`      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `expire_at`      DATETIME        NOT NULL DEFAULT '9999-12-30 23:59:59',
  `unbanned_at`    DATETIME        NOT NULL DEFAULT '1970-01-01 00:00:00',
  `unbanned_by`    CHAR(8)         NOT NULL DEFAULT '',
  `unban_reason`   VARCHAR(255)    NOT NULL DEFAULT '',
  `status`         TINYINT         NOT NULL DEFAULT 1,
  `source`         VARCHAR(32)     NOT NULL DEFAULT 'manual',
  `ref_id`         BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_account_status` (`account_id`, `status`),
  KEY `idx_expire` (`status`, `expire_at`),
  CONSTRAINT `fk_ban_account` FOREIGN KEY (`account_id`) REFERENCES `account`(`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```

### 4.1 状态机

```
insert(status=1, expire_at=未来或 9999-12-30 23:59:59)
   |
   |-- 到期且被 expire_outdated() 扫描 --> status=2 (Expired), unbanned_at=expire_at
   |-- 管理员 unban()                --> status=0 (Revoked), unbanned_at=NOW()
   `-- 未到期                        --> 保持 status=1
```

生效判定统一：`status=1 AND expire_at > NOW()`。查询时**不要**只看 `status`。

### 4.2 字段约定

| 字段 | 约定 |
| --- | --- |
| `expire_at` | `NOT NULL`；永久封禁写 `9999-12-30 23:59:59`（`PERMANENT_EXPIRE`），DB 默认值也是它 |
| `unbanned_at` | `NOT NULL`；未解封为 `1970-01-01 00:00:00`（哨兵值），不是 NULL |
| `status` | 1 生效 / 0 已解除 / 2 已过期 |
| `banned_by` | `CHAR(8)`，存管理员 `admin_id`；系统自动为 `auto`。超过 8 字符会失败/截断 |
| `unbanned_by` | `CHAR(8)`；管理员解封存 `admin_id`，自动过期写 `system` |
| `source` | 只允许 `manual/auto/report`，由 `ban_source_to_string` 保证 |
| `ref_id` | 0 表示无关联（DB `NOT NULL`，代码写 0） |
| `created_at` 类字段 | `banned_at` 由 DB 默认值写入，`insert` 不显式绑定 |

### 4.3 读取与 JSON 序列化

`BlacklistEntry` 的所有时间/操作人字段都是 `std::string`，`read_entry()` 用 `get_string()` 读取。Controller 的 `optional_to_json(entry.xxx)` 接收的是隐式构造的 `std::optional<std::string>`，**永远有值**，因此：

- `banned_by/expire_at/unbanned_at/unbanned_by/unban_reason` 在 JSON 中**始终是字符串**，不会是 `null`；
- 未解封记录返回 `"unbanned_at": "1970-01-01 00:00:00"`，前端需按哨兵值判断，而不是判 `null`。

### 4.4 索引与查询匹配

| 查询 | 命中索引 |
| --- | --- |
| `account_id + status + expire_at + ban_type` | `idx_account_status` 部分命中，`ban_type` 回表过滤 |
| `status + expire_at`（过期扫描） | `idx_expire` |
| `banned_at` 范围 + 排序 | 无专用索引，数据量大时需新增 `idx_banned_at` |
| `reason_code` / `ban_type` 过滤 | 无专用索引，按需增加 |

### 4.5 外键与删除

- `ON DELETE CASCADE`：注销账号会删除该用户全部封禁历史。
- 若未来需要保留审计记录，必须在注销前归档，或改为软删除并去掉级联。

## 5. `refresh_tokens` 已吊销 refresh token 表

```sql
CREATE TABLE `refresh_tokens` (
  `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `token_hash`  CHAR(64)        NOT NULL,
  `expires_at`  DATETIME        NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_token_hash` (`token_hash`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```

- 只存**已吊销**的 refresh token 哈希；有效 token 不落库。
- 列名是 `expires_at`，`TokenRepository::insert_refresh_token` 按此列写入。
- 代码不按 `expires_at` 清理历史记录；表会随登出/刷新持续增长，需要后续加清理任务。
- 查询键是 `token_hash`，命中即表示「该 token 已被吊销」。

## 6. 实际使用的 SQL 模式

### 6.1 插入封禁

```sql
INSERT INTO blacklist
  (account_id, reason, reason_code, ban_type, banned_by, expire_at, `status`, source, ref_id)
VALUES (?,?,?,?,?,?,?,?,?)
```

`banned_at`、`unbanned_at`、`unbanned_by`、`unban_reason` 依赖 DB 默认值。

### 6.2 查生效记录

```sql
SELECT id FROM blacklist
WHERE account_id=? AND status=? AND expire_at > NOW() AND ban_type=?;
```

### 6.3 解除封禁

```sql
-- type = All
UPDATE blacklist
SET unbanned_at=NOW(), unbanned_by=?, unban_reason=?, `status`=0
WHERE account_id=? AND status=1 AND expire_at > NOW();

-- type != All
UPDATE blacklist
SET unbanned_at=NOW(), unbanned_by=?, unban_reason=?, `status`=0
WHERE account_id=? AND ban_type=? AND status=1 AND expire_at > NOW();
```

### 6.4 过期兜底

```sql
UPDATE blacklist
SET unbanned_at=expire_at, `status`=2, unbanned_by='system', unban_reason='到期自动解除'
WHERE `status`=1 AND expire_at < NOW();
```

> `unbanned_by` 为 `CHAR(8)`，`'system'` 合法，不存在旧文档描述的 BIGINT 写入失败问题。真正的缺陷是**该函数没有接入任何定时任务**（见 [README.md](./README.md) R18）。

### 6.5 条件查询（待修复）

Repository 已构造：

```sql
SELECT * FROM blacklist
WHERE [reason_code=?] [AND ban_type=?] [AND banned_at>=?] [AND banned_at<=?]
  [AND status 条件] [AND account_id=?]
ORDER BY banned_at DESC
LIMIT ? OFFSET ?;
```

`status` 条件展开：

| 状态 | SQL 片段 |
| --- | --- |
| Active | `status=1 AND expire_at > NOW()` |
| Expired | `(status=2 OR (status=1 AND expire_at <= NOW()))` |
| Revoked | `status=0` |

已知缺陷：查询结果被丢弃（R3）；`account_id` 条件与绑定判断不一致（R3b）。

## 7. 时间与时区（重要）

项目存在**两种时间生成方式**，混用会导致偏移：

| 生成函数 | 时区 | 用途 |
| --- | --- | --- |
| `add_time_to_now()`（`services/admin/user_service.cpp`） | `localtime_r`，服务器本地时区 | 封禁 `expire_at`，与 MySQL `NOW()` 比较 |
| `utils::timestamp_to_string()`（`utils/utils.cpp`） | `gmtime_r`，**UTC** | refresh token 吊销记录的 `expires_at` |

结论与约束：

1. 与 `NOW()` 做比较的时间必须使用**本地时区**生成；封禁流程已如此实现。
2. `refresh_tokens.expires_at` 是 UTC 字符串，当前代码不做比较，只作为审计/过期参考；**禁止**直接用它和 `NOW()` 比较。
3. 部署到多时区/容器时，必须统一服务器 `TZ` 与 MySQL `time_zone`，否则 `expire_at > NOW()` 会偏移。
4. `PERMANENT_EXPIRE` 为 `9999-12-30 23:59:59`，避免 `9999-12-31` 的边界问题。

## 8. SQL 开发规范（硬约束）

1. 表名必须来自配置，Repository 构造函数注入。
2. 所有变量使用 `?` 绑定，**禁止字符串拼接用户输入**（表名拼接的是配置值，不是用户输入）。
3. 一条 Repository 方法只执行一条 SQL；跨表一致性由 Service 编排（当前无显式事务）。
4. `sql::SQLException` 必须转 `AppException(DB_ERROR)`；`expire_outdated` 例外，记录日志并返回 0。
5. 查询未命中返回 `nullopt` / 空结果，不抛 `NOT_FOUND`。
6. 时间字符串统一 `YYYY-MM-DD HH:MM:SS`；与 `NOW()` 比较的时间用本地时区生成。
7. 布尔/状态使用显式整数，不使用 MySQL `BOOLEAN`。
8. 新增字段必须同步：`db_sqls.md` → 本文档字段表 → Repository 的 SELECT/INSERT 列表 → `BlacklistEntry` / `AccountRecord` 结构。
9. 修改 `SELECT *` 时改为显式列名，避免表结构变化导致映射错位；现有 `get_active/search/get_history` 的 `SELECT *` 是历史实现。
10. 新增操作人字段时注意 `CHAR(8)` 上限；需要更长标识必须改表并评估历史数据。
11. 迁移脚本必须可重复执行（`IF NOT EXISTS`），并记录在本文件「变更记录」。

## 9. 变更记录

> 每次表结构变更在此追加：日期、变更人、表、变更内容、兼容性说明、回滚方式。

| 日期 | 表 | 变更 | 说明 |
| --- | --- | --- | --- |
| 2026-11-24 | — | 文档建立 | 仅记录现有结构，无 DDL 变更 |
| 2026-11-24 | 全部 | 文档校正 | 按 `db_sqls.md` 校正 `banned_by/unbanned_by` 为 `CHAR(8)`，补充 `refresh_tokens`/`admin_account` 实际 DDL |
| | | | |
