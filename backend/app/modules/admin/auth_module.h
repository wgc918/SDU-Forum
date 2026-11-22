#pragma once

#include <drogon/HttpAppFramework.h>

#include "../module.h"
#include "config/app_config.h"
#include "controllers/admin/auth_controller.h"
#include "repositories/admin_account_repository.h"
#include "repositories/connection_provider.h"
#include "repositories/token_repository.h"
#include "services/admin/auth_service.h"

namespace app::modules
{
/// @brief 管理员认证模块：登录、登出与令牌刷新
class AdminAuthModule : public Module
{
public:
    /// @brief 构造管理员认证模块
    /// @param config 应用配置
    /// @param db_provider 共享数据库连接提供者
    /// @param token_repo 共享令牌仓库
    AdminAuthModule(const infra::AppConfig& config,
                    repository::PooledDbConnectionProvider& db_provider,
                    repository::TokenRepository& token_repo);
    void register_routes(drogon::HttpAppFramework& app) override;

private:
    repository::AdminAccountRepository    m_account_repository;
    admin::service::AdminAuthService      m_service;
    admin::controller::AdminAuthController m_controller;
};
}  // namespace app::modules
