#include "./auth_module.h"

#include "config/app_config.h"
#include "routes/routes.h"

namespace app::modules
{

AuthModule::AuthModule(const infra::AppConfig&                 config,
                       repository::PooledDbConnectionProvider& db_provider,
                       repository::TokenRepository&            token_repo,
                       repository::BlacklistRepository&        blacklist_repo)
    : m_account_repository(db_provider, config.auth().account_table),
      m_service(m_account_repository, blacklist_repo, token_repo, config.jwt()),
      m_controller(m_service)
{
}

void AuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_auth_routes(app, m_controller);
}

}  // namespace app::modules
