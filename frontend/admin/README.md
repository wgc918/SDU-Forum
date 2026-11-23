# SDU Forum Admin

山东大学校园论坛「SDU Forum」后台管理系统前端原型。纯 HTML / CSS / JavaScript 实现，无构建步骤、无外部 CDN、无第三方框架。

当前包含 **24 个页面**（23 个可用页面 + 登录页），并附带完整的模拟数据、接口草案与 MySQL 表结构设计，可直接作为后端联调与数据库落地的参考。

---

## 1. 运行方式与目录

### 1.1 运行

- 直接双击 `login.html` 即可预览（推荐 Chrome / Edge）。
- 或在当前目录启动静态服务器：

```bash
python -m http.server 8765
# 打开 http://localhost:8765/login.html
```

**演示账号：`admin`　演示密码：`admin123`**

### 1.2 目录结构

```
admin/
├── login.html                 登录页（独立布局，无侧边栏）
├── index.html                 运营看板（默认首页）
├── users.html                 用户列表
├── bans.html                  封禁管理
├── mutes.html                 禁言管理
├── admins.html                管理员账号
├── roles.html                 角色与权限
├── review.html                审核队列
├── posts.html                 帖子管理
├── comments.html              评论管理
├── tags.html                  标签管理
├── sensitive-words.html       敏感词库
├── reports.html               举报工单
├── feedback.html              用户反馈
├── announcements.html         公告管理
├── notifications.html         消息模板与推送
├── recommendations.html       推荐位与运营位
├── points.html                积分与等级
├── analytics.html             数据分析
├── export-approval.html       数据导出审批
├── status.html                运行状态
├── audit-logs.html            操作审计日志
├── system-logs.html           系统日志
├── settings.html              站点配置
├── assets/
│   ├── style.css              设计变量 + 布局 + 全部通用组件与页面级样式
│   ├── app.js                 菜单/布局注入、表格、分页、弹层、Toast、工具函数
│   ├── mock.js                全量模拟数据（全局常量）
│   └── charts.js              SVG 图表：折线（支持双 Y 轴）/ 柱状（支持堆叠）/ 环形 / sparkline / 进度条
└── README.md                  本文件
```

所有页面只编写 `<main>` 内容，脚本顶部调用 `App.renderLayout('<menu-key>')` 注入侧边栏、顶栏与面包屑。

---

## 2. 完整页面清单

> 接口统一以 `/api/admin` 为前缀；下表参数为**前端当前使用的关键参数**，返回字段为页面真实消费的字段（未列出的审计字段按需扩展）。分页接口默认使用 `page`、`pageSize`，返回 `{ list, total, page, pageSize }`。
>
> 状态：✅ 已完成　🚧 待建设

| 文件名 | 页面名称 | 所属分组 | 依赖的 Mock 常量 | 对应后端接口（方法 + 路径 + 关键参数） | 主要返回字段 |
| --- | --- | --- | --- | --- | --- |
| `login.html` | 管理员登录 | 独立页面 | 无 | `POST /auth/login` `{ account, password, remember }`<br>`POST /auth/logout` | `token`、`expiresAt`、`admin{ id, name, account, roles[], permissions[] }` |
| `index.html` | 运营看板 | 概览 | `DASHBOARD`、`STATS`、`USERS`、`POSTS` | `GET /dashboard/overview?days=30`<br>`GET /dashboard/growth?days=30`<br>`GET /dashboard/interaction?days=14`<br>`GET /dashboard/retention`<br>`GET /todos`、`PATCH /todos/{id}`<br>`GET /activities?limit=8` | `kpis{ users, posts, comments, reports }`、`growth{ labels, newUsers, activeUsers }`、`interaction{ labels, posts, comments }`、`retention{ day1, day3, day7, day30 }`、`todos[]`、`activities[]`、`hotTags[]` |
| `users.html` | 用户列表 | 用户管理 | `USERS`、`BANS`、`STATS`、`POSTS`、`DICT` | `GET /users?page&pageSize&keyword&college&status&role&verified&registerFrom&registerTo`<br>`GET /users/{id}`<br>`POST /users/{id}/ban` `{ type, reason, durationDays, detail }`<br>`POST /users/{id}/unban`<br>`POST /users/batch-ban` `{ ids[], type, reason, durationDays }`<br>`GET /users/export?...` | `list[{ id, studentId, nickname, realName, college, major, grade, role, status, postCount, commentCount, reputation, verified, registerAt, lastActiveAt }]`、`total`、`summary{ total, active, banned, verified }` |
| `bans.html` | 封禁管理 | 用户管理 | `BANS`、`USERS` | `GET /bans?page&pageSize&status&from&to&keyword`<br>`POST /bans` `{ userId, type, reason, durationDays, note }`<br>`POST /bans/{id}/revoke`<br>`GET /bans/summary` | `list[{ id, userId, studentId, nickname, startAt, endAt, reason, type, status, operator }]`、`summary{ current, monthly, autoReleased, permanent }`、`reasons[{ reason, count }]` |
| `mutes.html` | 禁言管理 | 用户管理 | `USERS`、`TAGS` | `GET /mutes?page&pageSize&status&from&to&keyword`<br>`POST /mutes` `{ userId, durationDays, scopeBoardIds[], reason, note }`<br>`POST /mutes/{id}/revoke`<br>`GET /mutes/summary` | `list[{ id, userId, studentId, nickname, scope, startAt, endAt, reason, status, operator }]`、`summary{ current, monthly, autoReleased }`、`boards[{ id, name }]` |
| `admins.html` | 管理员账号 | 用户管理 | `ADMINS`、`ROLES` | `GET /admins?page&pageSize&keyword&role&status`<br>`POST /admins` `{ account, password, roles[], remark }`<br>`PUT /admins/{id}`<br>`PATCH /admins/{id}/status` `{ status }`<br>`POST /admins/{id}/reset-password` | `list[{ id, name, account, roles[], email, status, lastLoginAt, lastIp, remark }]`、`total`、`roles[]` |
| `roles.html` | 角色与权限 | 用户管理 | `ROLES`、`PERMISSION_MODULES` | `GET /roles`<br>`GET /permissions`<br>`PUT /roles/{id}/permissions` `{ permissionKeys[] }` | `roles[{ id, name, description, builtin, memberCount, permissionKeys[] }]`、`modules[{ key, name, desc, points[{ key, label }] }]`、`updatedAt` |
| `verifications.html` 🚧 | 实名认证 | 用户管理 | — | `GET /verifications?page&pageSize&status&keyword`<br>`POST /verifications/{id}/approve`<br>`POST /verifications/{id}/reject` `{ reason }` | `list[{ id, userId, studentId, realName, college, idCardMasked, submittedAt, status }]`、`total` |
| `review.html` | 审核队列 | 内容管理 | `POSTS`、`COMMENTS`、`REPORTS`、`USERS` | `GET /review/queue?type=post\|comment&page&pageSize`<br>`GET /review/stats`<br>`POST /review/batch` `{ items:[{ type, id }], action:'approve'\|'reject', reason }` | `items[{ type, id, title, excerpt, authorId, authorName, reason, createdAt, status }]`、`stats{ pending, approvedToday, rejectedToday, avgSeconds }` |
| `posts.html` | 帖子管理 | 内容管理 | `POSTS`、`TAGS`、`DICT` | `GET /posts?page&pageSize&keyword&status&tagId&board&from&to&sort`<br>`GET /posts/{id}`<br>`POST /posts/{id}/approve`<br>`POST /posts/{id}/reject` `{ reason }`<br>`POST /posts/{id}/hide`、`POST /posts/{id}/restore`<br>`POST /posts/batch` `{ ids[], action }`<br>`GET /posts/export?...` | `list[{ id, title, summary, authorId, authorName, board, tagName, status, pinned, essence, viewCount, likeCount, commentCount, createdAt }]`、`total`、`tabs{ all, pending, published, hidden }` |
| `comments.html` | 评论管理 | 内容管理 | `COMMENTS`、`POSTS`、`REPORTS` | `GET /comments?page&pageSize&keyword&status&postId`<br>`POST /comments/{id}/delete`<br>`POST /comments/{id}/restore`<br>`POST /comments/batch-delete` `{ ids[] }`<br>`GET /comments/{id}/reports` | `threads[{ postId, postTitle, comments[] }]`、`comments[{ id, postId, authorId, authorName, content, likeCount, status, reportCount, createdAt }]`、`total` |
| `tags.html` | 标签管理 | 内容管理 | `TAGS` | `GET /tags?keyword&category&status`<br>`POST /tags` `{ name, category, description, color, status }`<br>`PUT /tags/{id}`<br>`DELETE /tags/{id}`<br>`GET /tags/stats` | `list[{ id, name, category, description, color, postCount, status, createdAt }]`、`categories[]`、`cloud[{ name, weight }]` |
| `sensitive-words.html` | 敏感词库 | 内容管理 | `SENSITIVE_WORDS` | `GET /sensitive-words?page&pageSize&keyword&category&level`<br>`POST /sensitive-words` `{ word, category, level, action }`<br>`PUT /sensitive-words/{id}`<br>`POST /sensitive-words/batch-delete` `{ ids[] }`<br>`POST /sensitive-words/import`（multipart）<br>`GET /sensitive-words/export`<br>`GET/PUT /sensitive-words/policy` | `list[{ id, word, category, level, matchMode, hitCount, enabled }]`、`total`、`policy{ autoBlock, reviewBeforePublish, notifyAuthor }` |
| `reports.html` | 举报工单 | 社区治理 | `REPORTS`、`POSTS`、`COMMENTS`、`USERS`、`BANS` | `GET /reports/tickets?page&pageSize&status&targetType&from&to&keyword`<br>`GET /reports/tickets/{key}`<br>`POST /reports/tickets/{key}/resolve` `{ action, note, durationDays }` | `list[{ key, targetType, targetId, targetTitle, reportCount, reasons[], status, assignee, createdAt }]`、`detail{ target, records[], history[] }`、`summary{ pending, processing, resolvedToday, avgHandleMinutes }` |
| `feedback.html` | 用户反馈 | 社区治理 | `FEEDBACKS`、`USERS` | `GET /feedbacks?page&pageSize&status&type`<br>`GET /feedbacks/{id}`<br>`POST /feedbacks/{id}/reply` `{ content, status, attachments[] }`<br>`GET /feedbacks/stats` | `list[{ id, type, title, userId, studentId, content, status, createdAt }]`、`detail{ attachments[], replies[] }`、`stats{ pending, processing, resolved }` |
| `announcements.html` | 公告管理 | 运营中心 | `ANNOUNCEMENTS`、`ANNOUNCEMENT_GROUPS` | `GET /announcements?page&pageSize&position&status&keyword`<br>`POST /announcements` `{ title, content, positions[], scope, startAt, endAt, enabled }`<br>`PUT /announcements/{id}`<br>`PATCH /announcements/{id}/status` `{ enabled }`<br>`DELETE /announcements/{id}` | `list[{ id, title, summary, positions[], scope{ type, groups[] }, startAt, endAt, enabled, creator, status, createdAt }]`、`total` |
| `notifications.html` | 消息模板与推送 | 运营中心 | `NOTIFICATION_TEMPLATES`、`NOTIFICATION_GROUPS`、`USERS` | `GET /notification-templates`<br>`PUT /notification-templates/{id}` `{ name, title, body }`<br>`PATCH /notification-templates/{id}/status` `{ enabled }`<br>`POST /push/estimate` `{ audience }`<br>`POST /push` `{ title, content, audience, sendAt }` | `templates[{ id, name, code, scene, channels[], enabled, title, body, variables[] }]`、`estimate{ total, inbox, email, push, verified, recentActive }`、`task{ id, status, sendAt, estimatedCount }` |
| `recommendations.html` | 推荐位与运营位 | 运营中心 | `RECOMMENDATIONS`、`RECOMMENDATION_META`、`POSTS` | `GET /recommendations?position=global\|category\|carousel\|sidebar`<br>`POST /recommendations` `{ position, postIds[], startAt, endAt }`<br>`PUT /recommendations/order` `{ position, ids[] }`<br>`DELETE /recommendations/{id}` | `slots[{ key, label, desc }]`、`list[{ id, postId, title, authorName, board, tagName, viewCount, likeCount, commentCount, startAt, endAt, order }]` |
| `points.html` | 积分与等级 | 运营中心 | `POINT_LEVELS`、`POINT_RULES`、`POINT_LOGS`、`POINT_LEADERBOARD` | `GET /points/levels`<br>`PUT /points/levels/{level}` `{ name, requirement, permission, mode }`<br>`GET /points/rules`<br>`PATCH /points/rules/{id}/status` `{ enabled }`<br>`GET /points/logs?page&pageSize`<br>`GET /points/leaderboard?limit=20` | `levels[{ level, name, requirement, permission, mode }]`、`rules[{ id, action, value, dailyCap, enabled }]`、`logs[{ id, studentId, nickname, action, delta, balance, createdAt }]`、`leaderboard[{ rank, nickname, studentId, college, level, points }]` |
| `analytics.html` | 数据分析 | 运营中心 | `ANALYTICS`、`USERS` | `GET /analytics/overview?days=7\|30\|90&compare=1`<br>`GET /analytics/categories?days=30`<br>`GET /analytics/retention?days=30`<br>`GET /analytics/active-users?limit=10` | `kpis{ active, register, posts, interactionRate, delta }`、`daily[{ date, register, active, posts, comments, likes }]`、`categories[{ name, posts, comments, avgInteraction, delta }]`、`retention{ current, previous }`、`activeUsers[{ nickname, studentId, college, postCount, commentCount, lastActiveAt }]` |
| `export-approval.html` | 数据导出审批 | 系统 | `EXPORT_REQUESTS` | `GET /export-requests?status=pending\|approved\|rejected`<br>`PATCH /export-requests/{id}` `{ status, reviewNote }` | `list[{ id, applicant, account, dataType, scope, volume, requestedAt, status, approver, reviewedAt, reason, reviewNote }]`、`counts{ pending, approved, rejected }` |
| `status.html` | 运行状态 | 系统 | `STATUS_KPIS`、`STATUS_SERIES`、`STATUS_ERRORS`、`STATUS_RESOURCES`、`STATUS_NETWORK` | `GET /system/status`<br>`GET /system/metrics?range=1h\|24h\|7d`<br>`GET /system/errors?range=1h` | `kpis[{ key, label, value, data[] }]`、`series{ labels[], qps[], rt[] }`、`errors[{ createdAt, path, status, count, ip }]`、`resources[{ key, value, detail }]`、`network{ labels[], inbound[], outbound[] }` |
| `audit-logs.html` | 操作审计日志 | 系统 | `AUDIT_LOGS` | `GET /audit-logs?page&pageSize&operator&action&target&from&to`<br>`GET /audit-logs/export?...` | `list[{ id, createdAt, operator, role, action, tone, targetType, target, ip, result, detail, changes[{ label, before, after, kind }] }]`、`total` |
| `system-logs.html` | 系统日志 | 系统 | `SYSTEM_LOGS`、`SYSTEM_LOG_MODULES` | `GET /system-logs?page&pageSize&level&module&from&to&keyword` | `list[{ id, ts, level, module, message, payload }]`、`total`、`modules[]` |
| `settings.html` | 站点配置 | 系统 | `SETTINGS_SECTIONS`、`SETTINGS_META` | `GET /settings`<br>`PUT /settings` `{ sections:[{ key, fields:[{ key, value }] }] }`<br>`POST /settings/reset` | `sections[{ key, name, desc, fields[{ key, label, desc, type, value, min, max, suffix, options[] }] }]`、`meta{ updatedAt, operator }` |

未实现的菜单项统一指向 `#`，点击后 Toast 提示「页面正在建设中」，不会产生 404。

---

## 3. 从 Mock 切换到真实接口

### 3.1 当前数据流

每个页面在脚本顶部直接读取全局常量：

```js
const USERS = window.USERS;
const state = { page: 1, pageSize: 20 };
```

所有交互直接修改数组（`unshift`、`splice`、`Object.assign`、状态字段赋值），再调用页面内的 `renderXxx()` 重新渲染。**没有 loading / error 态，也没有网络层。**

### 3.2 推荐改造步骤

1. 新增 `assets/api.js`，统一封装请求、鉴权、错误与分页：

```js
window.API = {
  base: '/api/admin',
  async request(path, options) {
    const opts = Object.assign({ headers: {} }, options);
    opts.headers['Content-Type'] = 'application/json';
    const token = localStorage.getItem('sdu_admin_token');
    if (token) opts.headers.Authorization = 'Bearer ' + token;
    const res = await fetch(this.base + path, opts);
    if (res.status === 401) {
      localStorage.removeItem('sdu_admin_token');
      location.href = 'login.html';
      throw new Error('登录已过期');
    }
    const body = await res.json();
    if (!res.ok || body.code !== 0) throw new Error(body.message || '请求失败');
    return body.data;
  },
  get(path) { return this.request(path); },
  post(path, data) { return this.request(path, { method: 'POST', body: JSON.stringify(data) }); },
  put(path, data) { return this.request(path, { method: 'PUT', body: JSON.stringify(data) }); },
  patch(path, data) { return this.request(path, { method: 'PATCH', body: JSON.stringify(data) }); },
  del(path) { return this.request(path, { method: 'DELETE' }); }
};
```

2. 把常量改为可变状态，并增加异步加载：

```js
// 改造前
const USERS = window.USERS;

// 改造后
let users = [];
let total = 0;
let loading = false;

async function loadUsers() {
  loading = true;
  renderTable(); // 显示骨架/加载态
  try {
    const data = await API.get('/users?page=' + state.page + '&pageSize=' + state.pageSize +
      '&keyword=' + encodeURIComponent(state.keyword) + '&status=' + state.status);
    users = data.list;
    total = data.total;
  } catch (error) {
    App.toast(error.message, 'danger');
  } finally {
    loading = false;
    renderTable();
  }
}
```

3. 将写操作从「改数组」改为「调接口 → 成功后再刷新」：

```js
// 改造前
ADMINS.unshift(newAdmin);
renderTable();

// 改造后
await API.post('/admins', payload);
App.toast('创建成功', 'success');
await loadAdmins();
```

4. 分页改为服务端分页：`App.renderPagination` 的 `total` 使用接口返回值，`onChange` 中先更新 `state.page/pageSize`，再调用 `loadXxx()`。
5. 全部页面迁移完成后，移除 HTML 中的 `<script src="assets/mock.js"></script>`，并删除各页面顶部的 `window.XXX` 读取。

### 3.3 各页面替换点清单

> 「数据入口」指替换为接口请求的位置；「渲染函数」指请求成功后需要调用的函数；「写操作 fetch 位置」指当前直接修改数组的位置。

| 页面 | 数据入口 | 渲染函数 | 写操作 / fetch 位置 |
| --- | --- | --- | --- |
| `login.html` | 表单提交监听 | `showError / clearError` | 登录提交改为 `POST /auth/login`；成功写入 token 并跳转 `index.html` |
| `index.html` | `init()` | `renderKpis`、`renderCharts`、`renderTodos`、`renderActivities` | 待办勾选 `PATCH /todos/{id}`；`exportReport` 改为请求导出接口 |
| `users.html` | `refreshData()` / `renderAll()` | `renderKpis`、`renderAll`（内含 `App.renderTable`）、`updateBatchBar` | `openBanModal` 确认、`openUnban`、批量封禁、`exportCsv` |
| `bans.html` | `getFiltered()` → `renderAll()` | `renderKpis`、`renderTable`、`renderCharts`、`renderSuggest` | `openBanModal` 提交、`revoke` |
| `mutes.html` | `getFiltered()` → `renderAll()` | `renderKpis`、`renderTable`、`renderCharts`、`renderSuggest` | `openMuteModal` 提交、`revoke` |
| `admins.html` | `filtered()` → `renderTable()` | `renderTable`、`rolePills` | `openAdminModal` 的 `onConfirm`（POST/PUT）、状态开关 `PATCH`、`resetPassword` |
| `roles.html` | `renderRoles()`、`renderMatrix()` | `renderRoles`、`renderMatrix`、`syncModuleHeaders` | `#perm-save-btn` 保存 `PUT /roles/{id}/permissions`；重置仅改本地 draft |
| `review.html` | `renderAll()` | `renderStream`、`renderSources`、`renderProgress` | `applyStatus`、`batchApprove`、`finishItem` |
| `posts.html` | `renderAll()` | `renderTabs`、`renderTable`、`metaHTML` | `approve`、`reject`、`hide`、`restore`、`updatePost`、`exportCsv` |
| `comments.html` | `renderAll()` | `renderThreads`、`renderComments`、`updateToolbar` | `deleteSingle`、`restoreSingle`、`batchDelete`、`setDeleted` |
| `tags.html` | `renderAll()` | `renderTable`、`renderCloud`、`renderRank` | `openTagModal` 保存、`openDeleteModal` 删除 |
| `sensitive-words.html` | `getFiltered()` → `renderTable()` | `renderTable`、`renderPolicy` | `openAddModal`、`openEditModal`、`openDeleteModal`、`batchDelete`、`importCSV`、`exportCSV` |
| `reports.html` | `buildTickets()` / `getFiltered()` | `renderKpis`、`renderTable`、`openTicket` | `applyAction`（删除内容 / 禁言 / 封禁 / 驳回 / 标记已处理） |
| `feedback.html` | `getFiltered()` → `renderAll()` | `renderChips`、`renderList`、`renderDetail`、`timelineHTML` | `renderDetail` 内的回复提交与状态切换 |
| `announcements.html` | `getFiltered()` → `renderTable()` | `renderTable`、`drawerBody`、`updatePreview` | `openDrawer` 的保存按钮（POST/PUT）、`toggleAnnouncement`、`removeAnnouncement` |
| `notifications.html` | `renderTemplates()`、`renderReach()` | `renderTemplates`、`renderReach`、`templateDrawerBody` | 模板保存 `PUT`、模板开关 `PATCH`、`confirmSend` / `previewModal` → `POST /push` |
| `recommendations.html` | `listOf()` → `renderAll()` | `renderTabs`、`renderList` | `openPicker` 的 `onConfirm`（POST）、`bindDrag` 的 `drop`（PUT order）、`removeItem`（DELETE） |
| `points.html` | `renderLevels`、`renderRules`、`renderLogs`、`renderRank` | 同名函数 | `openLevelModal` 保存、规则开关 `PATCH` |
| `analytics.html` | `renderAll()` | `renderKpis`、`renderCharts`、`renderTables` | 切换时间 / 对比开关时重新请求 `overview / categories / retention` |
| `export-approval.html` | `renderTable()` | `renderTabs`、`renderTable`、`openReview` | `openReview` 内「通过 / 驳回」→ `PATCH /export-requests/{id}` |
| `status.html` | `renderKpis`、`renderApiChart`、`renderErrors`、`renderResources` | 同名函数 | 范围切换请求 `metrics`；刷新按钮重新请求 `status / errors` |
| `audit-logs.html` | `filtered()` → `renderTable()` | `renderTable`、`openDetail` | `exportCsv` 改为请求 CSV 接口；筛选参数直接透传 |
| `system-logs.html` | `filtered()` → `renderTable()` | `renderTable`、`openPayload` | 级别 / 模块 / 时间 / 关键词变化时请求 `/system-logs` |
| `settings.html` | `renderForm()` | `renderNav`、`renderForm`、`renderMeta` | `saveSettings` → `PUT /settings`；`restoreDefaults` → `POST /settings/reset` |

### 3.4 联调注意事项

- **统一响应包**：建议 `{ code: 0, message: 'ok', data: {...} }`；分页 `data` 为 `{ list, total, page, pageSize }`。
- **服务端筛选/分页**：当前所有筛选均为前端内存过滤，迁移时需把 `getFiltered()` 中的条件转换成 query 参数，删除本地过滤。
- **状态枚举**：前端展示值与后端值需建立映射（如公告 `scheduled/active/ended/disabled`、举报 `pending/processing/resolved/rejected`）。
- **并发与防抖**：搜索输入已使用 `App.debounce`，接入接口后增加 `AbortController` 取消上一次请求。
- **错误态**：为每个表格补充 `loading` 骨架、请求失败空状态与重试按钮；写操作失败时保留原数据。
- **权限**：前端需按登录返回的 `permissions[]` 隐藏菜单和按钮，后端必须再次校验。

---

## 4. 设计系统变量速查

设计变量全部定义在 `assets/style.css` 的 `:root`，深色主题通过 `html[data-theme="dark"]` 覆盖。

### 4.1 颜色

| 分类 | 变量 | 值（浅色） | 用途 |
| --- | --- | --- | --- |
| 品牌 | `--primary` | `#1E5BB8` | 主按钮、链接、选中态、图表主色 |
| 品牌 | `--primary-hover` | `#1A4FA0` | 主按钮 hover |
| 品牌 | `--primary-soft` | `#EFF4FC` | 浅色选中背景、提示块 |
| 侧栏 | `--sidebar-bg` | `#0F1B33` | 侧边栏、深色 JSON 面板 |
| 侧栏 | `--sidebar-text` | `#94A3B8` | 侧栏默认文字 |
| 侧栏 | `--sidebar-active-bar` | `#3B82F6` | 侧栏选中左侧竖条 |
| 背景 | `--bg-page` | `#F6F7F9` | 页面背景 |
| 背景 | `--bg-card` | `#FFFFFF` | 卡片、弹层 |
| 背景 | `--bg-subtle` | `#FAFBFC` | 表头、分组头 |
| 背景 | `--bg-hover` | `#F8FAFC` | 行 / 菜单 hover |
| 边框 | `--border` | `#E5E7EB` | 卡片、输入框边框 |
| 分割 | `--divider` | `#F1F3F5` | 表格分割线、内容分割线 |
| 文字 | `--text-1` | `#0F172A` | 标题、主要文字 |
| 文字 | `--text-2` | `#64748B` | 次要文字 |
| 文字 | `--text-3` | `#94A3B8` | 辅助说明、占位符 |
| 成功 | `--success` / `--success-soft` / `--success-text` | `#10B981` / `#ECFDF5` / `#047857` | 成功胶囊、正向数据 |
| 警告 | `--warning` / `--warning-soft` / `--warning-text` | `#F59E0B` / `#FFFBEB` / `#B45309` | 待处理、警告 |
| 危险 | `--danger` / `--danger-soft` / `--danger-text` | `#EF4444` / `#FEF2F2` / `#B91C1C` | 删除、封禁、错误 |
| 信息 | `--info` / `--info-soft` / `--info-text` | `#2563EB` / `#EFF6FF` / `#1D4ED8` | 中性提示、信息胶囊 |

### 4.2 字体与字号

| 变量 | 值 | 用途 |
| --- | --- | --- |
| `--font-sans` | `"Inter", "Noto Sans SC", "PingFang SC", "Microsoft YaHei", system-ui, sans-serif` | 全局字体 |
| `--fs-xs` | `11px` | 标签、辅助说明、表格 sub |
| `--fs-sm` | `12px` | 次要文字、按钮小号 |
| `--fs-md` | `13px` | 表格正文、表单控件 |
| `--fs-base` | `14px` | 页面正文 |
| `--fs-lg` | `15px` | 卡片重点文字、品牌名 |
| `--fs-title` | `20px` | 页面标题 |
| `--fs-kpi` | `28px` | KPI 数值 |

### 4.3 间距、圆角、阴影与布局

| 分类 | 变量 | 值 | 用途 |
| --- | --- | --- | --- |
| 间距 | `--sp-1` → `--sp-8` | `4 / 8 / 12 / 16 / 20 / 24 / 32px` | 8px 栅格间距 |
| 圆角 | `--radius-card` | `12px` | 卡片、面板 |
| 圆角 | `--radius-control` | `8px` | 按钮、输入框、下拉 |
| 圆角 | `--radius-badge` | `6px` | 小徽标 |
| 圆角 | `--radius-pill` | `999px` | 胶囊、Chips、开关 |
| 阴影 | `--shadow-card` | `0 1px 2px rgba(15,23,42,.04)` | 卡片默认 |
| 阴影 | `--shadow-pop` | `0 8px 24px rgba(15,23,42,.10)` | 下拉菜单、气泡 |
| 阴影 | `--shadow-modal` | `0 20px 48px rgba(15,23,42,.18)` | 模态框 / 抽屉 |
| 布局 | `--sidebar-w` / `--sidebar-w-collapsed` | `240px / 64px` | 侧栏展开 / 折叠 |
| 布局 | `--topbar-h` | `64px` | 顶栏高度 |
| 内容边距 | — | `24px` | 页面内容左右留白 |
| 响应式 | — | `1280px 最优 / 1024px 最小` | ≤1024px 侧栏自动折叠 |

### 4.4 常用类名

| 类名 | 用途 |
| --- | --- |
| `.page-head / .page-title / .page-desc` | 页面标题区 |
| `.card / .card-head / .card-title / .card-sub / .card-body` | 通用卡片 |
| `.filter-card / .filter-field / .filter-label / .filter-actions` | 筛选栏 |
| `.table-card / .data-table / .table-wrap` | 表格容器 |
| `.btn / .btn-primary / .btn-secondary / .btn-danger / .btn-sm` | 按钮 |
| `.segmented / .chips / .chip / .option-row / .option-pill` | 分段控件与多选胶囊 |
| `.switch / .switch-track` | 开关 |
| `.pill`（由 `App.pill` 生成） | 状态胶囊 |
| `.drawer / .sheet / .overlay` | 右侧抽屉 / 底部抽屉 / 遮罩 |
| `.kpi-grid / .kpi-card / .kpi-value` | KPI 卡片 |
| `.tabs / .tab` | 下划线 Tab |

---

## 5. 已知未实现项与建议方案

| 项目 | 当前状态 | 建议技术方案 | 优先级 |
| --- | --- | --- | --- |
| 富文本编辑器 | 公告页为 textarea + `**加粗**` 简化预览 | 引入 TipTap / Quill，输出 HTML + 纯文本；服务端使用 DOMPurify 或 Java HTML Sanitizer 过滤 | 高 |
| 文件 / 图片上传 | 反馈附件、头像、帖子图片均为模拟 URL | 对象存储直传：`POST /upload/presign` 获取签名 URL，前端分片上传，后端回调入库；限制 MIME、大小与图片压缩 | 高 |
| 拖拽排序持久化 | 推荐位支持 HTML5 拖拽，但仅内存排序 | 引入 SortableJS（含触屏与键盘无障碍），拖拽结束调用 `PUT /recommendations/order` | 高 |
| 服务端分页 / 筛选 | 全部为前端内存过滤 | 按第 3 节改造，接口返回 `list/total`，前端只保留查询条件 | 高 |
| 权限控制 | 角色矩阵仅前端展示，无路由 / 按钮级权限 | 登录返回 `permissions[]`；增加 `can('key')` 指令隐藏菜单与按钮；后端使用 RBAC（Casbin / Spring Security）强校验 | 高 |
| 登录与会话 | 本地演示账号，无真实 JWT | JWT Access + Refresh Token、HttpOnly Cookie、登录失败锁定、两步验证（TOTP） | 高 |
| 审计日志防篡改 | 仅前端提示「不可篡改」 | 后端追加写 + 哈希链（prev_hash）、按月分区、WORM 存储；禁止 UPDATE/DELETE 权限 | 高 |
| 消息推送通道 | 站内信 / 邮件 / 推送均为模拟 | 接入 SMTP、WebSocket / SSE 站内信、APNs / FCM；增加投递回执表与失败重试 | 中 |
| 实时监控 | 运行状态为静态数据，手动刷新 | 指标采集（Prometheus / Node Exporter）+ WebSocket / SSE 推送；前端 10s 增量更新 | 中 |
| 站点配置生效 | 保存仅更新前端内存 | 后端配置中心 + 缓存刷新；SMTP「测试连接」；配置版本号与乐观锁 | 中 |
| 数据导出 | 审计 / 用户 / 帖子为前端 CSV 下载 | 后端异步导出任务，生成文件后通知下载；接入导出审批状态与有效期 | 中 |
| 敏感词导入 | `importCSV` 仅解析本地文件 | 后端接收 multipart，事务批量导入并返回失败行报告 | 中 |
| 实名认证 | `verifications.html` 未建设 | 实现审核列表与详情；证件号脱敏存储（AES-GCM），仅授权角色可查看 | 中 |
| 图表交互 | SVG 图表无 tooltip / 缩放 / 导出 | 引入 ECharts 或为 SVG 增加 hover tooltip、时间缩放、PNG 导出 | 低 |
| 无障碍 | 拖拽仅鼠标，图表无文本替代 | 增加键盘排序、`aria-*`、图表的 `<table>` 摘要与色盲友好配色 | 低 |
| 国际化 | 仅简体中文，文案硬编码 | 引入 i18n 字典（JSON），按 `data-i18n` 替换；日期与数字使用 `Intl` | 低 |

---

## 6. MySQL 数据库表设计

### 6.1 通用约定

- MySQL **8.0+**，InnoDB，字符集 `utf8mb4`，排序规则 `utf8mb4_0900_ai_ci`。
- 主键统一 `BIGINT UNSIGNED AUTO_INCREMENT`；业务编号（如工单号、公告号）另设 `VARCHAR(32)` 唯一键。
- 所有业务表包含 `created_at DATETIME(3)`、`updated_at DATETIME(3)`；需要软删除的表增加 `deleted_at DATETIME(3) NULL`。
- 状态字段使用 `VARCHAR(20)` + 应用层枚举，避免 MySQL ENUM 变更成本。
- 结构化扩展信息使用 `JSON`（权限点、变更前后值、日志 payload、推送人群），并为高频查询字段建生成列 / 索引。
- 密码使用 **Argon2id** 或 bcrypt，禁止明文；手机号 / 证件号等 PII 加密存储。
- 日志类表不建外键，按月分区；业务表外键按需使用，默认 `ON DELETE RESTRICT`。

### 6.2 关系概览

```
admins ──< admin_roles >── roles ──< role_permissions >── permissions
users ──< bans / mutes / user_verifications / user_points
users ──< posts ──< comments
posts ──< post_likes / favorites / post_attachments
posts/comments ──< report_tickets ──< report_records / report_actions
users ──< feedbacks ──< feedback_replies / feedback_attachments
users ──< user_group_members >── user_groups
announcements ──< announcement_targets ; notification_templates ──< push_tasks ──< push_deliveries
recommendations >── posts ; point_rules / point_logs / trust_levels
export_requests / audit_logs / system_logs / settings / metric_snapshots / api_metrics / error_events
```

### 6.3 账号与权限

#### `admins`（管理员账号，对应 `admins.html` / `login.html`）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | BIGINT UNSIGNED PK | 主键 |
| `account` | VARCHAR(32) UNIQUE | 登录账号 |
| `name` | VARCHAR(32) | 昵称 |
| `email` | VARCHAR(128) | 绑定邮箱 |
| `password_hash` | VARCHAR(255) | Argon2id / bcrypt |
| `password_updated_at` | DATETIME(3) | 密码更新时间 |
| `status` | VARCHAR(20) | `active` / `disabled` |
| `last_login_at` | DATETIME(3) NULL | 最后登录时间 |
| `last_ip` | VARCHAR(45) NULL | IPv4 / IPv6 |
| `remark` | VARCHAR(255) | 备注 |
| `created_at` / `updated_at` | DATETIME(3) | 时间戳 |

索引：`uk_account(account)`、`idx_status(status)`、`idx_last_login(last_login_at)`。

#### `roles` / `permissions` / `role_permissions` / `admin_roles`

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `roles` | `id`、`name`、`key` UNIQUE、`description`、`builtin` TINYINT、`created_at` | 角色，`builtin=1` 表示内置角色 |
| `permissions` | `id`、`module_key`、`module_name`、`point_key` UNIQUE、`point_label`、`sort_order` | 权限点字典，7 模块 28 权限点 |
| `role_permissions` | `role_id`、`permission_id`，联合主键 | 角色-权限多对多 |
| `admin_roles` | `admin_id`、`role_id`，联合主键 | 管理员-角色多对多 |

索引：`role_permissions(role_id)`、`admin_roles(admin_id)`；`permissions(module_key, sort_order)`。

#### `admin_login_logs`（登录审计）

`id`、`admin_id`、`account`、`ip`、`user_agent`、`result`（`success/failed`）、`fail_reason`、`created_at`。索引 `(admin_id, created_at)`、`(ip, created_at)`。

### 6.4 用户与治理

#### `users`（用户，对应 `users.html` / 各业务页）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | BIGINT UNSIGNED PK | 用户 ID |
| `student_id` | VARCHAR(20) UNIQUE | 学号 |
| `nickname` | VARCHAR(32) | 昵称 |
| `real_name` | VARCHAR(32) NULL | 真实姓名（认证后） |
| `email` | VARCHAR(128) | 邮箱 |
| `password_hash` | VARCHAR(255) | 密码 |
| `college_id` | INT UNSIGNED | 学院 |
| `major` | VARCHAR(64) | 专业 |
| `grade` | VARCHAR(8) | 年级，如 `2026级` |
| `role` | VARCHAR(20) | `student` / `moderator` / `admin` |
| `status` | VARCHAR(20) | `active` / `banned` / `muted` / `inactive` |
| `reputation` | INT | 积分余额冗余 |
| `verified` | TINYINT(1) | 是否实名 |
| `avatar_url` | VARCHAR(255) | 头像 |
| `bio` | VARCHAR(255) | 简介 |
| `register_at` | DATETIME(3) | 注册时间 |
| `last_active_at` | DATETIME(3) | 最后活跃 |
| `deleted_at` | DATETIME(3) NULL | 软删除 |

索引：`uk_student_id`、`idx_college(college_id)`、`idx_status(status)`、`idx_register(register_at)`、`idx_last_active(last_active_at)`、`idx_grade(grade)`。

#### `user_verifications`（实名认证）

`id`、`user_id`、`real_name`、`student_id`、`id_card_cipher`（加密）、`id_card_mask`（展示）、`college`、`attachments JSON`、`status`（`pending/approved/rejected`）、`reviewer_id`、`review_note`、`reviewed_at`、`created_at`。索引 `(user_id)`、`(status, created_at)`。

#### `bans`（封禁）

`id`、`user_id`、`type`（`temporary/permanent`）、`reason`、`detail`、`start_at`、`end_at` NULL、`status`（`active/revoked/expired`）、`operator_id`、`revoked_at`、`revoke_note`、`created_at`。索引 `(user_id, status)`、`(status, end_at)`、`(created_at)`。

#### `mutes`（禁言）

`id`、`user_id`、`scope_type`（`global/boards`）、`scope_board_ids JSON`、`reason`、`note`、`start_at`、`end_at`、`status`、`operator_id`、`revoked_at`、`created_at`。索引 `(user_id, status)`、`(end_at)`。

#### `user_groups` / `user_group_members`（定向人群）

`user_groups`：`id`、`name`、`code` UNIQUE、`description`、`rule_json`（动态规则，如活跃天数）、`member_count`、`created_at`。
`user_group_members`：`group_id`、`user_id`，联合主键。索引 `(user_id)`。

### 6.5 内容

#### `boards` / `tags` / `sensitive_words`

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `boards` | `id`、`name`、`slug` UNIQUE、`description`、`sort_order`、`status` | 版块 / 分类，对应 5 个内容分类 |
| `tags` | `id`、`name` UNIQUE、`category`、`description`、`color`、`post_count`、`status`、`created_at` | 标签，`post_count` 可定时回写 |
| `sensitive_words` | `id`、`word` UNIQUE、`category`、`level`（`block/review/warn`）、`match_mode`（`exact/contains/regex`）、`hit_count`、`enabled`、`created_at` | 敏感词 |

索引：`tags(category, status)`；`sensitive_words(category, level)`；如需模糊匹配可加 `FULLTEXT(word)`（ngram）。

#### `posts`（帖子，对应 `posts.html` / `index.html` / `analytics.html`）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | BIGINT UNSIGNED PK | 帖子 ID |
| `author_id` | BIGINT UNSIGNED | 作者 |
| `board_id` | INT UNSIGNED | 分类 / 版块 |
| `tag_id` | INT UNSIGNED NULL | 标签 |
| `title` | VARCHAR(120) | 标题 |
| `summary` | VARCHAR(255) | 摘要 |
| `content` | MEDIUMTEXT | 原始 Markdown / 纯文本 |
| `content_html` | MEDIUMTEXT | 渲染后 HTML（已消毒） |
| `status` | VARCHAR(20) | `pending/published/hidden/deleted/rejected` |
| `is_pinned` / `is_essence` | TINYINT(1) | 置顶 / 精华 |
| `view_count` / `like_count` / `comment_count` / `favorite_count` | INT UNSIGNED | 冗余计数 |
| `ip` | VARCHAR(45) | 发布 IP |
| `created_at` / `updated_at` / `deleted_at` | DATETIME(3) | 时间戳 |

索引：`idx_status_created(status, created_at)`、`idx_board_created(board_id, created_at)`、`idx_tag(tag_id)`、`idx_author(author_id)`、`FULLTEXT(title, summary, content) WITH PARSER ngram`。

#### `post_attachments`

`id`、`post_id`、`type`（`image/file/video`）、`url`、`mime_type`、`size_bytes`、`width`、`height`、`sort_order`、`created_at`。索引 `(post_id, sort_order)`。

#### `comments`（评论，对应 `comments.html`）

`id`、`post_id`、`parent_id` NULL、`author_id`、`reply_to_user_id` NULL、`content`、`content_html`、`status`（`published/hidden/deleted`）、`like_count`、`report_count`、`ip`、`created_at`、`deleted_at`。索引 `(post_id, created_at)`、`(author_id)`、`(status)`。

#### 互动关系表

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `post_likes` | `post_id`、`user_id`、`created_at`，联合主键 | 帖子点赞 |
| `comment_likes` | `comment_id`、`user_id`、`created_at`，联合主键 | 评论点赞 |
| `favorites` | `post_id`、`user_id`、`created_at`，联合主键 | 收藏 |

### 6.6 举报与反馈

#### `report_tickets`（工单主表，对应 `reports.html`）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | BIGINT UNSIGNED PK | 主键 |
| `ticket_no` | VARCHAR(32) UNIQUE | 工单号，如 `EX-...` |
| `target_type` | VARCHAR(20) | `post` / `comment` |
| `target_id` | BIGINT UNSIGNED | 被举报内容 ID |
| `target_snapshot` | JSON | 处置时的内容快照 |
| `report_count` | INT | 举报次数（冗余） |
| `status` | VARCHAR(20) | `pending/processing/resolved/rejected` |
| `priority` | VARCHAR(10) | `low/normal/high` |
| `assignee_id` | BIGINT UNSIGNED NULL | 处理人 |
| `resolution` | VARCHAR(30) | `delete_content/mute_author/ban_author/reject/mark_done` |
| `resolution_note` | TEXT | 处理说明 |
| `created_at` / `resolved_at` | DATETIME(3) | 时间 |

索引：`uk_ticket_no`、`idx_status_created(status, created_at)`、`idx_target(target_type, target_id)`、`idx_assignee(assignee_id)`。

#### `report_records`（每次举报，支持行展开）

`id`、`ticket_id`、`reporter_id`、`reason_code`、`reason_text`、`evidence JSON`、`created_at`。索引 `(ticket_id, created_at)`、`(reporter_id)`。

#### `report_actions`（处置历史）

`id`、`ticket_id`、`operator_id`、`action`、`note`、`before_status`、`after_status`、`created_at`。索引 `(ticket_id, created_at)`。

#### `feedbacks` / `feedback_replies` / `feedback_attachments`

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `feedbacks` | `id`、`user_id`、`type`（`bug/suggestion/complaint/other`）、`title`、`content`、`status`（`pending/processing/resolved`）、`assignee_id`、`created_at`、`updated_at` | 用户反馈 |
| `feedback_replies` | `id`、`feedback_id`、`admin_id`、`content`、`is_internal`、`created_at` | 处理记录 / 回复时间线 |
| `feedback_attachments` | `id`、`feedback_id`、`url`、`mime_type`、`size_bytes`、`created_at` | 附件缩略图 |

索引：`feedbacks(status, created_at)`、`feedbacks(user_id)`、`feedback_replies(feedback_id, created_at)`。

### 6.7 运营

#### `announcements`（公告，对应 `announcements.html`）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | BIGINT UNSIGNED PK | 主键 |
| `title` | VARCHAR(120) | 标题 |
| `summary` | VARCHAR(255) | 摘要 |
| `content` | MEDIUMTEXT | 正文（简化富文本 / HTML） |
| `positions` | JSON | `["banner","modal","sidebar"]` |
| `scope_type` | VARCHAR(20) | `all` / `groups` |
| `status` | VARCHAR(20) | `scheduled/active/ended/disabled`（可由时间与 enabled 计算） |
| `start_at` / `end_at` | DATETIME(3) | 定时投放 |
| `enabled` | TINYINT(1) | 启用开关 |
| `creator_id` | BIGINT UNSIGNED | 创建人 |
| `view_count` | INT UNSIGNED | 曝光 / 阅读数 |
| `created_at` / `updated_at` | DATETIME(3) | 时间 |

索引：`idx_window(enabled, start_at, end_at)`、`idx_status(status)`。`positions` 若需按位置筛选，可建生成列 `has_banner` 或多列布尔字段。

#### `announcement_targets`

`announcement_id`、`group_id`，联合主键。索引 `(group_id)`。

#### `notification_templates`（消息模板）

`id`、`name`、`code` UNIQUE、`scene`、`channels` JSON（`["inbox","email","push"]`）、`title_template`、`body_template`、`variables` JSON、`enabled`、`created_at`、`updated_at`。

#### `push_tasks` / `push_deliveries`

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `push_tasks` | `id`、`title`、`content`、`audience` JSON（学号范围 / 用户组 / 活跃状态）、`channels` JSON、`send_at`、`status`（`scheduled/sending/sent/failed`）、`estimated_count`、`sent_count`、`creator_id`、`created_at` | 定向推送任务 |
| `push_deliveries` | `id`、`task_id`、`user_id`、`channel`、`status`（`pending/sent/failed/read`）、`error_message`、`sent_at`、`read_at` | 投递明细，可分区 |

索引：`push_tasks(status, send_at)`、`push_deliveries(task_id, status)`、`push_deliveries(user_id)`。

#### `recommendations`（推荐位，对应 `recommendations.html`）

`id`、`position`（`global/category/carousel/sidebar`）、`post_id`、`sort_order`、`start_at`、`end_at`、`created_by`、`created_at`。索引 `(position, sort_order)`、`(post_id)`、`(start_at, end_at)`；唯一约束 `uk_position_post(position, post_id)` 防止重复推荐。

#### 积分与等级（对应 `points.html`）

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `trust_levels` | `level` TINYINT PK、`name`、`min_points`、`requirement`、`permission_desc`、`mode`（`auto/manual`）、`updated_at` | L0-L4 信任等级 |
| `point_rules` | `id`、`action_code` UNIQUE、`action_name`、`points` INT（可为负）、`daily_cap` INT、`enabled`、`updated_at` | 积分规则 |
| `user_points` | `user_id` PK、`balance` INT、`level` TINYINT、`updated_at` | 用户积分余额与等级 |
| `point_logs` | `id`、`user_id`、`action_code`、`delta`、`balance_after`、`ref_type`、`ref_id`、`created_at` | 积分流水，按月分区 |

索引：`point_logs(user_id, created_at)`、`point_logs(action_code, created_at)`；`user_points(level, balance)` 支持排行榜。

### 6.8 系统与运维

#### `export_requests`（导出审批，对应 `export-approval.html`）

`id`、`request_no` UNIQUE、`applicant_id`、`data_type`、`scope` VARCHAR(255)、`filters` JSON、`volume` INT、`reason` TEXT、`status`（`pending/approved/rejected`）、`approver_id`、`review_note`、`reviewed_at`、`created_at`。索引 `(status, created_at)`、`(applicant_id)`。

#### `audit_logs`（操作审计，对应 `audit-logs.html`）

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | BIGINT UNSIGNED PK | 主键 |
| `operator_id` | BIGINT UNSIGNED | 操作人 |
| `operator_name` | VARCHAR(32) | 快照，防止改名后失真 |
| `role_name` | VARCHAR(64) | 操作时角色快照 |
| `action` | VARCHAR(32) | 操作类型 |
| `target_type` / `target_id` / `target_name` | VARCHAR / BIGINT / VARCHAR | 目标对象 |
| `ip` | VARCHAR(45) | 来源 IP |
| `result` | VARCHAR(10) | `success/failed` |
| `detail` | VARCHAR(500) | 操作说明 |
| `changes` | JSON | `[{ label, before, after, kind }]` |
| `prev_hash` / `hash` | CHAR(64) | 哈希链，防篡改 |
| `created_at` | DATETIME(3) | 时间 |

索引：`(created_at)`、`(operator_id, created_at)`、`(action, created_at)`、`(target_type, target_id)`。建议按天 / 月分区，数据库账号仅授予 `INSERT/SELECT`。

#### `system_logs`（系统日志，对应 `system-logs.html`）

`id`、`level`（`ERROR/WARN/INFO/DEBUG`）、`module`、`message`、`payload` JSON、`trace_id`、`request_id`、`created_at`。索引 `(level, created_at)`、`(module, created_at)`、`(trace_id)`；按月分区，保留 30～90 天。

#### `settings` / `settings_history`（站点配置）

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `settings` | `id`、`group_key`、`config_key` UNIQUE、`value_json` JSON、`value_type`（`switch/number/select`）、`label`、`description`、`sort_order`、`updated_at` | 站点配置项 |
| `settings_history` | `id`、`operator_id`、`before_json`、`after_json`、`created_at` | 配置变更历史 |

索引：`settings(group_key, sort_order)`。

#### 监控指标

| 表 | 关键字段 | 说明 |
| --- | --- | --- |
| `metric_snapshots` | `id`、`captured_at`、`cpu_percent`、`memory_percent`、`disk_percent`、`online_users`、`net_in_mbps`、`net_out_mbps` | 每分钟资源快照，用于 KPI 与 sparkline |
| `api_metrics` | `id`、`bucket_at`、`qps`、`avg_rt_ms`、`p95_rt_ms`、`error_count` | 分钟 / 小时聚合，对应 API 双轴图 |
| `error_events` | `id`、`window_start`、`path`、`status_code`、`occurrences`、`source_ip` | 异常请求聚合 |

索引：`metric_snapshots(captured_at)`、`api_metrics(bucket_at)`、`error_events(window_start, status_code)`；建议设置 TTL / 分区清理。

#### `admin_todos`（运营看板待办）

`id`、`admin_id`、`title`、`type`、`ref_type`、`ref_id`、`done`、`due_at`、`created_at`。索引 `(admin_id, done, due_at)`。

### 6.9 示例 DDL

```sql
CREATE TABLE admins (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  account VARCHAR(32) NOT NULL,
  name VARCHAR(32) NOT NULL,
  email VARCHAR(128) NOT NULL,
  password_hash VARCHAR(255) NOT NULL,
  password_updated_at DATETIME(3) NULL,
  status VARCHAR(20) NOT NULL DEFAULT 'active',
  last_login_at DATETIME(3) NULL,
  last_ip VARCHAR(45) NULL,
  remark VARCHAR(255) NOT NULL DEFAULT '',
  created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
  UNIQUE KEY uk_account (account),
  KEY idx_status (status),
  KEY idx_last_login (last_login_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE posts (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  author_id BIGINT UNSIGNED NOT NULL,
  board_id INT UNSIGNED NOT NULL,
  tag_id INT UNSIGNED NULL,
  title VARCHAR(120) NOT NULL,
  summary VARCHAR(255) NOT NULL DEFAULT '',
  content MEDIUMTEXT NOT NULL,
  content_html MEDIUMTEXT NOT NULL,
  status VARCHAR(20) NOT NULL DEFAULT 'pending',
  is_pinned TINYINT(1) NOT NULL DEFAULT 0,
  is_essence TINYINT(1) NOT NULL DEFAULT 0,
  view_count INT UNSIGNED NOT NULL DEFAULT 0,
  like_count INT UNSIGNED NOT NULL DEFAULT 0,
  comment_count INT UNSIGNED NOT NULL DEFAULT 0,
  favorite_count INT UNSIGNED NOT NULL DEFAULT 0,
  ip VARCHAR(45) NULL,
  created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
  deleted_at DATETIME(3) NULL,
  KEY idx_status_created (status, created_at),
  KEY idx_board_created (board_id, created_at),
  KEY idx_tag (tag_id),
  KEY idx_author (author_id),
  FULLTEXT KEY ft_content (title, summary, content) WITH PARSER ngram
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE audit_logs (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  operator_id BIGINT UNSIGNED NOT NULL,
  operator_name VARCHAR(32) NOT NULL,
  role_name VARCHAR(64) NOT NULL DEFAULT '',
  action VARCHAR(32) NOT NULL,
  target_type VARCHAR(20) NOT NULL,
  target_id BIGINT UNSIGNED NULL,
  target_name VARCHAR(255) NOT NULL DEFAULT '',
  ip VARCHAR(45) NOT NULL,
  result VARCHAR(10) NOT NULL DEFAULT 'success',
  detail VARCHAR(500) NOT NULL DEFAULT '',
  changes JSON NULL,
  prev_hash CHAR(64) NULL,
  hash CHAR(64) NOT NULL,
  created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  KEY idx_created (created_at),
  KEY idx_operator (operator_id, created_at),
  KEY idx_action (action, created_at),
  KEY idx_target (target_type, target_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```

### 6.10 索引、分区与安全建议

- **中文搜索**：`posts` 的全文索引使用 `ngram` 分词；敏感词模糊匹配可单独使用 Aho-Corasick 服务，不必全表 `LIKE`。
- **高频筛选**：内容 / 用户 / 日志均按「状态 + 时间」建复合索引，遵循最左前缀；避免在枚举字段上单独建过多低区分度索引。
- **大表分区**：`audit_logs`、`system_logs`、`point_logs`、`push_deliveries` 按月分区；监控指标按天清理。
- **计数冗余**：帖子 / 评论的点赞、评论、收藏数使用冗余列，由异步任务或事务内更新，避免实时 `COUNT(*)`。
- **审计不可变**：`audit_logs` 仅授予 INSERT/SELECT，配合 `prev_hash` 哈希链校验；导出需写入审计。
- **PII 保护**：身份证、手机号使用 AES-GCM 加密；列表仅返回脱敏字段，密钥托管 KMS。
- **权限落库**：`permissions.point_key` 与前端 `PERMISSION_MODULES` 一一对应，新增权限点需同时更新前端矩阵与后端校验。

---

## 7. 新增页面指引

1. 复制任意已完成页面结构，将 `<main data-page="xxx">` 中的内容替换为新内容。
2. 引入脚本（顺序固定）：

```html
<link rel="stylesheet" href="assets/style.css">
<script src="assets/app.js"></script>
<script src="assets/mock.js"></script>
<script src="assets/charts.js"></script>
```

3. 在 `assets/app.js` 的 `MENU` 中将对应菜单项的 `href` 改为新文件名；页面脚本显式调用 `App.renderLayout('xxx')`，布局注入具备幂等保护。
4. 表格统一使用 `App.renderTable(container, columns, rows, options)`，分页统一使用 `App.renderPagination(container, options)`。
5. 页面级样式只追加到 `assets/style.css` 末尾，复用既有变量与组件类，不覆盖全局规范。

## 8. 通用能力

- **布局**：侧边栏可折叠（240px / 64px），≤1024px 自动折叠；顶栏包含面包屑、全局搜索（⌘K / Ctrl+K）、通知、主题切换、账号菜单。
- **组件**：`App.toast`、`App.modal`、`App.drawer`（支持 `side: 'bottom'` 底部抽屉）、`App.renderTable`、`App.renderPagination`、`App.avatarHTML`、`App.pill`、`App.emptyHTML`。
- **图表**：`Charts.line`（支持 `dualAxis` 双 Y 轴）、`Charts.bar`（支持 `stacked` 堆叠）、`Charts.donut`、`Charts.sparkline`、`Charts.progress`，全部为手写 SVG。
- **主题**：默认浅色，顶栏可切换深色；侧边栏折叠与主题偏好写入 `localStorage`。
- **浏览器**：Chrome / Edge 最新版；使用原生 `fetch`、`IntersectionObserver`、HTML5 Drag & Drop、`navigator.clipboard`。
