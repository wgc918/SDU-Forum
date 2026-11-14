#include "./auth_module.h"

#include <memory>

#include "config/app_config.h"
#include "middleware/auth_middleware.h"
#include "repositories/account_repository.h"
#include "repositories/blacklist_repository.h"
#include "repositories/token_repository.h"
#include "routes/routes.h"

namespace app::modules
{

AuthModule::AuthModule(const infra::AppConfig& config, dcp::ConnectionPool* db_pool,
                       rcp::RedisConnectionPool* redis_pool)
    : m_account_repository(*db_pool, config.auth().account_table),
      m_blacklist_repository(*db_pool, config.auth().blacklist_table),
      m_token_repo(
          *db_pool, *redis_pool,
          {config.auth().refresh_token_table, config.jwt().hash_secret,
           config.jwt().refresh_token_ttl_seconds(), config.jwt().access_token_ttl_seconds()}),
      m_service(m_account_repository, m_blacklist_repository, m_token_repo, config.jwt(),
                config.auth()),
      m_controller(m_service),
      m_access_middleware(std::make_shared<middleware::AccessAuthMiddleware>(config.jwt())),
      m_refresh_middleware(std::make_shared<middleware::RefreshAuthMiddleware>(config.jwt()))
{
}

void AuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_auth_routes(app, m_controller);
}

void AuthModule::register_middlewares(drogon::HttpAppFramework& app)
{
    app.registerMiddleware(m_access_middleware);
    app.registerMiddleware(m_refresh_middleware);
}

}  // namespace app::modules