#include "auth_module.h"

#include <memory>

#include "config/app_config.h"
#include "middleware/admin_auth_middleware.h"
#include "middleware/role_middleware.h"
#include "repositories/admin_account_repository.h"
#include "repositories/token_repository.h"
#include "routes/routes.h"

namespace app::modules
{

AdminAuthModule::AdminAuthModule(const infra::AppConfig& config, dcp::ConnectionPool* db_pool,
                                 rcp::RedisConnectionPool* redis_pool)
    : m_db_provider(*db_pool),
      m_cache_provider(*redis_pool),
      m_account_repository(m_db_provider, config.auth().admin_account_table),
      m_token_repo(
          m_db_provider, m_cache_provider,
          {config.auth().refresh_token_table, config.jwt().hash_secret,
           config.jwt().refresh_token_ttl_seconds(), config.jwt().access_token_ttl_seconds()}),
      m_service(m_account_repository, m_token_repo, config.jwt()),
      m_controller(m_service),
      m_access_middleware(
          std::make_shared<middleware::AdminAccessAuthMiddleware>(config.jwt(), m_token_repo)),
      m_refresh_middleware(
          std::make_shared<middleware::AdminRefreshAuthMiddleware>(config.jwt(), m_token_repo)),
      m_role_middleware(std::make_shared<middleware::AdminRoleMiddle>())
{
}

void AdminAuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_admin_auth_routes(app, m_controller);
}

void AdminAuthModule::register_middlewares(drogon::HttpAppFramework& app)
{
    app.registerMiddleware(m_access_middleware);
    app.registerMiddleware(m_refresh_middleware);
    app.registerMiddleware(m_role_middleware);
}

}  // namespace app::modules