#include "./auth_module.h"

#include "config/app_config.h"
#include "repositories/account_repository.h"
#include "routes/routes.h"
#include "services/admin/ban_service.h"

namespace app::modules
{

AuthModule::AuthModule(const infra::AppConfig& config, repository::TokenRepository& token_repo,
                       repository::AccountRepository& account_repo,
                       admin::service::BanService&    ban_service)
    : m_account_repository(account_repo),
      m_service(m_account_repository, token_repo, ban_service, config.jwt()),
      m_controller(m_service)
{
}

void AuthModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_auth_routes(app, m_controller);
}

}  // namespace app::modules
