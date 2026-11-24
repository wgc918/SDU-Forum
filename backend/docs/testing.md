# 测试体系

> 适用代码：`backend/tests/`  
> 最后核对：2026-11-24（详细用例设计见 [tests/docs/tests.md](../tests/docs/tests.md)）

## 1. 框架与运行

| 项 | 说明 |
| --- | --- |
| 测试框架 | doctest 2.4.11（`tests/third_party/doctest.h`） |
| 构建 | CMake，根 `CMakeLists.txt` 通过 `add_subdirectory(tests)` 接入 |
| 运行器 | CTest |
| 标签 | `fast;unit` |
| 入口 | `tests/support/test_main.cpp` |

```bash
# 构建
cmake -S backend -B backend/build
cmake --build backend/build -j

# 全部测试
ctest --test-dir backend/build --output-on-failure

# 只跑单元测试
ctest --test-dir backend/build -L unit --output-on-failure

# 单独运行某个可执行文件
./backend/build/tests/test_auth_service
```

> 测试必须能在**无 MySQL、无 Redis、无网络**的环境下通过。任何需要真实依赖的测试都不属于本套件。

## 2. 测试目标

| 可执行文件 | 覆盖对象 | 说明 |
| --- | --- | --- |
| `test_token_repository` | `repository::TokenRepository` | Redis/DB 降级、吊销查询、写入返回值 |
| `test_account_repository` | `repository::AccountRepository` | 查询/插入/删除、SQL 异常转换 |
| `test_admin_account_repository` | `repository::AdminAccountRepository` | 管理员查询/插入 |
| `test_utils` | `utils::*` | Bearer 解析、trim 等纯函数 |
| `test_auth_service` | `service::AuthService` | 登录、注册、登出、刷新、注销；CTest 超时 60s |

CTest 名称与目标同名；`AuthServiceUnit` 单独设置 60 秒超时（PBKDF2 150000 次迭代较慢）。

## 3. 测试替身（`tests/support/test_doubles.h`）

项目约定五类替身：

| 类型 | 用途 | 断言对象 |
| --- | --- | --- |
| Dummy | 仅占位，不参与断言 | 无 |
| Stub | 返回固定值 | 被测对象返回值 |
| Spy | 记录调用参数与顺序 | 交互是否发生 |
| Mock | 预置期望，`verify()` 校验契约 | 交互契约 |
| Fake | 轻量真实实现（内存缓存） | 端到端状态 |

现有实现：

- `StubResultSet`：按列名返回预设的 `uint64/int/string`；支持固定行数。
- `StubDbConnection`：共享 `StubDbState`，可配置查询/更新/绑定是否抛 `SQLException`、是否返回空结果集。
- `StubDbConnectionProvider`：每次 `acquire()` 返回新句柄但共享同一状态，模拟连接池语义。
- Fake Cache / Spy / Mock TokenRepository：用于验证 Redis 命中、降级、写入参数。
- 所有 Provider 采用「原型 + 共享 State」模式：句柄可复制，状态唯一。

新增替身时必须遵守：

1. 不得访问真实网络/数据库。
2. 状态必须可注入、可断言；不要用全局静态变量。
3. Mock 的期望在用例结束必须 `verify()`，否则视为漏测。

## 4. 用例约定

来自 [tests/docs/tests.md](../tests/docs/tests.md)：

- 命名：`TEST_CASE("被测类/方法 - 场景")`，场景描述预期行为。
- 一个用例只验证一个行为；成功与失败分开。
- 异常断言：

```cpp
CHECK_THROWS_AS(service.login(...), common::AppException);
try { service.login(...); }
catch (const common::AppException& e) {
    CHECK(e.code() == common::ErrorCode::InvalidCredentials);
}
```

- 不只断言「抛了异常」，必须断言 `ErrorCode`。
- 数据库异常用 `StubDbConnection` 的 `throw_on_query/update/prepare/set_*` 触发。
- 时间相关逻辑不做真实等待；通过构造输入或固定返回值验证。
- 不依赖用例执行顺序；每个用例独立构造依赖。

## 5. 当前覆盖情况

| 模块 | 覆盖 | 备注 |
| --- | --- | --- |
| `TokenRepository` | ✅ 34 个用例 | Redis 命中/未命中/异常降级、DB 回写、写入失败 |
| `AccountRepository` | ✅ 21 个用例 | 查询命中/未命中、插入、删除、SQL 异常 |
| `AdminAccountRepository` | ✅ 15 个用例 | 查询命中/未命中、插入、SQL 异常 |
| `AuthService` | ✅ 34 个用例 | 登录/注册/登出/刷新/注销主路径与错误路径 |
| `utils` | ✅ 1 个用例 | Bearer 解析 |
| `BlacklistRepository` | ❌ 未覆盖 | 写路径、生效判定、unban、过期兜底均无测试 |
| `AdminUserService` | ❌ 未覆盖 | 幂等、时长计算、来源一致性无测试 |
| 中间件 | ❌ 未覆盖 | 缺 token、吊销、封禁、角色不匹配均无测试 |
| `AppConfig` | ❌ 未覆盖 | 环境变量展开、validate 无测试 |
| HTTP 端到端 | ❌ 未覆盖 | 无 Drogon 集成测试 |

> `tests/docs/tests.md` 中部分描述已过时（例如仍写 `BlacklistRepository` 持有 `ConnectionPool`、登录封禁被注释）。**以当前代码与本文档为准**，修改测试时同步更新该文件。

## 6. 待补测试（按优先级）

1. **P0 `BlacklistRepository`**
   - `insert` 成功后字段映射；
   - `exists_active` 只匹配同维度；
   - `unban(All)` 与 `unban(type)` 的 SQL 参数；
   - `expire_outdated` 的返回值与异常分支；
   - `search` 的 `account_id` 绑定缺陷（修复后加回归测试）。
2. **P0 `AdminUserService`**
   - 已有 `All` / 同维度 → `Conflict`；
   - `time_type=0xFFFF` → 永久时间；
   - `time_type>2`、`time=0` → `InvalidParam`；
   - 4xxx + `manual` → `InvalidParam`；
   - `Unspecified` 无 reason → `InvalidParam`；
   - `unban` 无生效记录 → `NotFound`。
3. **P1 中间件**
   - 缺 `Authorization`、非 Bearer、空 token；
   - 已吊销 access/refresh；
   - 验签失败、过期；
   - `All` 封禁 → 403；
   - 角色不匹配 → 403；
   - **回归测试：已写响应后不得继续调用 `next`**（R1/R1b）。
4. **P1 `AppConfig`**
   - `${VAR}` / `${VAR:-default}`；
   - 缺必填项启动失败；
   - admin 密钥为空的行为（修复后加断言）。
5. **P2 HTTP 集成**
   - 需要 Drogon 测试应用与内存替身；当前无基础设施，暂缓。

## 7. 新增测试 checklist

- [ ] 测试文件放在 `tests/unit/<层>/test_xxx.cpp`
- [ ] 在 `tests/CMakeLists.txt` 新增目标并 `add_test`
- [ ] 设置标签 `fast;unit`
- [ ] 使用 `tests/support/test_doubles.h`，不访问真实依赖
- [ ] 成功路径 + 失败路径 + 边界值
- [ ] 异常断言校验 `ErrorCode`
- [ ] Mock 用例结束调用 `verify()`
- [ ] 更新本文档 §5 覆盖表
- [ ] 若测试暴露缺陷，在 [README.md](./README.md) §4 登记
