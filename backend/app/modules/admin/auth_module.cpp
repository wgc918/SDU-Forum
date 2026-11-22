#include "auth_module.h"

#include "config/app_config.h"
#include "routes/routes.h"

namespace app::modules
{

AdminAuthModule::AdminAuthModule(const infra::AppConfig&                 config,
                                 repository::PooledDbConnectionProvider& db_provider,
                                 repository::TokenRepository&            token_repo)
    : m_account_repository(db_provider, config.auth().admin_account_table),
      m_service(m_account_repository, token_repo, config.jwt()),
      m_controller(m_service)
{
}

void AdminAuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_admin_auth_routes(app, m_controller);
}

}  // namespace app::modules
