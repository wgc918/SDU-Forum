#pragma once

#include <drogon/HttpAppFramework.h>

#include "./module.h"
#include "./repositories/blacklist_repository.h"
#include "./repositories/user_repository.h"
#include "config/app_config.h"
#include "controllers/auth_controller.h"
#include "middleware/auth_middleware.h"
#include "services/auth_service.h"

namespace app::modules
{
class AuthModule : public Module
{
public:
    AuthModule(const infra::JwtConfig& config, repository::UserRepository& user_repo,
               repository::BlacklistRepository& blacklist_repo);
    void register_routes(drogon::HttpAppFramework& app) override;
    void register_middlewares(drogon::HttpAppFramework& app) override;

private:
    service::AuthService       m_service;
    controller::AuthController m_controller;

    std::shared_ptr<middleware::AuthMiddleware> m_middleware;
};
}  // namespace app::modules