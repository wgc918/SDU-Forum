#include "role_middleware.h"

#include "auth/token_signer.h"
#include "common/api_response.h"
#include "common/errors.h"
#include "log.h"
#include "middleware/auth_middleware.h"

namespace middleware
{
RoleMiddleware::RoleMiddleware(const std::string& role) : m_role(role)
{
}

void RoleMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                            drogon::MiddlewareNextCallback&& next_cb,
                            drogon::MiddlewareCallback&&     mcb)
{
    std::string cur = current_role(req);
    if (cur.empty())
    {
        LOGW("current role is empty");
        mcb(common::error_response(common::ErrorCode::Forbidden, "current role is empty"));
    }

    if (cur != m_role)
    {
        LOGW("current role is not matched");
        mcb(common::error_response(common::ErrorCode::Forbidden, "current role is not matched"));
    }

    next_cb(std::move(mcb));
}

UserRoleMiddleware::UserRoleMiddleware() : RoleMiddleware(infra::kUserRole)
{
}

void UserRoleMiddleware::invoke(const drogon::HttpRequestPtr&    req,
                                drogon::MiddlewareNextCallback&& next_cb,
                                drogon::MiddlewareCallback&&     mcb)
{
    RoleMiddleware::invoke(req, std::move(next_cb), std::move(mcb));
}

AdminRoleMiddle::AdminRoleMiddle() : RoleMiddleware(infra::kAdminRole)
{
}

void AdminRoleMiddle::invoke(const drogon::HttpRequestPtr&    req,
                             drogon::MiddlewareNextCallback&& next_cb,
                             drogon::MiddlewareCallback&&     mcb)
{
    RoleMiddleware::invoke(req, std::move(next_cb), std::move(mcb));
}
}  // namespace middleware