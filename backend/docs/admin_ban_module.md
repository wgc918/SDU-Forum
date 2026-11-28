# 管理员封禁管理模块 开发文档

> 版本：v1.0｜修订：2026-11-28

---

## 一、目标

本模块提供管理端封禁能力，并向认证/业务模块提供只读判定入口：

- 封禁：按维度对账号封禁，支持临时与永久；
- 解除封禁：按维度解除，`All` 解除全部；
- 封禁历史：单用户封禁记录分页查询；
- 黑名单：全局封禁记录分页与条件筛选；
- 判定入口：供认证中间件与业务模块按 `account_id` + 维度查询生效封禁。

本模块只负责**封禁记录与判定**。账号生命周期由用户认证模块负责；管理员身份由管理员认证模块提供。

---

## 二、数据模型

### 2.1 `blacklist`（封禁记录表）

| 字段 | 类型 | 描述 |
| --- | --- | --- |
| `id` | `BIGINT UNSIGNED` | 主键自增 |
| `account_id` | `BIGINT UNSIGNED` | 被封账号内部 ID，外键引用 `account.id` |
| `reason` | `VARCHAR(255)` | 展示原因；标准原因码存标准文案，自定义原因码存原文，默认空串 |
| `reason_code` | `SMALLINT UNSIGNED` | 原因分类码，见 2.2；默认 0（自定义） |
| `ban_type` | `TINYINT UNSIGNED` | 封禁维度，见 2.2 |
| `banned_by` | `CHAR(8)` | 操作人 `admin_id`；系统写入 `auto`，默认 `auto` |
| `banned_at` | `DATETIME` | 封禁时间，默认 `CURRENT_TIMESTAMP` |
| `expire_at` | `DATETIME` | 解封时间；永久封禁为 `9999-12-30 23:59:59`，非空 |
| `unbanned_at` | `DATETIME` | 实际解封/过期时间；未解封为 `1970-01-01 00:00:00`，非空 |
| `unbanned_by` | `CHAR(8)` | 解封操作人 `admin_id`；自动过期写 `system`，默认空串 |
| `unban_reason` | `VARCHAR(255)` | 解封原因/备注，默认空串 |
| `status` | `TINYINT` | 1=生效中，0=已解除，2=已过期，默认 1 |
| `source` | `VARCHAR(32)` | `manual` / `auto` / `report` |
| `ref_id` | `BIGINT UNSIGNED` | 关联举报单/工单 ID，0=无关联 |

索引：`PRIMARY KEY (id)`、`KEY idx_account_status (account_id, status)`、`KEY idx_expire (status, expire_at)`、`CONSTRAINT fk_ban_account FOREIGN KEY (account_id) REFERENCES account(id) ON DELETE CASCADE ON UPDATE CASCADE`。

设计说明：

- **单表而非“生效表 + 历史表”**：状态位区分生效/解除/过期，查询时按状态与时间过滤，避免双表同步与迁移；
- **`ban_type = All` 不展开成多条记录**：全站封禁用一条记录表达所有维度的并集，判定时先看 `All`，再看具体维度；
- **`banned_by` / `unbanned_by` 存管理员 ID 字符串**：作为审计快照，不建外键指向管理员表，避免管理员账号变更影响历史记录；
- **哨兵值代替 NULL**：`expire_at` 永久值用 `9999-12-30 23:59:59`，`unbanned_at` 未解封值用 `1970-01-01 00:00:00`，简化比较与序列化；
- **账号与封禁记录**：用户注销为软删除（`account.deleted_at` 非空），`account` 行保留，封禁记录不级联删除，便于治理审计；判定与操作时按无效账号处理；
- **`ref_id` 预留关联**：举报/工单模块上线后写入，不需要改表。

### 2.2 枚举

**封禁维度 `BanType`：**

| 值 | 名称 | 业务含义 |
| --- | --- | --- |
| 0 | `All` | 全站封禁，覆盖所有维度 |
| 1 | `Comment` | 禁止评论 |
| 2 | `Post` | 禁止发帖 |
| 3 | `PrivateMessage` | 禁止私信 |
| 4 | `Login` | 禁止登录（仅限制新登录与刷新，不主动踢出已有会话） |
| 5 | `ProfileEdit` | 禁止修改资料 |
| 6 | `Upload` | 禁止上传附件/图片 |
| 7 | `ReadOnly` | 只读模式 |

**原因码 `ReasonCode`：** 按业务域分段，数值稳定，只增不改：

| 区间 | 含义 | 示例 |
| --- | --- | --- |
| 0 | 自定义原因 | `Unspecified`，配合 `reason` 原文 |
| 1xxx | 内容违规 | 广告、违法、色情、暴力、政治、版权、隐私、谣言 |
| 2xxx | 行为违规 | 人身攻击、骚扰、仇恨、冒充、屡犯、恶意举报、引战 |
| 3xxx | 账号安全 | 盗号、共享账号、机器人、安全风险 |
| 4xxx | 系统自动 | 风控规则命中、举报阈值，仅允许 `auto/report` |
| 5xxx | 其他 | 用户申请、长期不活跃、其他、管理员自定义 |

每个原因码规划元数据：

| 字段 | 说明 |
| --- | --- |
| `display_key` | 前端国际化 key，例如 `ban.reason.spam_ad` |
| `default_text` | 中文兜底文案 |
| `auto_only` | 是否仅允许系统/举报来源 |
| `deprecated` | 是否废弃（保留数值） |

**来源 `BanSource`：** `manual`（人工）/ `auto`（系统）/ `report`（举报）。

**状态 `status`：** `1` 生效中 / `0` 已解除 / `2` 已过期。

校验规则：

1. `reason_code` 必须是已登记的原因码；
2. `ban_type` 必须是已登记的维度；
3. 4xxx 与 `auto_only` 原因只允许 `source = auto/report`；
4. `Unspecified`、`ManualCustom` 必须提供非空 `reason` 原文；
5. `reason` 长度不超过 255。

### 2.3 状态机与生效判定

```
                 unban
   Active(1) ──────────────→ Revoked(0)
       │
       │ expire_at <= NOW()
       ▼
   Expired(2)（可由兜底任务落库，也可仅按时间逻辑判定）
```

约定：

- **生效判定**：一律使用 `status = 1 AND expire_at > NOW()`，禁止只看 `status`；
- **已过期**：`status = 2` 或（`status = 1` 且 `expire_at <= NOW()`）；
- **已解除**：`status = 0`；
- 只有 `Active` 可以转出，`Revoked` 与 `Expired` 为终态；
- 再次封禁不复用旧记录，插入新记录，历史保持完整；
- 兜底任务只把“仍为 1 但已过期”的记录置为 2，不反向修改。

### 2.4 时长与时间计算

| `time_type` | 单位 | 说明 |
| --- | --- | --- |
| 0 | 分钟 | `expire_at = now + time * 60s` |
| 1 | 小时 | `expire_at = now + time * 3600s` |
| 2 | 天 | `expire_at = now + time * 86400s` |
| `0xFFFF` | 永久 | `expire_at = 9999-12-30 23:59:59`，忽略 `time` |

校验：

- `time_type` 只允许 0/1/2/`0xFFFF`；
- 非永久时 `time` 必须大于 0；
- 时间格式统一 `YYYY-MM-DD HH:MM:SS`，按UTC生成，与 MySQL `NOW()` 比较；
- 部署时保证应用与数据库时区一致，避免 `expire_at > NOW()` 偏移。

---

## 三、封禁检查与判定入口

### 3.1 封禁检查分层

| 维度 | 检查位置 | 时机 | 结果 |
| --- | --- | --- | --- |
| `All` | 认证中间件 | 每次 access/refresh 鉴权 | `403 ACCOUNT_BANNED` |
| `All`、`Login` | 登录 Service | 密码校验通过后 | `403 ACCOUNT_BANNED` |
| `Login` | 刷新 Service | refresh 验签通过后 | `403 ACCOUNT_BANNED` |
| 具体维度 | 业务 Service | 评论/发帖/私信/上传等动作前 | 按业务错误处理 |

约定：

- `Login` 只限制“新登录与续期”，不主动失效已有 access token；
- `All` 由认证中间件统一拦截，业务层不再重复判断；登录接口无 token，需在 Service 显式校验 `All`；
- 封禁判断与token校验没有联系，封禁时不会吊销当前的token，是独立的两个维度

### 3.2 只读判定入口

封禁模块对外提供内部只读查询入口，供认证中间件与业务模块调用：

- 入参：`account_id` + `ban_type`；
- 出参：是否生效及生效记录的 `expire_at`；
- 判定条件：`status = 1 AND expire_at > NOW()`；
- 不缓存、不直接暴露 SQL；解除或到期后下一次判定即恢复。

---

## 四、接口设计

### 4.1 接口清单

全部接口要求管理员 access 认证 + 管理员角色校验，请求头携带 `Authorization: Bearer <access_token>`。

| 接口 | 方法 | 路径 | 成功 |
| --- | --- | --- | --- |
| 封禁用户 | POST | `/users/{account_id}/ban` | `204` |
| 解除封禁 | PATCH | `/users/{account_id}/unban` | `204` |
| 封禁历史 | GET | `/users/history` | `200` + 分页 |
| 黑名单 | GET | `/users/blacklist` | `200` + 分页 |

以下路径均以 `/admin/api/v1` 为前缀。操作人身份从 token 读取，请求体不传 `operator_id`；时间字段统一 `YYYY-MM-DD HH:MM:SS`，时间戳字段使用 Unix 秒。

### 4.2 封禁用户

`POST /admin/api/v1/users/{account_id}/ban` ｜ 认证：access 认证 + 角色

路径参数：

| 参数 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `account_id` | uint64 | 是 | 大于 0，被封账号内部 ID |

body：

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `reason_code` | int | 是 | 已登记的 `ReasonCode` |
| `ban_type` | int | 是 | 0–7 的合法 `BanType` |
| `time_type` | int | 是 | 0/1/2/`0xFFFF` |
| `time` | int | 条件 | 非永久时必填且大于 0；永久时忽略 |
| `reason` | string | 条件 | ≤255；自定义原因码时必填 |
| `source` | string | 是 | `manual` / `auto` / `report`，默认 `m |
| `ref_id` | uint64 | 否 | 非负，默认 0 |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 校验目标账号存在（`deleted_at IS NULL`），不存在返回 `404 NOT_FOUND`；
3. 校验原因码、维度、来源一致性、原因文本、时长；
4. 若已有 `All` 生效记录，返回 `409 CONFLICT`；
5. 若同维度已有生效记录，返回 `409 CONFLICT`；
6. 计算 `expire_at`：临时按单位换算，永久使用 `9999-12-30 23:59:59`；
7. 写入 `blacklist`：`status = 1`，`banned_by = 当前管理员`，`source`、`ref_id` 按请求写入；
8. 返回 `204`。

### 4.3 解除封禁

`PATCH /admin/api/v1/users/{account_id}/unban` ｜ 认证：access 认证 + 角色

路径参数：`account_id`（uint64，大于 0）。

body：

| 字段 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `ban_type` | int | 是 | 0–7 的合法 `BanType` |
| `reason` | string | 是 | ≤255，解封原因/备注（前端自定义枚举） |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 校验 `ban_type` 合法、`reason` 长度合法；
3. 查询该账号是否存在生效记录；
4. 无生效记录返回 `404 NOT_FOUND`；
5. 按维度更新：`status = 0`，`unbanned_at = NOW()`，`unbanned_by = 当前管理员`，`unban_reason = reason`；
6. `ban_type = All` 更新该账号全部生效记录，其他值只更新对应维度；
7. 受影响行数为 0 时返回 `404 NOT_FOUND`；
8. 返回 `204`。

语义说明：

- 只有 `All` 封禁时，按具体维度解除不会命中记录，返回 `404`；必须先解除 `All`；
- 单维度或多维度封禁，可用 `All` 一次性全部解除；
- 解除后判定立即恢复，无需等待缓存或 token 过期。

### 4.4 封禁历史

`GET /admin/api/v1/users/history` ｜ 认证：access 认证 + 角色

查询参数：

| 参数 | 类型 | 必填 | 约束 |
| --- | --- | --- | --- |
| `account_id` | uint64 | 是 | 大于 0 |
| `page` | int | 是 | 大于 0 |
| `page_size` | int | 是 | 1–100 |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 校验 `account_id`、分页参数；
3. 查询该账号全部记录（含生效、解除、过期），按 `banned_at DESC` 排序；
4. 返回分页结构；账号不存在或无记录时返回空数组与 `total = 0`，不返回 404。

```json
{
  "data": {
    "page": 1,
    "page_size": 20,
    "total": 2,
    "items": [
      {
        "id": 12,
        "account_id": 123,
        "reason": "发布广告或垃圾信息",
        "reason_code": 1001,
        "ban_type": 0,
        "banned_by": "admin001",
        "banned_at": "2026-11-20 10:00:00",
        "expire_at": "9999-12-30 23:59:59",
        "unbanned_at": "1970-01-01 00:00:00",
        "unbanned_by": "",
        "unban_reason": "",
        "status": 1,
        "source": "manual",
        "ref_id": 0
      }
    ]
  }
}
```

### 4.5 黑名单

`GET /admin/api/v1/users/blacklist` ｜ 认证：access 认证 + 角色

查询参数：

| 参数 | 类型 | 必填 | 默认 | 约束 |
| --- | --- | --- | --- | --- |
| `page` | int | 是 | 无 | 大于0 |
| `page_size` | int | 否 | 20 | 1–100 |
| `reason_code` | int | 条件 | 无 | 已登记的 `ReasonCode` |
| `ban_type` | int | 条件 | 无 | 0–7 的合法 `BanType` |
| `status` | string | 条件 | 无 | `active` / `expired` / `revoked` 或 `1` / `2` / `0` |
| `from` | string | 条件 | 无 | `YYYY-MM-DD HH:MM:SS`，`banned_at` 起始（含） |
| `to` | string | 条件 | 无 | `YYYY-MM-DD HH:MM:SS`，`banned_at` 结束（含） |

关键逻辑：

1. access 认证中间件 + 角色中间件完成校验；
2. 校验分页、原因码、维度、状态、时间格式；
3. 组合筛选条件，取交集；多个条件可组合；
4. 状态条件按语义展开：
   - `active` → `status = 1 AND expire_at > NOW()`；
   - `expired` → `status = 2 OR (status = 1 AND expire_at <= NOW())`；
   - `revoked` → `status = 0`；
5. 按 `banned_at DESC` 排序，返回分页结构；无匹配记录时返回空数组与 `total = 0`。

响应结构同封禁历史。

---

## 六、后续规划

- 举报、工单、申诉流程打通，`ref_id` 关联具体单据；
- 自动风控与规则引擎，补全 `source = auto` 与 4xxx 原因码；
- 用户通知（站内信/邮件）；
