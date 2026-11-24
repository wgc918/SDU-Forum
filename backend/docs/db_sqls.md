-- =============================================================================
-- SDU-Forum 数据库建表与常用 SQL 参考
-- 最后核对：2026-11-24
--
-- 维护约定：
--   1. 本文件是「可直接执行的 DDL 参考」，字段语义与 docs/database.md 必须同步。
--   2. 表名在代码中来自 [auth] 配置，禁止在 Repository 中硬编码。
--   3. banned_by / unbanned_by 为 CHAR(8)，存储 admin_account.admin_id，
--      不是 admin_account.id；超长操作人 ID 会写入失败或截断。
--   4. refresh_tokens 的时间列名是 expires_at（代码按此列写入）。
-- =============================================================================

-- -----------------------------------------------------------------------------
-- 1. 用户账号表
-- -----------------------------------------------------------------------------
CREATE TABLE `account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '内部主键，外键关联用',
  `public_id`     CHAR(24)        NOT NULL                COMMENT '对外公开ID，不可枚举',
  `user_id`       CHAR(12)        NOT NULL                COMMENT '登录账号，业务约定12位数字（代码未强制）',
  `password_hash` CHAR(64)        NOT NULL                COMMENT 'PBKDF2-HMAC-SHA512 hex',
  `salt`          CHAR(32)        NOT NULL                COMMENT '随机盐 hex',
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000 COMMENT 'PBKDF2迭代次数',
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_public_id` (`public_id`),
  UNIQUE KEY `uk_user_id`   (`user_id`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_bin
  COMMENT='用户账号表';

-- 注册（AccountRepository::insert_account_by_user_id）
INSERT INTO account (public_id,user_id,password_hash,salt,iterations) VALUES (?,?,?,?,?);

-- 登录（AccountRepository::find_account_by_user_id）
SELECT id, public_id, password_hash, salt, iterations FROM account WHERE user_id=?;

-- 注销账号（AuthService::logoff 调用；blacklist 按外键级联删除）
DELETE FROM account WHERE public_id=?;

-- -----------------------------------------------------------------------------
-- 2. 管理员账号表
--    注意：admin_id 与 Controller 的长度上限 8 一致。
--    当前 DDL 未声明 admin_id 唯一索引，代码依赖其唯一性，建议补：
--      UNIQUE KEY `uk_admin_id` (`admin_id`)
-- -----------------------------------------------------------------------------
CREATE TABLE `admin_account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '内部主键，外键关联用',
  `admin_id`      CHAR(8)         NOT NULL                COMMENT '登录账号，如 admin001',
  `password_hash` CHAR(64)        NOT NULL                COMMENT 'PBKDF2-HMAC-SHA512 hex',
  `salt`          CHAR(32)        NOT NULL                COMMENT '随机盐 hex',
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000 COMMENT 'PBKDF2迭代次数',
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_bin
  COMMENT='管理员账号表';

-- 管理员登录（AdminAccountRepository::find_admin_account_by_admin_id）
SELECT id, password_hash, salt, iterations FROM admin_account WHERE admin_id=?;

-- 写入管理员（当前仅由脚本/人工执行；代码未绑定 iterations，依赖 DB 默认值）
INSERT INTO admin_account (admin_id,password_hash,salt) VALUES (?,?,?);

-- -----------------------------------------------------------------------------
-- 3. 封禁记录表
-- -----------------------------------------------------------------------------
CREATE TABLE `blacklist` (
  `id`             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '封禁记录主键',
  `account_id`     BIGINT UNSIGNED NOT NULL                COMMENT '被封账号，引用 account.id',
  `reason`         VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT '封禁原因（对外展示）',
  `reason_code`    SMALLINT UNSIGNED NOT NULL DEFAULT 0    COMMENT '原因分类码，便于统计/自动化',
  `ban_type`       TINYINT UNSIGNED NOT NULL DEFAULT 0     COMMENT '0=全站 1=禁评论 2=禁发帖 ...',
  `banned_by`      CHAR(8)         NOT NULL DEFAULT 'auto' COMMENT '操作人 admin_id，auto=系统自动',
  `banned_at`      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '封禁时间',
  `expire_at`      DATETIME        NOT NULL DEFAULT '9999-12-30 23:59:59' COMMENT '解封时间，9999-12-30=永久封禁',
  `unbanned_at`    DATETIME        NOT NULL DEFAULT '1970-01-01 00:00:00' COMMENT '实际解封时间，1970-01-01=未解封',
  `unbanned_by`    CHAR(8)         NOT NULL DEFAULT ''     COMMENT '解封操作人 admin_id',
  `unban_reason`   VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT '解封原因/备注',
  `status`         TINYINT         NOT NULL DEFAULT 1      COMMENT '1=生效中 0=已解除 2=已过期',
  `source`         VARCHAR(32)     NOT NULL DEFAULT 'manual' COMMENT '来源：manual/auto/report',
  `ref_id`         BIGINT UNSIGNED NOT NULL DEFAULT 0      COMMENT '关联工单/举报记录ID，0=无',
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

-- 插入封禁记录（BlacklistRepository::insert；banned_at 依赖 DB 默认值）
INSERT INTO blacklist
  (account_id, reason, reason_code, ban_type, banned_by, expire_at, `status`, source, ref_id)
VALUES (?,?,?,?,?,?,?,?,?);

-- 查询某维度生效记录（生效判定统一：status=1 AND expire_at > NOW()）
SELECT id FROM blacklist
WHERE account_id=? AND `status`=1 AND expire_at > NOW() AND ban_type=?;

-- 解除封禁（type != All：只解除单维度）
UPDATE blacklist
SET unbanned_at=NOW(), unbanned_by=?, unban_reason=?, `status`=0
WHERE account_id=? AND ban_type=? AND `status`=1 AND expire_at > NOW();

-- 解除封禁（type == All：解除该账号全部生效记录）
UPDATE blacklist
SET unbanned_at=NOW(), unbanned_by=?, unban_reason=?, `status`=0
WHERE account_id=? AND `status`=1 AND expire_at > NOW();

-- 过期兜底（BlacklistRepository::expire_outdated；需由定时任务周期调用）
UPDATE blacklist
SET unbanned_at=expire_at, `status`=2, unbanned_by='system', unban_reason='到期自动解除'
WHERE `status`=1 AND expire_at < NOW();

-- -----------------------------------------------------------------------------
-- 4. 旧 refresh_token 吊销表
--    只保存「已吊销」的 refresh token 哈希，不保存有效 token。
-- -----------------------------------------------------------------------------
CREATE TABLE `refresh_tokens` (
  `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'token记录主键',
  `token_hash`  CHAR(64)        NOT NULL                COMMENT 'HMAC-SHA256 + base64url 后的哈希',
  `expires_at`  DATETIME        NOT NULL                COMMENT '原 token 过期时间',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_token_hash` (`token_hash`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_0900_ai_ci
  COMMENT='记录未过期的旧refresh_token';

SELECT id FROM refresh_tokens WHERE token_hash=?;
INSERT INTO refresh_tokens (token_hash,expires_at) VALUES (?,?);
