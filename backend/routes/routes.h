#pragma once

namespace drogon
{
class HttpAppFramework;
}

namespace controller
{
class AuthController;
}

namespace admin::controller
{
class AdminAuthController;
}

namespace routes
{

/// 注册认证相关路由：POST /sdu_forum/api/v1/auth/login
void register_auth_routes(drogon::HttpAppFramework& app, controller::AuthController& controller);

void register_admin_auth_routes(drogon::HttpAppFramework&               app,
                                admin::controller::AdminAuthController& controller);
}  // namespace routes
