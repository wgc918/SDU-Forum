#include "./auth_module.h"

#include <memory>

#include "config/app_config.h"
#include "repositories/account_repository.h"
#include "repositories/blacklist_repository.h"
#include "routes/routes.h"

namespace app::modules
{

AuthModule::AuthModule(const infra::JwtConfig& config, repository::AccountRepository& user_repo,
                       repository::BlacklistRepository& blacklist_repo)
    : m_service(user_repo, blacklist_repo, config),
      m_controller(m_service),
      m_middleware(
          std::make_shared<middleware::AuthMiddleware>(config.access_secret, config.issuer))
{
}

void AuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_auth_routes(app, m_controller);
}

void AuthModule::register_middlewares(drogon::HttpAppFramework& app)
{
    app.registerMiddleware(m_middleware);
}

}  // namespace app::modules