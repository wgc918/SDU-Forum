//-----------------------------------------------------------------------------
// 文件: test_it_admin_auth_flow.cpp
// 描述: L2 服务层 —— 管理端鉴权编排与真实 MySQL。
//-----------------------------------------------------------------------------
#include <string>

#include "auth/token_signer.h"
#include "common/errors.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"
#include "repositories/admin_account_repository.h"
#include "repositories/token_repository.h"
#include "services/admin/auth_service.h"
#include "support/test_doubles.h"
#include "utils/utils.h"

namespace
{

template <typename Callable>
common::ErrorCode error_code_of(Callable&& callable)
{
    try
    {
        callable();
    }
    catch (const common::AppException& error)
    {
        return error.code();
    }
    return common::ErrorCode::InternalError;
}

struct Fixture
{
    test_support::FakeCacheProvider    cache;
    repository::AdminAccountRepository admin_account;
    repository::TokenRepository        token;
    admin::service::AdminAuthService   auth;

    Fixture()
        : admin_account(it::db(), infra::AppConfig::instance().table().admin_account_table),
          token(it::db(), cache, it::token_store_config()),
          auth(admin_account, token, infra::AppConfig::instance().jwt())
    {
    }
};

std::string hash_of(const Fixture& fixture, const std::string& token)
{
    return utils::hmac_sha256_base64url(fixture.token.hash_secret(), token);
}

}  // namespace

TEST_CASE("admin_auth_flow: 登录成功并签发 admin 角色 token")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "adpass12");

    const auto result = fx.auth.login(admin_id, "adpass12");
    CHECK(result.admin_id == admin_id);
    CHECK_FALSE(result.access_token.empty());
    CHECK_FALSE(result.refresh_token.empty());

    const auto& jwt = infra::AppConfig::instance().jwt();
    auto        access =
        infra::TokenSigner(jwt.admin_access_secret, jwt.issuer).verify(result.access_token);
    REQUIRE(access.has_value());
    CHECK(access->sub == admin_id);
    CHECK(access->role == infra::kAdminRole);

    // 域隔离：admin token 不能通过用户密钥校验
    CHECK_FALSE(
        infra::TokenSigner(jwt.access_secret, jwt.issuer).verify(result.access_token).has_value());
}

TEST_CASE("admin_auth_flow: 错误密码与不存在管理员")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "adpass12");

    CHECK(static_cast<int>(error_code_of([&] { fx.auth.login(admin_id, "badpass1"); })) ==
          static_cast<int>(common::ErrorCode::InvalidCredentials));
    CHECK(static_cast<int>(error_code_of([&] { fx.auth.login("nobody", "adpass12"); })) ==
          static_cast<int>(common::ErrorCode::InvalidCredentials));
}

TEST_CASE("admin_auth_flow: 刷新 access token（不轮换 refresh）")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "adpass12");
    const auto login = fx.auth.login(admin_id, "adpass12");

    const auto refreshed =
        fx.auth.refresh_token(login.refresh_token, login.access_token, false);
    CHECK_FALSE(refreshed.access_token.empty());
    CHECK(infra::TokenSigner(infra::AppConfig::instance().jwt().admin_access_secret,
                             infra::AppConfig::instance().jwt().issuer)
              .verify(refreshed.access_token)
              .has_value());
    CHECK(refreshed.refresh_token.empty());
    CHECK_FALSE(fx.token.find_refresh_token(hash_of(fx, login.refresh_token)));
}

TEST_CASE("admin_auth_flow: 轮换 refresh token 后旧 token 被吊销")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "adpass12");
    const auto login = fx.auth.login(admin_id, "adpass12");

    const auto refreshed = fx.auth.refresh_token(login.refresh_token, login.access_token, true);
    CHECK_FALSE(refreshed.refresh_token.empty());
    CHECK(refreshed.admin_id == admin_id);
    CHECK(fx.token.find_refresh_token(hash_of(fx, login.refresh_token)));
}

TEST_CASE("admin_auth_flow: 登出吊销 token 且不可重复登出")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "adpass12");
    const auto login = fx.auth.login(admin_id, "adpass12");

    fx.auth.logout(login.refresh_token, login.access_token);
    CHECK(fx.token.find_refresh_token(hash_of(fx, login.refresh_token)));
    CHECK(fx.cache.fake().contains(hash_of(fx, login.access_token)));

    CHECK(static_cast<int>(
              error_code_of([&] { fx.auth.logout(login.refresh_token, login.access_token); })) ==
          static_cast<int>(common::ErrorCode::Unauthorized));
}

TEST_CASE("admin_auth_flow: 非法 refresh token 被拒绝")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string admin_id = it::new_admin_id();
    it::seed_admin(admin_id, "adpass12");
    const auto login = fx.auth.login(admin_id, "adpass12");

    CHECK(static_cast<int>(error_code_of(
              [&] { fx.auth.refresh_token("not.a.valid.token", login.access_token, false); })) ==
          static_cast<int>(common::ErrorCode::Unauthorized));
}
