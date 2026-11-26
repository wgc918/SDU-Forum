//-----------------------------------------------------------------------------
// 文件: test_it_auth_flow.cpp
// 描述: L2 服务层 —— 用户鉴权编排（登录/注册/登出/刷新/注销）与真实 MySQL。
//-----------------------------------------------------------------------------
#include <string>

#include "auth/token_signer.h"
#include "common/ban_enums.h"
#include "common/errors.h"
#include "config/app_config.h"
#include "doctest.h"
#include "it_env.h"
#include "it_support.h"
#include "repositories/account_repository.h"
#include "repositories/blacklist_repository.h"
#include "repositories/token_repository.h"
#include "services/admin/ban_service.h"
#include "services/auth_service.h"
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
    return common::ErrorCode::InternalError;  // 未抛出
}

struct Fixture
{
    test_support::FakeCacheProvider    cache;
    repository::AccountRepository      account;
    repository::TokenRepository        token;
    repository::BlacklistRepository    blacklist;
    admin::service::BanService         ban;
    service::AuthService               auth;

    Fixture()
        : account(it::db(), infra::AppConfig::instance().table().account_table),
          token(it::db(), cache, it::token_store_config()),
          blacklist(it::db(), infra::AppConfig::instance().table().blacklist_table),
          ban(blacklist),
          auth(account, token, ban, infra::AppConfig::instance().jwt())
    {
    }
};

std::string hash_of(const Fixture& fixture, const std::string& token)
{
    return utils::hmac_sha256_base64url(fixture.token.hash_secret(), token);
}

}  // namespace

TEST_CASE("auth_flow: 登录成功并签发可校验的双 token")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    std::string       public_id;
    it::seed_account(user_id, "secret123", &public_id);

    const auto result = fx.auth.login(user_id, "secret123");
    CHECK(result.public_id == public_id);
    CHECK_FALSE(result.access_token.empty());
    CHECK_FALSE(result.refresh_token.empty());
    CHECK(result.access_token_exp > 0);
    CHECK(result.refresh_token_exp > result.access_token_exp);

    const auto& jwt = infra::AppConfig::instance().jwt();
    auto        access = infra::TokenSigner(jwt.access_secret, jwt.issuer)
                             .verify(result.access_token);
    REQUIRE(access.has_value());
    CHECK(access->sub == public_id);
    CHECK(access->role == infra::kUserRole);

    auto refresh = infra::TokenSigner(jwt.refresh_secret, jwt.issuer)
                       .verify(result.refresh_token);
    REQUIRE(refresh.has_value());
    CHECK(refresh->sub == public_id);
}

TEST_CASE("auth_flow: 错误密码与不存在账号返回同一错误")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret123");

    CHECK(static_cast<int>(error_code_of([&] { fx.auth.login(user_id, "wrong"); })) ==
          static_cast<int>(common::ErrorCode::InvalidCredentials));
    CHECK(static_cast<int>(error_code_of([&] { fx.auth.login("__it_missing__", "secret123"); })) ==
          static_cast<int>(common::ErrorCode::InvalidCredentials));
}

TEST_CASE("auth_flow: 全站封禁与登录封禁均拒绝登录")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string all_user = it::new_user_id();
    const uint64_t    all_id   = it::seed_account(all_user, "secret123");
    it::seed_ban(all_id, ban::BanType::All, ban::BanStatus::Active, ban::ReasonCode::SpamAd, "all",
                 it::local_timestamp(-60), it::local_timestamp(3600));
    CHECK(static_cast<int>(error_code_of([&] { fx.auth.login(all_user, "secret123"); })) ==
          static_cast<int>(common::ErrorCode::AccountBanned));

    const std::string login_user = it::new_user_id();
    const uint64_t    login_id   = it::seed_account(login_user, "secret123");
    it::seed_ban(login_id, ban::BanType::Login, ban::BanStatus::Active, ban::ReasonCode::SpamAd,
                 "login", it::local_timestamp(-60), it::local_timestamp(3600));
    CHECK(static_cast<int>(error_code_of([&] { fx.auth.login(login_user, "secret123"); })) ==
          static_cast<int>(common::ErrorCode::AccountBanned));
}

TEST_CASE("auth_flow: 注册成功且重复 user_id 被拒绝")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    const std::string public_id = fx.auth.signUp(user_id, "secret123");
    CHECK_FALSE(public_id.empty());
    CHECK(it::account_id_of(user_id) > 0);

    CHECK(static_cast<int>(error_code_of([&] { fx.auth.signUp(user_id, "another"); })) ==
          static_cast<int>(common::ErrorCode::InvalidParam));
}

TEST_CASE("auth_flow: 登出吊销 refresh 与 access token")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret123");
    const auto login = fx.auth.login(user_id, "secret123");

    fx.auth.logout(login.refresh_token, login.access_token);

    CHECK(fx.token.find_refresh_token(hash_of(fx, login.refresh_token)));
    CHECK(fx.cache.fake().contains(hash_of(fx, login.access_token)));
    CHECK(fx.cache.fake().ttl_of(hash_of(fx, login.access_token)) ==
          it::token_store_config().access_ttl_s);

    CHECK(static_cast<int>(
              error_code_of([&] { fx.auth.logout(login.refresh_token, login.access_token); })) ==
          static_cast<int>(common::ErrorCode::Unauthorized));
}

TEST_CASE("auth_flow: 刷新 access token 且不轮换 refresh")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret123");
    const auto login = fx.auth.login(user_id, "secret123");

    const auto refreshed =
        fx.auth.refresh_token(login.refresh_token, login.access_token, false);
    CHECK_FALSE(refreshed.access_token.empty());
    CHECK(infra::TokenSigner(infra::AppConfig::instance().jwt().access_secret,
                             infra::AppConfig::instance().jwt().issuer)
              .verify(refreshed.access_token)
              .has_value());
    CHECK(refreshed.refresh_token.empty());
    CHECK_FALSE(fx.token.find_refresh_token(hash_of(fx, login.refresh_token)));
}

TEST_CASE("auth_flow: 轮换 refresh token 后旧 token 被吊销")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret123");
    const auto login = fx.auth.login(user_id, "secret123");

    const auto refreshed =
        fx.auth.refresh_token(login.refresh_token, login.access_token, true);
    CHECK_FALSE(refreshed.refresh_token.empty());
    CHECK(infra::TokenSigner(infra::AppConfig::instance().jwt().refresh_secret,
                             infra::AppConfig::instance().jwt().issuer)
              .verify(refreshed.refresh_token)
              .has_value());
    CHECK(refreshed.public_id == login.public_id);
    CHECK(fx.token.find_refresh_token(hash_of(fx, login.refresh_token)));
}

TEST_CASE("auth_flow: 非法 refresh token 被拒绝")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    it::seed_account(user_id, "secret123");
    const auto login = fx.auth.login(user_id, "secret123");

    CHECK(static_cast<int>(error_code_of(
              [&] { fx.auth.refresh_token("not.a.valid.token", login.access_token, false); })) ==
          static_cast<int>(common::ErrorCode::Unauthorized));
}

TEST_CASE("auth_flow: 刷新时校验登录封禁")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    const uint64_t    id      = it::seed_account(user_id, "secret123");
    const auto        login   = fx.auth.login(user_id, "secret123");

    it::seed_ban(id, ban::BanType::Login, ban::BanStatus::Active, ban::ReasonCode::SpamAd, "login",
                 it::local_timestamp(-60), it::local_timestamp(3600));

    CHECK(static_cast<int>(error_code_of([&] {
              fx.auth.refresh_token(login.refresh_token, login.access_token, false);
          })) == static_cast<int>(common::ErrorCode::AccountBanned));
}

TEST_CASE("auth_flow: 注销删除账号")
{
    it::load_config();
    it::reset_db();
    Fixture fx;

    const std::string user_id = it::new_user_id();
    std::string       public_id;
    it::seed_account(user_id, "secret123", &public_id);
    const auto login = fx.auth.login(user_id, "secret123");

    fx.auth.logoff(public_id, login.refresh_token, login.access_token);

    CHECK_FALSE(fx.account.find_account_by_public_id(public_id).has_value());
    CHECK_FALSE(fx.account.find_account_by_user_id(user_id).has_value());
}
