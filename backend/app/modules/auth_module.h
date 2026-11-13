#pragma once

#include <drogon/HttpAppFramework.h>

#include <memory>

#include "./module.h"
#include "./repositories/account_repository.h"
#include "./repositories/blacklist_repository.h"
#include "config/app_config.h"
#include "conn-pool/ConnectionPool.h"
#include "controllers/auth_controller.h"
#include "middleware/auth_middleware.h"
#include "redis-pool/RedisConnectionPool.h"
#include "repositories/token_repository.h"
#include "services/auth_service.h"

namespace app::modules
{
class AuthModule : public Module
{
public:
    AuthModule(const infra::AppConfig& config, dcp::ConnectionPool* db_pool,
               rcp::RedisConnectionPool* redis_pool);
    void register_routes(drogon::HttpAppFramework& app) override;
    void register_middlewares(drogon::HttpAppFramework& app) override;

private:
    repository::AccountRepository                      m_account_repository;
    repository::BlacklistRepository                    m_blacklist_repository;
    repository::TokenRepository                        m_token_repo;
    service::AuthService                               m_service;
    controller::AuthController                         m_controller;
    std::shared_ptr<middleware::AccessAuthMiddleware>  m_access_middleware;
    std::shared_ptr<middleware::RefreshAuthMiddleware> m_refresh_middleware;
};
}  // namespace app::modules