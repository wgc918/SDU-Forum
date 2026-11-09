#include "routes.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include "common/api_response.h"
#include "controllers/auth_controller.h"
#include "middleware/auth_middleware.h"

namespace routes
{

void register_auth_routes(drogon::HttpAppFramework& app, controller::AuthController& controller)
{
    app.registerHandler(
        "/sdu_forum/api/v1/auth/login",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.login(req, std::move(callback)); },
        {drogon::Post});
    app.registerHandler(
        "/sdu_forum/api/v1/auth/sign_up",
        [&controller](const drogon::HttpRequestPtr&                         req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { controller.signUp(req, std::move(callback)); },
        {drogon::Post});
    // 受保护路由示例：中间件通过类名挂载到具体路由，供后续业务路由复制
    app.registerHandler("/sdu_forum/api/v1/auth/me",
                        [](const drogon::HttpRequestPtr&                         req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                        {
                            Json::Value data;
                            data["public_id"] = middleware::current_public_id(req);
                            callback(common::data_response(200, data));
                        },
                        {drogon::Get, middleware::AuthMiddleware::classTypeName()});
}

}  // namespace routes
