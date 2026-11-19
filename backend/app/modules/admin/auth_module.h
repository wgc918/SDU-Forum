#pragma once

#include <drogon/HttpAppFramework.h>

#include <memory>

#include "../module.h"
#include "config/app_config.h"
#include "conn-pool/ConnectionPool.h"
#include "controllers/admin/auth_controller.h"
#include "middleware/admin_auth_middleware.h"
#include "middleware/role_middleware.h"
#include "redis-pool/RedisConnectionPool.h"
#include "repositories/admin_account_repository.h"
#include "repositories/token_repository.h"
#include "services/admin/auth_service.h"

namespace app::modules
{
class AdminAuthModule : public Module
{
public:
    AdminAuthModule(const infra::AppConfig& config, dcp::ConnectionPool* db_pool,
                    rcp::RedisConnectionPool* redis_pool);
    void register_routes(drogon::HttpAppFramework& app) override;
    void register_middlewares(drogon::HttpAppFramework& app) override;

private:
    repository::PooledDbConnectionProvider                  m_db_provider;
    repository::PooledCacheConnectionProvider               m_cache_provider;
    repository::AdminAccountRepository                      m_account_repository;
    repository::TokenRepository                             m_token_repo;
    admin::service::AdminAuthService                        m_service;
    admin::controller::AdminAuthController                  m_controller;
    std::shared_ptr<middleware::AdminAccessAuthMiddleware>  m_access_middleware;
    std::shared_ptr<middleware::AdminRefreshAuthMiddleware> m_refresh_middleware;
    std::shared_ptr<middleware::AdminRoleMiddle>            m_role_middleware;
};
}  // namespace app::modules