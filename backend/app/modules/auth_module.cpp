#include "./auth_module.h"

#include "config/app_config.h"
#include "routes/routes.h"
#include "services/admin/ban_service.h"

namespace app::modules
{

AuthModule::AuthModule(const infra::AppConfig&                 config,
                       repository::PooledDbConnectionProvider& db_provider,
                       repository::TokenRepository&            token_repo,
                       admin::service::BanService&             ban_service)
    : m_account_repository(db_provider, config.auth().account_table),
      m_service(m_account_repository, token_repo, ban_service, config.jwt()),
      m_controller(m_service)
{
}

void AuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_auth_routes(app, m_controller);
}

}  // namespace app::modules
