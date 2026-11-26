# 后端集成测试说明

> 对应实现：`backend/tests/integration/`；入口脚本：`tests/integration/scripts/run_integration.sh`。
> 当前规模：13 个测试二进制、135 个用例、14 个 CTest 条目（含 1 个建表夹具），全量通过。

## 1. 分层架构

```
tests/integration/
├── CMakeLists.txt                 # 集成测试目标、CTest fixtures/labels
├── config/config-int.ini          # 测试专用配置（127.0.0.1:18080 / it_ 表 / Redis db 15）
├── sql/schema.sql                 # it_account / it_admin_account / it_refresh_tokens / it_blacklist
├── support/
│   ├── it_env.{h,cpp}             # 环境变量、DB/Redis 可用性探测、安全护栏
│   ├── it_support.{h,cpp}         # ItEnvironment 单例、建表/清库、数据播种、SQL 断言助手
│   ├── http_client.{h,cpp}        # POSIX socket HTTP/1.1 客户端 + jsoncpp 响应断言助手
│   ├── server_process.{h,cpp}     # fork/exec 启动 sdu_forum_server、探活、优雅停止、日志落盘
│   └── it_test_common.h           # JSON 序列化、Bearer 头、data 节点读取等公共助手
├── infra/                         # L4：配置 / MySQL 池 / Redis 池
├── repositories/                  # L3：账号、Token、黑名单仓储
├── services/                      # L2：用户鉴权、管理员鉴权、封禁编排
├── api/                           # L1：真实 HTTP 接口契约
└── scripts/                       # setup_db.sh / run_integration.sh / report_html.py / coverage_summary.py
```

| 层级 | 被测对象 | 依赖 | 边界 |
| --- | --- | --- | --- |
| L1 `api/` | 真实 `sdu_forum_server` 子进程 + HTTP | 真实 MySQL、Redis（部分用例） | 只通过 HTTP 断言，不直接调用业务代码 |
| L2 `services/` | `AuthService` / `AdminAuthService` / `AdminUserService` | 真实 MySQL + 内存 Fake 缓存 | 不启动服务端，验证业务编排与错误语义 |
| L3 `repositories/` | 账号 / Token / 黑名单仓储 | 真实 MySQL；Redis 可用时走真实 Redis | 验证 SQL、约束与缓存回写/降级 |
| L4 `infra/` | `AppConfig`、MySQL 连接池、Redis 连接池 | 真实依赖 | 验证配置加载、安全护栏、借还与并发 |

## 2. 环境准备

* **MySQL**：默认 `127.0.0.1:3306`，账号 `wgc`，库 `testdb`（连接参数在 `config-int.ini` 中直接修改；密码可用 `SDU_IT_DB_PASSWORD` 覆盖）。
* **Redis**：默认 `127.0.0.1:6379`，**db 15**（地址与 db 在 `config-int.ini` 中修改；密码可用 `SDU_IT_REDIS_PASSWORD` 覆盖，db 必须 ≥ 1）。
* **服务端端口**：`18080`，由 `config-int.ini` 的 `server.port` 指定（L1 用）。
* **表隔离**：所有表必须以 `it_` 开头；`it_account`、`it_admin_account`、`it_refresh_tokens`、`it_blacklist` 由夹具创建/清空。
* **安全护栏**（`it_env.cpp`，不满足直接失败且不执行 DDL/DML）：
  1. `account_table / admin_account_table / blacklist_table / refresh_token_table` 全部以 `it_` 开头；
  2. 数据库名不得为 `mysql / information_schema / performance_schema / sys`；
  3. Redis db 不得为 0；清理只用 `FLUSHDB`，禁止 `FLUSHALL`。
* **动态库**：Trae 环境自带旧版 `libstdc++`，运行前必须 `unset LD_LIBRARY_PATH`（脚本已内置）。

## 3. 用例清单

### 3.1 L1 接口层（72 个，全部通过）

L1 通过真实 `sdu_forum_server` 子进程发起 HTTP 请求，按「模块 → API → 成功/失败分支」组织：

| 模块 | 基准路径 | API 数 | 用例数 | 文件 |
| --- | --- | --- | --- | --- |
| 用户认证 | `/sdu_forum/api/v1/auth` | 5 | 26 | `api/test_it_api_user_auth.cpp` |
| 管理员认证 | `/admin/api/v1/auth` | 3 | 14 | `api/test_it_api_admin_auth.cpp` |
| 管理端用户 | `/admin/api/v1/users` | 4 | 21 | `api/test_it_api_admin_users.cpp` |
| 通用契约 | 全站横切 | — | 11 | `api/test_it_api_contract.cpp` |

#### 3.1.1 用户认证模块（26）

| API | 鉴权 | 验证内容 |
| --- | --- | --- |
| `POST /sign-up` | 公开 | 成功：`201` + `data.public_id`（24 位）+ 账号落库且密码迭代次数为生产默认值；失败：缺字段/非法 JSON → `400 INVALID_JSON`，空/超长/类型错误/重复 `user_id` → `400 INVALID_PARAM` |
| `POST /login` | 公开 | 成功：`200` 双 token、`token_type=Bearer`、两个过期时间且 refresh > access；失败：密码错误与账号不存在统一 `401 INVALID_CREDENTIALS`，全站/禁止登录维度封禁 `403 ACCOUNT_BANNED`，缺字段 `400 INVALID_JSON` |
| `POST /refresh-token` | Bearer **refresh** | 成功：`200` 新 access（不轮换时 refresh 为空，轮换时返回新 refresh），并校验签名与 `sub/role` claim；失败：旧 refresh 重放、access 当 refresh、过期/篡改 → `401`，缺 `refreshing` → `400 INVALID_PARAM`，Login 维度封禁 → `403` |
| `POST /logout` | Bearer **access** | 成功：`204` 空 body，refresh 落库、access 写入缓存；重复登出 → `401`（令牌已失效）；缺 Authorization/方案错误/过期/篡改 → `401` |
| `DELETE /logoff` | Bearer **access** | 成功：`204`，账号及关联记录删除，注销后无法再登录（`401 INVALID_CREDENTIALS`） |

Bearer 统一分支：缺失或非 Bearer → `401 UNAUTHORIZED`；签名错误/过期 → `401`（`invalid or expired access token`）；登出使用已吊销 refresh → `401`（`the refresh token is old`）。

#### 3.1.2 管理员认证模块（14）

| API | 鉴权 | 验证内容 |
| --- | --- | --- |
| `POST /login` | 公开 | 成功：`200` 双 token、`admin_id`、角色为管理员；失败：密码错误/管理员不存在 `401 INVALID_CREDENTIALS`，缺字段/`admin_id` 超长 `400` |
| `POST /refresh-token` | Bearer **admin refresh** | 成功：`200` 新 access（签名校验）；轮换返回新 refresh，旧 refresh 重放 → `401`（已吊销） |
| `POST /logout` | Bearer **admin access** | 成功：`204` 空 body；重复登出 → `401` |

双密钥/角色隔离（4 个方向）：用户 access → 管理端 `401`；管理员 access → 用户端 `401`；用户 refresh → 管理端刷新 `401`；管理员 refresh → 用户端刷新 `401`。另覆盖管理端缺失 Bearer 与过期管理员 access token。

#### 3.1.3 管理端用户模块（21）

| API | 鉴权 | 验证内容 |
| --- | --- | --- |
| `POST /ban` | Bearer admin | 成功：`204`，黑名单新增生效记录、历史新增一条；全站永久（`time_type=0xFFFF`）、登录维度 + 时长（`time_type=0,time=30`）、自定义原因（`5999`）均正确落地；失败：同维度重复封禁 `409 CONFLICT`，`account_id=0/缺失`、非法 `reason_code/ban_type/time_type/time`、非法 `source`、自动类原因（`4001`）未配 `auto/report` → `400` |
| `POST /unban` | Bearer admin | 成功：`204`，黑名单移除、历史 `status=0` 且记录 `unban_reason`；失败：未封禁/无可解除记录 `404 NOT_FOUND`，缺 `account_id` `400 INVALID_JSON` |
| `GET /history?account_id=` | Bearer admin | 成功：`200` 数组，按 `banned_at` 倒序，含 `status`（1 生效/0 解除）、`expire_at`、`reason` 等字段；不存在的账号返回空数组；失败：`account_id` 非数字或缺失 `400 INVALID_PARAM` |
| `GET /blacklist` | Bearer admin | 成功：`200` `{page,page_size,total,items}`，默认 20 条/页；支持 `reason_code`、`ban_type`、`status`、`from/to` 时间范围筛选及分页；条目含 14 个完整字段；未知参数被忽略；失败：`page=0`、`page_size=0/101`、分页非数字、非法枚举、仅给 `status` 缺少 `reason_code/ban_type` → `400 INVALID_PARAM` |

说明：当前接口未暴露 `source/keyword/account_id` 黑名单筛选与账号状态查询，测试按现有契约覆盖，不做越界断言。

#### 3.1.4 通用契约模块（11）

| 类别 | 验证内容 | 期望 |
| --- | --- | --- |
| 路由/方法 | 未知路由、未注册子路径、错误方法 | `404` / `404` / `405` |
| 请求解析 | 非法 JSON、缺少 `Content-Type`、空 body、多余字段、超大 body | `400 INVALID_JSON` / `400 INVALID_JSON` / `400 INVALID_JSON` / 忽略并正常处理 / `413` 或 `400`（服务端拒绝即可，且后续请求仍可用） |
| 成功信封 | 登录成功响应 | `data` 包裹、`Content-Type: application/json`、无 `error` |
| 错误信封 | 鉴权失败、数据库失败 | `{error:{code,message}}`，无 `data`；`500 DB_ERROR` 且不出现 stack/trace/源码路径 |

### 3.2 L2 服务层（31 个，全部通过）

| 套件 | 数量 | 覆盖要点 |
| --- | --- | --- |
| `test_it_auth_flow` | 10 | 登录签发/校验、同错误文案、封禁拒绝登录、注册与重复、登出吊销、刷新不轮换/轮换吊销、非法 refresh、刷新校验封禁、注销删除账号 |
| `test_it_admin_auth_flow` | 6 | 管理员登录角色、错误密码、刷新、轮换吊销、登出、非法 refresh |
| `test_it_admin_user_flow` | 15 | 永久/限时封禁、重复封禁冲突、全站下级维度约束、解封与不存在、过期兜底、黑名单筛选、自定义原因、参数校验、自动来源 |

### 3.3 L3 仓储层（18 个，全部通过）

| 套件 | 数量 | 覆盖要点 |
| --- | --- | --- |
| `test_it_account_repo` | 5 | 双唯一键查询、空结果、删除、重复 user_id / public_id 约束 |
| `test_it_token_repo` | 5 | refresh 落库与回写缓存、access 仅缓存、缓存不可用降级、重复约束 |
| `test_it_blacklist_repo` | 8 | 生效判定、过期兜底、按维度解封、All 解封、历史排序、条件检索分页、已解除不生效 |

### 3.4 L4 基础设施（14 个，全部通过）

| 套件 | 数量 | 覆盖要点 |
| --- | --- | --- |
| `test_it_config` | 5 | 配置加载、三项安全护栏拒绝路径 |
| `test_it_db_pool` | 5 | 查询/绑定/更新、并发借还、夹具建表 |
| `test_it_redis_pool` | 4 | PING、SET/GET/DEL、FLUSHDB、多次借还（Redis 缺失时整体跳过） |

## 4. 核心流程覆盖矩阵

计算口径：对 13 条核心流程 × 4 类分支（正常路径、鉴权/权限、参数/业务错误、依赖降级/边界）逐一列出**适用**单元，已覆盖 / 适用即为覆盖率。

| 流程 | 正常 | 鉴权/权限 | 参数/业务错误 | 依赖降级/边界 |
| --- | --- | --- | --- | --- |
| 用户注册 | ✔ | — | ✔ | — |
| 用户登录 | ✔ | — | ✔（401/403/400） | — |
| 用户刷新 | ✔ | ✔ | ✔ | ✔（Fake 缓存 + DB 降级） |
| 用户登出 | ✔ | ✔ | ✔ | ✔（真实 Redis + Fake） |
| 用户注销 | ✔ | ✔ | — | ✔（真实 Redis + Fake） |
| 管理员登录 | ✔ | — | ✔ | — |
| 管理员刷新 | ✔ | ✔ | ✔ | — |
| 管理员登出 | ✔ | ✔ | — | ✔ |
| 封禁 | ✔ | ✔ | ✔ | ✔（409/404） |
| 解封 | ✔ | ✔ | ✔ | ✔（404） |
| 封禁历史 | ✔ | ✔ | ✔ | — |
| 黑名单列表 | ✔ | ✔ | ✔ | — |
| 通用 HTTP 契约 | ✔ | ✔（401 信封） | ✔（400/404/405/500） | ✔（超大请求体） |

* 适用单元 **39**，已覆盖 **39**，核心流程分支覆盖率 **100%**。
* 另有 4 个跨密钥隔离方向（用户→管理端、管理端→用户端、用户 refresh→管理端、管理 refresh→用户端）全部覆盖。
* 行覆盖率（gcov）为可选项，可用 `scripts/coverage_summary.py` 汇总，不作为验收门槛。

## 5. 运行指南

```bash
cd /home/wgc/SDU-Forum/backend
unset LD_LIBRARY_PATH

# 1) 配置 + 编译
cmake -S . -B build -DSDU_BUILD_INTEGRATION_TESTS=ON
cmake --build build -j"$(nproc)"

# 2) 单元测试（无回归）
ctest --test-dir build -L unit --output-on-failure

# 3) 一键集成测试（建表 → ctest → HTML 报告）
bash tests/integration/scripts/run_integration.sh

# 4) 已编译时跳过构建
bash tests/integration/scripts/run_integration.sh --no-build

# 5) 稳定性复跑
bash tests/integration/scripts/run_integration.sh --repeat 2

# 6) 单套件 / 单用例
./build/tests/integration/test_it_api_admin_users
./build/tests/integration/test_it_api_admin_users -tc="api_admin_users: 黑名单默认分页返回生效封禁"
ctest --test-dir build -R "IT_api_" --output-on-failure
```

产物位置：

* JUnit：`build/reports/integration-junit.xml`
* HTML：`build/reports/integration-report.html`
* 服务端日志：`build/reports/it-server-<port>.log`

常见问题：

| 现象 | 处理 |
| --- | --- |
| 二进制报 `GLIBCXX_...` / 无法启动 | 先 `unset LD_LIBRARY_PATH` |
| 大量 DB 用例失败 | 检查 MySQL 账号与 `testdb` 权限；必要时修改 `config-int.ini` 的 `[database]` |
| Redis 用例被跳过 | Redis 不可用属预期；L2 用 Fake 缓存仍覆盖登出/刷新语义 |
| 端口被占用 | 修改 `config-int.ini` 的 `server.port`，勿与已运行服务端冲突 |
| L1 请求无响应 | 查看 `build/reports/it-server-18080.log`；确认服务端已启动且未崩溃 |
| 表不存在 | 执行 `bash tests/integration/scripts/setup_db.sh` 或使用一键脚本 |

## 6. 维护规范

* **命名**：`TEST_CASE("<模块>: <场景>")`，模块名与文件/接口保持一致（如 `api_admin_users`）。
* **造数**：统一使用 `it::seed_account / seed_admin / seed_ban`；每个用例开头调用 `it::load_config(); it::server(); it::reset_db();`（L1 先启动服务端再清库，避免 fork 时携带连接池）。
* **清理**：`it::reset_db()` 只清空 `it_*` 表；Redis 使用 `it::redis_flush_db()`（FLUSHDB）。
* **禁止**：连接非 `it_` 表、对其他库执行 DDL/DML、`FLUSHALL`、用例间依赖执行顺序、在断言中依赖同一秒内 JWT 字符串不同（JWT 无随机盐，同秒同载荷结果相同）。
* **新增接口用例**：在 `api/` 新建或复用套件 → 覆盖「成功 / 未认证 / 非法参数 / 业务冲突」四类分支 → 在本文第 3、4 节登记。
* **新增 Redis 依赖用例**：加 `* doctest::skip(!it::redis_available())`，并在 L2 用 `FakeCacheProvider` 提供等价覆盖。

## 7. 已知限制与后续项

1. `AdminUserService::get_user_status` 尚未暴露路由，L1 通过历史/黑名单接口间接验证状态。
2. 黑名单列表的 `total` 为当前筛选结果集大小（近似值），未做独立 `COUNT(*)`。
3. Redis 缺失时 8 个用例跳过（4 个 API 登出/注销 + 4 个 Redis 池用例）；服务端降级启动未做 L1 验证。
4. 刷新接口在不轮换模式下不返回 `public_id`，属当前接口契约；测试以 token 签名与 payload 断言替代。
5. 超大请求体用例以「服务端拒绝（413/400/连接提前关闭）」为通过条件，以兼容 Drogon 的不同拒绝时机。
