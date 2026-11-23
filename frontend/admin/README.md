# SDU Forum Admin

山东大学校园论坛「SDU Forum」后台管理系统前端原型。纯 HTML / CSS / JavaScript 实现，无构建步骤、无外部 CDN、无第三方框架。

## 运行方式

- 直接双击 `login.html` 即可在浏览器中预览（推荐使用 Chrome / Edge）。
- 也可以在本目录启动静态服务器：

```bash
python -m http.server 8765
# 打开 http://localhost:8765/login.html
```

**演示账号：`admin`　演示密码：`admin123`**

## 目录结构

```
admin/
├── login.html              登录页（独立布局，无侧边栏）
├── index.html              运营看板（默认首页）
├── users.html              用户列表
├── assets/
│   ├── style.css           设计变量 + 布局 + 全部通用组件样式
│   ├── app.js              布局注入、表格、分页、弹层、Toast、工具函数
│   ├── mock.js             全量模拟数据（全局常量）
│   └── charts.js           SVG 图表：折线 / 柱状 / 环形 / sparkline / 进度条
└── README.md               本文件
```

## 页面清单

| 页面 | 文件 | 状态 | 后端接口（草案） |
| --- | --- | --- | --- |
| 管理员登录 | `login.html` | 已完成 | `POST /api/admin/auth/login` |
| 运营看板 | `index.html` | 已完成 | `GET /api/admin/dashboard/overview` 等 |
| 用户列表 | `users.html` | 已完成 | `GET /api/admin/users`、`POST /api/admin/users/{id}/ban` |
| 封禁管理 | `bans.html` | 已完成 | `GET /api/admin/bans`、`POST /api/admin/bans` |
| 禁言管理 | `mutes.html` | 已完成 | `GET /api/admin/mutes`、`POST /api/admin/mutes` |
| 管理员账号 | `admins.html` | 已完成 | `GET /api/admin/admins`、`POST /api/admin/admins` |
| 角色与权限 | `roles.html` | 已完成 | `GET /api/admin/roles`、`PUT /api/admin/roles/{id}/permissions` |
| 实名认证 | `verifications.html` | 待建设 | `GET /api/admin/verifications` |
| 审核队列 | `review.html` | 已完成 | `GET /api/admin/review/queue`、`POST /api/admin/review/batch` |
| 帖子管理 | `posts.html` | 已完成 | `GET /api/admin/posts`、`POST /api/admin/posts/{id}/approve` |
| 评论管理 | `comments.html` | 已完成 | `GET /api/admin/comments`、`POST /api/admin/comments/batch-delete` |
| 标签管理 | `tags.html` | 已完成 | `GET /api/admin/tags`、`POST /api/admin/tags` |
| 敏感词库 | `sensitive-words.html` | 已完成 | `GET /api/admin/sensitive-words`、`PUT /api/admin/sensitive-words/policy` |
| 举报工单 | `reports.html` | 已完成 | `GET /api/admin/reports/tickets`、`POST /api/admin/reports/tickets/{key}/resolve` |
| 用户反馈 | `feedback.html` | 已完成 | `GET /api/admin/feedbacks`、`POST /api/admin/feedbacks/{id}/reply` |
| 公告管理 | `announcements.html` | 已完成 | `GET /api/admin/announcements`、`POST /api/admin/announcements` |
| 消息推送 | `notifications.html` | 已完成 | `GET /api/admin/notification-templates`、`POST /api/admin/push` |
| 推荐位管理 | `recommendations.html` | 已完成 | `GET /api/admin/recommendations`、`PUT /api/admin/recommendations/order` |
| 积分与等级 | `points.html` | 已完成 | `GET /api/admin/points/levels`、`GET /api/admin/points/logs` |
| 数据统计 | `analytics.html` | 已完成 | `GET /api/admin/analytics/overview`、`GET /api/admin/analytics/categories` |
| 导出审批 | `export-approval.html` | 已完成 | `GET /api/admin/export-requests`、`PATCH /api/admin/export-requests/{id}` |
| 运行状态 | `status.html` | 已完成 | `GET /api/admin/system/status`、`GET /api/admin/system/metrics` |
| 操作审计 | `audit-logs.html` | 已完成 | `GET /api/admin/audit-logs` |
| 系统日志 | `system-logs.html` | 已完成 | `GET /api/admin/system-logs` |
| 站点配置 | `settings.html` | 已完成 | `GET /api/admin/settings`、`PUT /api/admin/settings` |

未实现的菜单项统一指向 `#`，点击后通过 Toast 提示「页面正在建设中」，不会产生 404。

## 数据与接口约定

所有数据来自 `assets/mock.js` 暴露的全局常量，页面不使用硬编码表格行：

`USERS`、`POSTS`、`COMMENTS`、`TAGS`、`BANS`、`REPORTS`、`FEEDBACKS`、`ANNOUNCEMENTS`、`ANNOUNCEMENT_GROUPS`、`NOTIFICATION_GROUPS`、`NOTIFICATION_TEMPLATES`、`POINT_LEVELS`、`POINT_RULES`、`POINT_LOGS`、`POINT_LEADERBOARD`、`RECOMMENDATION_META`、`RECOMMENDATIONS`、`ANALYTICS`、`LOGS`、`ADMINS`、`ROLES`、`PERMISSION_MODULES`、`EXPORT_REQUESTS`、`STATUS_KPIS`、`STATUS_SERIES`、`STATUS_ERRORS`、`STATUS_RESOURCES`、`STATUS_NETWORK`、`AUDIT_LOGS`、`SYSTEM_LOGS`、`SYSTEM_LOG_MODULES`、`SETTINGS_SECTIONS`、`SETTINGS_META`、`SENSITIVE_WORDS`、`STATS`、`DASHBOARD`、`DICT`。

### 用户列表（`users.html`）

- 数据字段：`id / studentId / nickname / realName / college / major / grade / role / status / postCount / commentCount / reputation / verified / email / bio / registerAt / lastActiveAt`
- 接口：
  - `GET /api/admin/users?page=1&pageSize=20&keyword=&college=&status=&role=&verified=`
  - `GET /api/admin/users/{id}`
  - `POST /api/admin/users/{id}/ban` `{ type, reason, detail, durationDays }`
  - `POST /api/admin/users/{id}/unban`
  - `POST /api/admin/users/batch-ban` `{ ids:[], type, reason, durationDays }`
  - `GET /api/admin/users/export?keyword=&status=`
- 已实现：关键词 / 学院 / 状态 / 角色 / 实名 / 注册日期筛选、前端分页、每页条数切换、全选与批量封禁、行点击详情抽屉、封禁与解封、CSV 导出。

### 运营看板（`index.html`）

- 数据字段：`DASHBOARD.growth / interaction / retention / todos / activities / hotTags`、`STATS`
- 接口：
  - `GET /api/admin/dashboard/overview`
  - `GET /api/admin/dashboard/growth?days=30`
  - `GET /api/admin/dashboard/interaction?days=14`
  - `GET /api/admin/dashboard/retention`
  - `GET /api/admin/todos`、`PATCH /api/admin/todos/{id}`
  - `GET /api/admin/activities?limit=8`
- 已实现：4 个 KPI、成员增长折线图、内容互动柱状图、留存环形图、待办勾选、最新动态时间线、热门标签进度条、CSV 报表导出。

### 公告管理（`announcements.html`）

- 数据字段：`id / title / summary / content / positions[] / scope{ type, groups } / startAt / endAt / enabled / creator / createdAt`
- 接口：`GET /api/admin/announcements`、`POST /api/admin/announcements`、`PUT /api/admin/announcements/{id}`、`PATCH /api/admin/announcements/{id}/status`、`DELETE /api/admin/announcements/{id}`
- 已实现：关键词 / 位置 / 状态筛选、分页、投放状态自动计算（未开始 / 投放中 / 已结束 / 已停用）、新建与编辑抽屉（位置多选、接收范围、定时起止、启用开关）、简化富文本预览、启停与删除。

### 消息模板与推送（`notifications.html`）

- 数据字段：`NOTIFICATION_TEMPLATES`（`name / code / scene / channels[] / enabled / title / body / variables[]`）、`NOTIFICATION_GROUPS`
- 接口：`GET /api/admin/notification-templates`、`PUT /api/admin/notification-templates/{id}`、`POST /api/admin/push/estimate`、`POST /api/admin/push`
- 已实现：模板渠道图标、启用开关、编辑抽屉（变量点击插入、实时预览）、定向推送人群筛选、预计触达人数与分渠道估算、立即 / 定时发送、发送预览与二次确认。

### 积分与等级（`points.html`）

- 数据字段：`POINT_LEVELS`、`POINT_RULES`、`POINT_LOGS`、`POINT_LEADERBOARD`
- 接口：`GET /api/admin/points/levels`、`PUT /api/admin/points/levels/{level}`、`GET /api/admin/points/rules`、`PATCH /api/admin/points/rules/{id}/status`、`GET /api/admin/points/logs`、`GET /api/admin/points/leaderboard`
- 已实现：信任等级表与编辑弹窗、积分规则启用开关、积分流水分页（正负色与余额）、排行榜 TOP20（等级胶囊与前三名样式）。

### 推荐位与运营位（`recommendations.html`）

- 数据字段：`RECOMMENDATION_META`、`RECOMMENDATIONS[global|category|carousel|sidebar]`
- 接口：`GET /api/admin/recommendations`、`POST /api/admin/recommendations`、`PUT /api/admin/recommendations/order`、`DELETE /api/admin/recommendations/{id}`
- 已实现：按位置分 Tab、拖拽排序、生效时间段展示、添加推荐模态（帖子搜索 + 勾选 + 生效时间）、移除推荐。

### 数据分析（`analytics.html`）

- 数据字段：`ANALYTICS.daily`（180 天）、`ANALYTICS.categories`、`ANALYTICS.retention`
- 接口：`GET /api/admin/analytics/overview?days=30&compare=1`、`GET /api/admin/analytics/categories`、`GET /api/admin/analytics/retention`、`GET /api/admin/analytics/active-users`
- 已实现：7 / 30 / 90 天切换、对比上期开关、4 个 KPI（环比 + sparkline）、成员增长双线图（含上期对比线）、分类堆叠柱状图、次月留存环形图、分类表现与活跃用户 TOP10 表格。

### 管理员账号（`admins.html`）

- 数据字段：`ADMINS`（`name / account / roles[] / status / lastLoginAt / lastIp / remark`）
- 接口：`GET /api/admin/admins`、`POST /api/admin/admins`、`PUT /api/admin/admins/{id}`、`PATCH /api/admin/admins/{id}/status`、`POST /api/admin/admins/{id}/reset-password`
- 已实现：关键词 / 角色 / 状态筛选、分页、角色胶囊、状态开关、新增与编辑模态框（账号、初始密码、角色多选、备注）、重置密码。

### 角色与权限（`roles.html`）

- 数据字段：`ROLES.permissionKeys`、`PERMISSION_MODULES`（7 大模块、28 个权限点）
- 接口：`GET /api/admin/roles`、`GET /api/admin/permissions`、`PUT /api/admin/roles/{id}/permissions`
- 已实现：左侧角色列表与内置标记、右侧权限矩阵、模块级全选 / 半选（indeterminate）、未保存提示、保存与重置、切换角色时未保存确认。

### 数据导出审批（`export-approval.html`）

- 数据字段：`EXPORT_REQUESTS`
- 接口：`GET /api/admin/export-requests`、`PATCH /api/admin/export-requests/{id}`
- 已实现：待审批 / 已通过 / 已驳回 Tab 与计数、行展开申请理由、审批抽屉、通过审批、驳回时强制填写理由。

### 运行状态（`status.html`）

- 数据字段：`STATUS_KPIS`、`STATUS_SERIES`、`STATUS_ERRORS`、`STATUS_RESOURCES`、`STATUS_NETWORK`
- 接口：`GET /api/admin/system/status`、`GET /api/admin/system/metrics?range=24h`、`GET /api/admin/system/errors`
- 已实现：CPU / 内存 / 磁盘 / 在线用户 KPI 与 sparkline、QPS 与响应时间双 Y 轴折线（1h / 24h / 7d）、异常请求表格、资源仪表条与网络 I/O 双线图。

### 操作审计日志（`audit-logs.html`）

- 数据字段：`AUDIT_LOGS`（含 `changes[{ label, before, after, kind }]`）
- 接口：`GET /api/admin/audit-logs`、`GET /api/admin/audit-logs/export`
- 已实现：不可篡改提示条与导出、操作人 / 类型 / 目标 / 时间范围筛选、精确到秒的时间列、底部抽屉展示操作前后双栏对比（删除红色、新增绿色）。

### 系统日志（`system-logs.html`）

- 数据字段：`SYSTEM_LOGS`、`SYSTEM_LOG_MODULES`
- 接口：`GET /api/admin/system-logs?level=&module=&from=&to=&keyword=`
- 已实现：ERROR / WARN / INFO / DEBUG 多选 Chips、模块 / 时间 / 关键词筛选、等宽时间戳与级别淡色行底、底部抽屉深色 JSON payload（行号 + 复制按钮）。

### 站点配置（`settings.html`）

- 数据字段：`SETTINGS_SECTIONS`（5 组、24 项配置）、`SETTINGS_META`
- 接口：`GET /api/admin/settings`、`PUT /api/admin/settings`、`POST /api/admin/settings/reset`
- 已实现：左侧锚点导航与滚动高亮、行式配置表单（开关 / 数字 / 下拉）、底部固定保存条、数值范围校验、保存后 Toast 与最后修改信息、恢复默认。

### 登录（`login.html`）

- 接口：`POST /api/admin/auth/login` `{ account, password, remember }`
- 已实现：前端校验、密码可见切换、加载态、错误提示；演示环境使用本地账号校验。

## 新增页面指引

1. 复制任意已完成页面的结构，将 `<main data-page="xxx">` 中的内容替换为新页面内容。
2. 引入脚本（顺序固定）：

```html
<link rel="stylesheet" href="assets/style.css">
<script src="assets/app.js"></script>
<script src="assets/mock.js"></script>
<script src="assets/charts.js"></script>
```

3. 在 `assets/app.js` 的 `MENU` 中将对应菜单项的 `href` 改为新文件名，`renderLayout('xxx')` 会自动注入侧边栏与顶栏，并高亮当前菜单、生成面包屑。
4. 表格统一使用 `App.renderTable(container, columns, rows, options)`，分页统一使用 `App.renderPagination(container, options)`。

## 通用能力

- **布局**：侧边栏可折叠（宽 240px / 64px），≤1024px 自动折叠；顶栏包含面包屑、全局搜索（⌘K / Ctrl+K）、通知、主题切换、账号菜单。
- **组件**：`App.toast`、`App.modal`、`App.drawer`、`App.renderTable`、`App.renderPagination`、`App.avatarHTML`、`App.pill`、`App.emptyHTML`。
- **图表**：`Charts.line / bar / donut / sparkline / progress`，全部为手写 SVG。
- **主题**：默认浅色，顶栏可切换深色；侧边栏折叠与主题偏好写入 `localStorage`。

## 设计系统

颜色、字号、间距、圆角、阴影全部通过 `assets/style.css` 的 `:root` 变量管理。主色 `#1E5BB8`，页面背景 `#F6F7F9`，侧边栏 `#0F1B33`，卡片圆角 12px，按钮 / 输入框圆角 8px。字体栈：`"Inter", "Noto Sans SC", "PingFang SC", "Microsoft YaHei", system-ui, sans-serif`。
