#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

#include <functional>
#include <string>

#include "auth/token_signer.h"
#include "config/app_config.h"
#include "repositories/token_repository.h"

namespace middleware
{

inline constexpr char kRoleAttribute[] = "role";

class RoleMiddleware
{
public:
    RoleMiddleware(const std::string& role);
    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb);

private:
    std::string m_role;
};

class UserRoleMiddleware : public RoleMiddleware,
                           public drogon::HttpMiddleware<UserRoleMiddleware, false>
{
public:
    UserRoleMiddleware();
    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

class AdminRoleMiddle : public RoleMiddleware, public drogon::HttpMiddleware<AdminRoleMiddle, false>
{
public:
    AdminRoleMiddle();
    void invoke(const drogon::HttpRequestPtr& req, drogon::MiddlewareNextCallback&& next_cb,
                drogon::MiddlewareCallback&& mcb) override;
};

std::string current_role(const drogon::HttpRequestPtr& req);
}  // namespace middleware