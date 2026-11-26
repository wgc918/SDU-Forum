-- SDU-Forum 集成测试专用表结构（仅 it_ 前缀，禁止触碰业务表）
-- 由 scripts/setup_db.sh 在当前库中执行；执行前会 DROP 同名 it_ 表。

DROP TABLE IF EXISTS `it_blacklist`;
DROP TABLE IF EXISTS `it_refresh_tokens`;
DROP TABLE IF EXISTS `it_account`;
DROP TABLE IF EXISTS `it_admin_account`;

CREATE TABLE `it_account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `public_id`     CHAR(24)        NOT NULL,
  `user_id`       CHAR(12)        NOT NULL,
  `password_hash` CHAR(64)        NOT NULL,
  `salt`          CHAR(32)        NOT NULL,
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000,
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_it_public_id` (`public_id`),
  UNIQUE KEY `uk_it_user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE `it_admin_account` (
  `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `admin_id`      CHAR(8)         NOT NULL,
  `password_hash` CHAR(64)        NOT NULL,
  `salt`          CHAR(32)        NOT NULL,
  `iterations`    INT UNSIGNED    NOT NULL DEFAULT 150000,
  `created_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_it_admin_id` (`admin_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE `it_refresh_tokens` (
  `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `token_hash` CHAR(64)        NOT NULL,
  `expires_at` DATETIME        NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_it_token_hash` (`token_hash`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `it_blacklist` (
  `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `account_id`   BIGINT UNSIGNED NOT NULL,
  `reason`       VARCHAR(255)    NOT NULL DEFAULT '',
  `reason_code`  SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `ban_type`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `banned_by`    CHAR(8)         NOT NULL DEFAULT 'auto',
  `banned_at`    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `expire_at`    DATETIME        NOT NULL DEFAULT '9999-12-30 23:59:59',
  `unbanned_at`  DATETIME        NOT NULL DEFAULT '1970-01-01 00:00:00',
  `unbanned_by`  CHAR(8)         NOT NULL DEFAULT '',
  `unban_reason` VARCHAR(255)    NOT NULL DEFAULT '',
  `status`       TINYINT         NOT NULL DEFAULT 1,
  `source`       VARCHAR(32)     NOT NULL DEFAULT 'manual',
  `ref_id`       BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_it_account_status` (`account_id`, `status`),
  KEY `idx_it_expire` (`status`, `expire_at`),
  CONSTRAINT `fk_it_ban_account` FOREIGN KEY (`account_id`) REFERENCES `it_account` (`id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
