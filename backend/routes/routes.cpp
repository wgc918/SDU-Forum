#include "routes.h"

#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <json/json.h>

#include <cstdint>
#include <string>

#include "controllers/admin/auth_controller.h"
#include "controllers/admin/user_controller.h"
#include "controllers/auth_controller.h"
#include "middleware/admin_auth_middleware.h"
#include "middleware/auth_middleware.h"
#include "middleware/role_middleware.h"

namespace routes
{

void register_auth_routes(drogon::HttpAppFramework& app, controller::AuthController& controller)
{
    const std::string base_url = "/sdu_forum/api/v1/auth";

    app.registerHandler(
        base_url + "/login",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.login(req, std::move(callback)); },
        {drogon::Post});

    app.registerHandler(
        base_url + "/sign-up",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.signUp(req, std::move(callback)); },
        {drogon::Post});

    app.registerHandler(
        base_url + "/logout",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.logout(req, std::move(callback)); },
        {drogon::Post, middleware::AccessAuthMiddleware::classTypeName(),
         middleware::UserRoleMiddleware::classTypeName()});

    app.registerHandler(
        base_url + "/refresh-token",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.refresh_token(req, std::move(callback)); },
        {drogon::Post, middleware::RefreshAuthMiddleware::classTypeName(),
         middleware::UserRoleMiddleware::classTypeName()});

    app.registerHandler(
        base_url + "/logoff",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.logoff(req, std::move(callback)); },
        {drogon::Post, middleware::AccessAuthMiddleware::classTypeName(),
         middleware::UserRoleMiddleware::classTypeName()});
}

void register_admin_auth_routes(drogon::HttpAppFramework&               app,
                                admin::controller::AdminAuthController& controller)
{
    const std::string base_url = "/admin/api/v1/auth";

    app.registerHandler(
        base_url + "/login",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.login(req, std::move(callback)); },
        {drogon::Post});

    app.registerHandler(
        base_url + "/logout",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.logout(req, std::move(callback)); },
        {drogon::Post, middleware::AdminAccessAuthMiddleware::classTypeName(),
         middleware::AdminRoleMiddle::classTypeName()});

    app.registerHandler(
        base_url + "/refresh-token",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.refresh_token(req, std::move(callback)); },
        {drogon::Post, middleware::AdminRefreshAuthMiddleware::classTypeName(),
         middleware::AdminRoleMiddle::classTypeName()});
}

void register_admin_user_routes(drogon::HttpAppFramework&               app,
                                admin::controller::AdminUserController& controller)
{
    const std::string base_url = "/admin/api/v1/users";

    app.registerHandler(
        base_url + "/{account_id}/ban",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const uint64_t                                        account_id)
        { controller.ban(req, std::move(callback), account_id); },
        {drogon::Post, middleware::AdminAccessAuthMiddleware::classTypeName(),
         middleware::AdminRoleMiddle::classTypeName()});

    app.registerHandler(
        base_url + "/{account_id}/unban",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const uint64_t                                        account_id)
        { controller.unban(req, std::move(callback), account_id); },
        {drogon::Patch, middleware::AdminAccessAuthMiddleware::classTypeName(),
         middleware::AdminRoleMiddle::classTypeName()});

    app.registerHandler(
        base_url + "/history",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const uint64_t                                        account_id)
        { controller.get_history(req, std::move(callback), account_id); },
        {drogon::Get, middleware::AdminAccessAuthMiddleware::classTypeName(),
         middleware::AdminRoleMiddle::classTypeName()});

    app.registerHandler(
        base_url + "/blacklist",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.get_blacklist(req, std::move(callback)); },
        {drogon::Get, middleware::AdminAccessAuthMiddleware::classTypeName(),
         middleware::AdminRoleMiddle::classTypeName()});
}
}  // namespace routes
