# Auth 
### 1.账号表
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

### 2. 账号封禁表
CREATE TABLE `blacklist` (
  `id`             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '封禁记录主键',
  `account_id`     BIGINT UNSIGNED NOT NULL                COMMENT '被封账号，引用 account.id',
  `reason`         VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT '封禁原因（对外展示）',
  `reason_code`    SMALLINT UNSIGNED NOT NULL DEFAULT 0    COMMENT '原因分类码，便于统计/自动化',
  `ban_type` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=全站 1=禁评论 2=禁发帖 ...',
  `banned_by`      BIGINT UNSIGNED NULL                    COMMENT '操作人 admin.id，NULL=系统自动',
  `banned_at`      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '封禁时间',
  `expire_at`      DATETIME        NULL                    COMMENT '解封时间，NULL=永久封禁',
  `unbanned_at`    DATETIME        NULL                    COMMENT '实际解封时间，NULL=未解封',
  `unbanned_by`    BIGINT UNSIGNED NULL                    COMMENT '解封操作人',
  `unban_reason`   VARCHAR(255)    NULL                    COMMENT '解封原因/备注',
  `status`         TINYINT         NOT NULL DEFAULT 1      COMMENT '1=生效中 0=已解除 2=已过期',
  `source`         VARCHAR(32)     NOT NULL DEFAULT 'manual' COMMENT '来源：manual/auto/report',
  `ref_id`         BIGINT UNSIGNED NULL                    COMMENT '关联工单/举报记录ID',
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

  