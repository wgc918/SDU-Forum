#include "user_module.h"

#include "routes/routes.h"

namespace app::modules
{
AdminUserModule::AdminUserModule(repository::BlacklistRepository& blacklist_repo)
    : m_service(blacklist_repo), m_controller(m_service)
{
}

void AdminUserModule::register_routes(drogon::HttpAppFramework& app)
{
    routes::register_admin_user_routes(app, m_controller);
}

}  // namespace app::modules
