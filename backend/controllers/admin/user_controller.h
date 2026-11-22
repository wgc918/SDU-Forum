#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>

namespace admin::service
{
class AdminUserService;
}

namespace admin::controller
{

class AdminUserController
{
public:
    AdminUserController(admin::service::AdminUserService& admin_user_service);

    void ban(const drogon::HttpRequestPtr&                         req,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void unban(const drogon::HttpRequestPtr&                         req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void get_status(const drogon::HttpRequestPtr&                         req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void get_history(const drogon::HttpRequestPtr&                         req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void get_blacklist(const drogon::HttpRequestPtr&                         req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    admin::service::AdminUserService& m_service;
};
}  // namespace admin::controller