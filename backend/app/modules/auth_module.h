#pragma once

#include <drogon/HttpAppFramework.h>

#include "./module.h"
#include "./repositories/account_repository.h"
#include "config/app_config.h"
#include "controllers/auth_controller.h"
#include "repositories/token_repository.h"
#include "services/admin/ban_service.h"
#include "services/auth_service.h"

namespace app::modules
{
/// @brief 用户认证模块：登录、注册、登出与令牌刷新
class AuthModule : public Module
{
public:
    /// @brief 构造用户认证模块
    /// @param config 应用配置
    /// @param db_provider 共享数据库连接提供者
    /// @param token_repo 共享令牌仓库
    /// @param blacklist_repo 共享黑名单仓库
    AuthModule(const infra::AppConfig& config, repository::TokenRepository& token_repo,
               repository::AccountRepository& account_repo,
               admin::service::BanService&    ban_service);
    void register_routes(drogon::HttpAppFramework& app) override;

private:
    repository::AccountRepository& m_account_repository;
    service::AuthService           m_service;
    controller::AuthController     m_controller;
};
}  // namespace app::modules
