# 全局架构与跨模块机制

> 适用代码：`backend/`　最后核对：2026-11-24\
> 本文只描述**全局**设计；具体接口见 [modules.md](./modules.md)。

## 1. 技术栈与构建目标

| 类别      | 选型                              |
| ------- | ------------------------------- |
| 语言      | C++17                           |
| HTTP 框架 | Drogon                          |
| JSON    | jsoncpp                         |
| 加密      | OpenSSL（HMAC-SHA256、PBKDF2）     |
| 数据库     | MySQL，MySQL Connector/C++       |
| 缓存      | Redis，redis-plus-plus + hiredis |
| 配置      | 自研 INI 单例 + 环境变量展开              |
| 日志      | 自研同步日志单例                        |
| 构建      | CMake，CTest                     |
| 测试      | doctest 2.4.11                  |

根 `CMakeLists.txt` 产出三个业务目标：

| 目标                 | 内容                                         |
| ------------------ | ------------------------------------------ |
| `sdu_infra`（静态库）   | 连接池、INI、日志、配置、密码哈希、JWT 签名器                 |
| `sdu_app`          | 中间件、路由、Controller、Service、Repository、utils |
| `sdu_forum_server` | `app/` 层入口、`AppContext`、三个业务模块             |
| `tests`            | 5 个单测可执行文件，接入 CTest                        |

## 2. 目录与分层

```
backend/
├── app/                     # 组装层：入口、AppContext、模块注册
│   ├── app.cpp              # main：定位配置 -> 初始化 -> 注册 -> listen
│   ├── app_context.{h,cpp}  # 共享依赖的唯一持有者
│   └── modules/             # Module 实现，只注册路由
├── routes/                  # URL -> Controller 绑定 + 中间件声明
├── controllers/             # HTTP 协议层：参数校验、响应构造
│   └── admin/               # 管理端 Controller
├── services/                # 业务编排层
│   └── admin/               # 管理端 Service + BanService
├── repositories/            # 数据访问层（每类一个 Repository）
├── middleware/              # 认证、角色、异常处理
├── common/                  # 响应、错误码、封禁枚举（无状态）
├── utils/                   # Bearer 解析、trim、HMAC、时间格式化
├── infrastructrue/          # 基础设施库（注意目录名拼写）
│   ├── database-connection-pool/
│   ├── redis-connection-pool/
│   ├── ini/  log/  config/  auth/  security/  codec/  error/  rabbitMQ-pool/
├── config/config-dev.ini
├── docs/
└── tests/
```

### 2.1 依赖方向（硬约束）

```
app -> routes -> controllers -> services -> repositories -> infrastructrue
                                   |
                                   +-> common（错误码 / 枚举）
middleware -> services(BanService) / repositories / infrastructrue
```

- Controller **不得**直接访问 Repository。
- Repository **不得**包含业务判断（例如「是否重复封禁」应在 Service）。
- Controller **不得**自行拼 JSON 错误响应，必须抛 `AppException`。
- routes **不得**写业务逻辑，只做绑定。
- 单例（`AppConfig`、`IniFile`、`Logger`、连接池）只在 `app` 层初始化/关闭。

## 3. 启动流程

入口 `app/app.cpp`：

1. **定位配置文件**，优先级从高到低：
   1. 环境变量 `SDU_FORUM_CONFIG`
   2. `argv[1]`
   3. `config/config-dev.ini`
   4. `../config/config-dev.ini`
   5. `../../config/config-dev.ini`
2. `infra::AppConfig::instance().load(path)`：加载 INI、展开环境变量、`validate()`。
3. 初始化日志（级别、文件、控制台、按行轮转）。
4. 构造 `AppContext`：创建 DB/Redis 连接池、Provider、Repository、Service、6 个中间件、3 个模块。
5. `init_infrastructure()`：连接池 `init()`（最小连接、回收线程）。
6. `register_middlewares()`：向 Drogon 注册中间件类。
7. `register_routes()`：三个模块分别注册路由。
8. 注册 `PostHandlingAdvice` 统一访问日志。
9. 设置监听地址/端口、线程数，`app().run()`。

`AppContext` 析构时对两个连接池调用 `shutdown()`。**连接池必须在 Repository 之前构造、之后析构**，否则 Repository 持有悬空引用。

## 4. 组件装配（AppContext）

`AppContext` 是唯一装配点，持有：

- `dcp::ConnectionPool*`（MySQL）、`rcp::RedisConnectionPool*`（Redis）
- `PooledDbConnectionProvider`、`PooledCacheConnectionProvider`（实现 repository 侧接口）
- `repository::TokenRepository`、`BlacklistRepository`、`AccountRepository`、`AdminAccountRepository`
- Service：`AuthService`、`admin::AuthService`、`admin::AdminUserService`
- `admin::service::BanService`（被认证中间件与 `AuthService` 复用）
- 中间件：`AccessAuthMiddleware`、`RefreshAuthMiddleware`、`AdminAccessAuthMiddleware`、`AdminRefreshAuthMiddleware`、`UserRoleMiddleware`、`AdminRoleMiddle`
- 模块：`AuthModule`、`admin::AuthModule`、`admin::UserModule`

**装配规则**：Provider 必须先于 Repository 构造；Repository 先于 Service；Service 先于 Module。所有对象只构造一次，禁止在 Controller 内 `new` 连接或 Repository。

模块接口（`app/modules/module.h`）：

```cpp
class Module {
public:
    virtual ~Module() = default;
    virtual void register_routes(drogon::HttpAppFramework& app) = 0;
};
```

模块只做一件事：把 Controller 的成员函数绑定到 URL。新增模块必须同时修改 `AppContext`。

已知例外：`admin::AuthModule` 的构造函数直接接收 `PooledDbConnectionProvider` 具体类型（而非 `IDbConnectionProvider`），因此管理员认证链路无法用接口替身做单测（R23）。新模块应优先依赖接口。

## 5. 配置系统

### 5.1 加载机制

- 基于 `IniFile` 单例，`AppConfig` 在启动时加载一次并全进程只读。
- 所有字符串值支持 `${VAR}` 与 `${VAR:-default}` 展开；环境变量存在但为空串时使用默认值。
- INI 节不存在时返回空节，不抛异常；键不存在时使用结构体默认值。
- `bool` 解析：`true` / `TRUE` / `1` 为真，其余为假。

### 5.2 配置项清单（与 `AppConfig::load` 对齐）

| 节          | 键                              | 类型         | 默认 / 说明                                                  |
| ---------- | ------------------------------ | ---------- | -------------------------------------------------------- |
| `server`   | `address`                      | string     | `0.0.0.0`                                                |
|            | `port`                         | int        | `8080`，必须非 0                                             |
|            | `thread_num`                   | int        | `4`                                                      |
| `log`      | `level`                        | string     | `INFO`                                                   |
|            | `file`                         | string     | 结构体默认 `logs/server.log`；开发 INI 为 `../logs/server.log`    |
|            | `console`                      | bool       | `true`                                                   |
|            | `max_lines`                    | int        | `100000`，按行轮转阈值                                          |
| `database` | `host` / `port`                | string/int | `127.0.0.1` / `3306`                                     |
|            | `user` / `password` / `dbname` | string     | user、dbname 必填                                           |
|            | `pool_min` / `pool_max`        | int        | `5` / `30`，`min>0` 且 `max>=min`                          |
|            | `max_idle_time`                | int        | `300` 秒                                                  |
|            | `connection_timeout_ms`        | int        | `3000`                                                   |
|            | `op_num`                       | int        | `4`，每次扩容连接数                                              |
| `redis`    | `host` / `port`                | string/int | `127.0.0.1` / `6379`                                     |
|            | `passward`                     | string     | **代码读取的就是这个拼写**；标准 `password` 无效                         |
|            | `db`                           | int        | `0`                                                      |
| `auth`     | `issuer`                       | string     | `SDU_Forum`                                              |
|            | `access_jwt_secret`            | string     | 必填                                                       |
|            | `refresh_jwt_secret`           | string     | 必填                                                       |
|            | `admin_access_secret`          | string     | **配置文件未提供，当前为空；`validate` 不校验**                          |
|            | `admin_refresh_secret`         | string     | 同上                                                       |
|            | `access_token_minutes`         | int        | `60`，必须 >0                                               |
|            | `refresh_token_hours`          | int        | `168`，必须 >0                                              |
|            | `hash_secret`                  | string     | **硬编码** `dfacadgafeqwfqvab`，`AppConfig::load` 不读取该键（R20） |
|            | `account_table`                | string     | `account`，必填                                             |
|            | `blacklist_table`              | string     | `blacklist`，必填                                           |
|            | `admin_account_table`          | string     | `admin_account`，可缺省                                      |
|            | `refresh_token_table`          | string     | **未从 INI 读取**，恒为结构体默认 `refresh_tokens`                   |

### 5.3 配置约束

- 新增必填配置必须加入 `AppConfig::validate()`；否则线上会以空值运行。
- 表名一律来自 `AuthConfig`，Repository 构造函数接收表名，**禁止在 SQL 中硬编码表名**。
- 密钥不得写入日志；`AppConfig` 不提供打印全量配置的方法。
- 修改 `config-dev.ini` 时注意键名唯一，当前 `access_jwt_secret`/`refresh_jwt_secret` 各重复两次，后者覆盖前者。
- `hash_secret` 不从 INI 读取，所有环境共用同一默认值；轮换吊销哈希密钥需要改代码并接受旧记录失效。
- `refresh_token_table` 不参与加载，表名变更只能改代码。

## 6. HTTP 路由与中间件

### 6.1 路由注册方式

`routes/routes.cpp` 中三个函数：

- `register_auth_routes`：用户认证
- `register_admin_auth_routes`：管理员认证
- `register_admin_user_routes`：管理员用户管理

每个 handler 的第三个参数同时声明 HTTP 方法与中间件链，顺序即执行顺序：**认证中间件在前，角色中间件在后**。

### 6.2 中间件清单

| 类                            | 作用                               | 关键属性               |
| ---------------------------- | -------------------------------- | ------------------ |
| `AccessAuthMiddleware`       | 校验用户 access token、查账号、查 `All` 封禁 | `public_id`、`role` |
| `RefreshAuthMiddleware`      | 校验用户 refresh token（同上）           | `public_id`、`role` |
| `AdminAccessAuthMiddleware`  | 校验管理员 access token，不查账号/封禁       | `admin_id`、`role`  |
| `AdminRefreshAuthMiddleware` | 校验管理员 refresh token              | `admin_id`、`role`  |
| `UserRoleMiddleware`         | 要求 `role == sduforum_user`       | 读取 `role`          |
| `AdminRoleMiddle`            | 要求 `role == sduforum_admin`      | 读取 `role`          |

- 用户与管理员**共用属性键 `role`**，但各自路由链只挂自己的 Role 中间件。
- `current_public_id(req)` / `current_admin_id(req)` 是 Controller 获取身份的**唯一**方式，禁止从 Header 重新解析。
- 异常中间件 `install_exception_handler()` 必须最先安装，保证所有 `AppException` 被转为统一 JSON。

### 6.3 中间件执行顺序（用户受保护接口）

```
异常处理 -> 解析 Bearer -> 查吊销表/缓存 -> 验签 -> 查 account -> 查 All 封禁 -> Role 校验 -> Controller
```

管理员链跳过「查 account / 查封禁」。

## 7. 统一响应与错误处理

### 7.1 响应包络（硬约束）

```json
// 成功
{ "data": { ... } }

// 创建
HTTP 201 + { "data": { ... } }

// 无内容
HTTP 204，无 body

// 失败
{ "error": { "code": "INVALID_PARAM", "message": "...", "details": { ... } } }
```

必须使用 `common/api_response.h`：

- `json_response(status, value)`
- `data_response(status, data)`
- `created_response(data)` → 201
- `no_content_response()` → 204
- `error_response(...)`

### 7.2 错误码映射

| `ErrorCode`          | 字符串                   | HTTP | 典型场景                      |
| -------------------- | --------------------- | ---- | ------------------------- |
| `InvalidJson`        | `INVALID_JSON`        | 400  | body 非 JSON、缺字段           |
| `InvalidParam`       | `INVALID_PARAM`       | 400  | 类型/范围/枚举非法                |
| `Unauthorized`       | `UNAUTHORIZED`        | 401  | 缺 token、token 无效/已吊销/过期   |
| `InvalidCredentials` | `INVALID_CREDENTIALS` | 401  | 账号不存在或密码错误                |
| `AccountBanned`      | `ACCOUNT_BANNED`      | 403  | 登录时命中封禁                   |
| `Forbidden`          | `FORBIDDEN`           | 403  | 角色不匹配                     |
| `NotFound`           | `NOT_FOUND`           | 404  | 账号/封禁记录不存在                |
| `Conflict`           | `CONFLICT`            | 409  | 重复封禁                      |
| `DatabaseError`      | `DB_ERROR`            | 500  | SQL 异常、连接不可用              |
| `RedisError`         | `" REDIS_ERROR"`      | 500  | 缓存异常；**注意名称含前导空格（缺陷 R7）** |
| `InternalError`      | `INTERNAL_ERROR`      | 500  | 未预期异常                     |

### 7.3 异常规范

- Service / Repository 抛出 `common::AppException(code, message, details?)`。
- Controller 只负责协议校验，校验失败也抛 `AppException`。
- 全局异常中间件把 `AppException` 转为对应状态码；其他异常统一 500 `internal server error`，**不泄漏内部信息**。
- `details` 必须是 JSON 对象或 null，禁止放裸字符串。

## 8. 认证与 Token 体系

### 8.1 JWT 结构

HS256，三段 Base64URL（无 padding）。Header 固定：

```json
{ "alg": "HS256", "typ": "JWT" }
```

Payload：

```json
{ "iss": "SDU_Forum", "iat": 1730000000, "exp": 1730003600, "sub": "usr_xxx", "role": "sduforum_user" }
```

- 用户 `sub = public_id`，`role = sduforum_user`。
- 管理员 `sub = admin_id`，`role = sduforum_admin`。
- 校验顺序：段数 → 签名（`CRYPTO_memcmp`）→ `alg` → `iss` → `sub` 非空 → `role` 非空 → `exp` 未过期。**先验签，后解析 payload**。
- 4 套密钥：用户 access/refresh、管理员 access/refresh，互不通用。
- TTL：access 默认 60 分钟，refresh 默认 168 小时。

### 8.2 Token 吊销（历史多次调整的核心设计）

问题：JWT 无状态，登出/注销后旧 token 在过期前仍有效。

当前方案（**必须保持两边一致**）：

| Token         | 存储                                                | 键 / 值                     | TTL               |
| ------------- | ------------------------------------------------- | ------------------------- | ----------------- |
| refresh token | MySQL `refresh_tokens` 表（**仅吊销时写入**，有效 token 不落库） | `token_hash`              | 写入时带 `expires_at` |
| access token  | Redis                                             | key/value 均为 `token_hash` | access TTL        |

- `TokenRepository::hash_secret()` 只是返回 `JwtConfig::hash_secret`（**硬编码默认值**，不从 INI 读取）；Service/中间件调用 `utils::hmac_sha256_base64url(hash_secret, token)` 生成存储哈希，**不存明文**。
- 中间件的判定顺序：**先查吊销，再验签**。原因：吊销记录可覆盖「签名仍有效但已登出」的情况。
- refresh 查询：先 Redis，未命中/异常降级 DB；DB 命中后 best-effort 回写 Redis。
- access 查询：只查 Redis；Redis 不可用时返回「未吊销」（fail-open），这是当前取舍。
- 写入失败语义：refresh 入库失败 → `DB_ERROR`；access 写 Redis 失败在 refresh 流程中被忽略，在 logout 中视为失败。

### 8.3 登出 / 刷新 / 注销的 token 流转

- **logout**：Header 放 access，body 放 refresh。校验 refresh 未被吊销 → 验签 → 旧 refresh 哈希入库 → 旧 access 哈希写 Redis。
- **refresh-token**：Header 放 refresh，body 放 `refreshing` 与旧 access。
  - 始终签发新 access，并把旧 access 写入吊销缓存；
  - `refreshing=true` 时额外签发新 refresh，并把旧 refresh 入库；
  - `refreshing=false` 时 `LoginResult.refresh_token` 为空串、`refresh_token_exp=0`，响应仍保留这两个字段，客户端需判断。
- **logoff**：删除账号（按 `public_id`），并吊销当前 access+refresh。

## 9. 封禁体系

### 9.1 维度（`BanType`）

| 值 | 名称               | 语义     |
| - | ---------------- | ------ |
| 0 | `All`            | 全站封禁   |
| 1 | `Comment`        | 禁止评论   |
| 2 | `Post`           | 禁止发帖   |
| 3 | `PrivateMessage` | 禁止私信   |
| 4 | `Login`          | 禁止登录   |
| 5 | `ProfileEdit`    | 禁止修改资料 |
| 6 | `Upload`         | 禁止上传   |
| 7 | `ReadOnly`       | 只读模式   |

### 9.2 原因码分段（`ReasonCode`）

| 段                | 含义    | 备注                                 |
| ---------------- | ----- | ---------------------------------- |
| 0                | 自定义原因 | 必须提供 `reason` 原文                   |
| 1001–1008        | 内容违规  | 广告、违法、色情、暴力、政治、版权、隐私、谣言            |
| 2001–2007        | 行为违规  | 人身攻击、骚扰、仇恨、冒充、屡犯、恶意举报、引战           |
| 3001–3004        | 账号安全  | 盗号、共享、机器人、安全风险                     |
| 4001–4002        | 系统自动  | `auto_only=true`，来源必须是 auto/report |
| 5001–5003 / 5999 | 其他    | 用户申请、长期未登录、其他、管理员自定义               |

**枚举只增不改**：废弃码保留数值并标记 `deprecated`。

### 9.3 来源与状态

- `BanSource`：`manual` / `auto` / `report`，入参为字符串。
- `BanStatus`：`1=生效中`、`0=已解除`、`2=已过期`；对外文案 `active` / `revoked` / `expired`。

### 9.4 生效判定（硬约束）

统一为：

```sql
status = 1 AND expire_at > NOW()
```

- 查询时**不依赖** `status` 单独判断是否过期。
- `expire_outdated()` 只是全局兜底：把 `status=1 AND expire_at<NOW()` 置为 `2`，并把 `unbanned_at=expire_at`。
- 永久封禁用哨兵时间 `9999-12-30 23:59:59`（`PERMANENT_EXPIRE`），因此天然满足 `> NOW()`。
- 未解封哨兵 `1970-01-01 00:00:00`（`NOT_UNBANNED`）已定义；`insert` 不写 `unbanned_at`，依赖 DB 默认值写入该哨兵值（列是 `NOT NULL`，不是 NULL）。

### 9.5 校验规则（`ValidateBanInput`）

1. `reason_code` 必须是枚举合法值；
2. `ban_type` 必须合法；
3. 来源一致性：`auto_only` 或 4xxx 原因必须 `source ∈ {auto, report}`；
4. 文本一致性：`Unspecified` / `ManualCustom` **按规则**必须提供非空 `reason`。
   - 注意实际实现：`AdminUserService::resolve_reason_text` 会先用标准文案（`（自定义原因）` / `（管理员自定义）`）兜底，再传给 `ValidateBanInput`，因此空自定义原因**不会**触发该失败（风险 R17）。
5. `reason` 长度 ≤ 255。

补充（Controller/Service 层）：

- `account_id > 0`；
- `time_type ∈ {0,1,2,0xFFFF}`；非永久时 `time ∈ [1, 0xFFFFFFFF]`；
- `time_type` 单位：0=分钟、1=小时、2=天；
- 服务层还会把标准原因码转成中文文案落库（`resolve_reason_text`）。

### 9.6 封禁判定的分层策略（核心设计）

代码注释明确：

> 在鉴权中间件中检测 `All`，能立即响应全局封禁；具体 `banType` 在执行具体业务前检测。

因此：

- `All`：只由认证中间件统一拦截，业务层**不要**重复判断；
- 具体维度：由对应业务 Service 在动作前查询 `BanService::is_banned(account_id, type)`；
- 不要写「All 与单维度联合判断」的重复逻辑。

### 9.7 幂等与解除语义

- 已有 `All` 生效 → 任何新封禁都拒绝（`CONFLICT`）；
- 已有同维度生效 → 拒绝；
- 已有具体维度、再封 `All` → 允许；
- 解除单维度：只解除该维度；
- 解除 `All`：解除该账号全部生效记录；
- 存在 `All` 时解除单维度：Repository 更新 0 行 → `NOT_FOUND`（符合「All 不能被单维度解除」的设计）。

## 10. 跨模块公共机制

| 机制   | 实现                                                                          | 约束                                                                                                                          |
| ---- | --------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| 连接获取 | `repository::IDbConnectionProvider::acquire()` / `ICacheConnectionProvider` | Repository 只依赖接口，禁止直接用连接池单例                                                                                                 |
| RAII | `PooledDbConnection` / `PooledCacheConnection`                              | 离开作用域自动归还；空守卫表示获取失败                                                                                                         |
| 表名注入 | Repository 构造函数接收表名                                                         | 禁止硬编码                                                                                                                       |
| 错误转换 | Repository 捕获 `sql::SQLException` → `DB_ERROR`                              | 禁止把 SQL 异常泄漏到上层                                                                                                             |
| 身份注入 | 中间件 attributes                                                              | `public_id` / `admin_id` / `role`                                                                                           |
| 时间格式 | `"YYYY-MM-DD HH:MM:SS"`                                                     | 封禁 `expire_at` 用**本地时区**（`add_time_to_now`）生成并与 `NOW()` 比较；`utils::timestamp_to_string` 用 **UTC**，仅用于 refresh token 审计（R21） |
| 日志   | `LOGD/LOGI/LOGW/LOGE`                                                       | 敏感信息（密码、token）禁止打印                                                                                                          |
| 单例   | `AppConfig` / `IniFile` / `Logger` / 两个连接池                                  | 只在 `app` 层初始化；测试用替身不要触发单例                                                                                                   |

## 11. 请求生命周期（用户接口示例）

```
Drogon 收到请求
  -> PostHandlingAdvice 记录访问日志（响应后）
  -> AccessAuthMiddleware
       - 取 Authorization: Bearer <token>
       - parse_bearer 校验格式
       - TokenRepository::find_access_token(hash) 命中 -> 401
       - TokenSigner.verify -> 失败 401
       - AccountRepository.find_account_by_public_id -> 不存在时当前未判空，会解引用空 optional（R19）
       - BanService.is_banned(id, All) -> 封禁 403
       - 写入 attributes: public_id / role
  -> UserRoleMiddleware
       - role != sduforum_user -> 403
  -> Controller
       - JSON / 参数校验 -> 失败抛 AppException
       - 调 Service
  -> Service -> Repository -> DB/Redis
  -> Controller 构造 data_response / no_content_response
  -> 异常中间件（若抛出）转换为 error_response
```

## 12. 全局硬约束速查

1. 响应只能通过 `common/api_response.h` 构造。
2. 业务失败只能抛 `AppException`，禁止返回自定义错误 JSON。
3. 表名必须来自配置。
4. 用户身份只能从中间件 attributes 读取。
5. `All` 封禁只在中间件判断；具体维度在业务动作前判断。
6. Token 吊销检查必须在验签之前。
7. access / refresh / admin 四套密钥不得混用。
8. `BanType`、`ReasonCode` 只增不改。
9. 过期判定必须带 `expire_at > NOW()`。
10. 中间件中任何「已写响应」的分支必须立即 `return`。
11. 新增必填配置必须进入 `validate()`。
12. 单元测试不得访问真实 MySQL/Redis/网络。
13. 管理员 ID（`admin_id`、`banned_by`、`unbanned_by`）长度上限 8；需要更长标识必须先改 DDL 与 Controller。
14. 与 DB `NOW()` 比较的时间必须用本地时区生成；UTC 时间只能用于展示/审计。
15. `banned_by/unbanned_by` 存 `admin_id` 字符串，不是管理员表主键 `id`。
