#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>

namespace service
{
class AuthService;
}

namespace manager::controller
{

/// HTTP 适配层：解析请求、参数校验、调用 Service、返回统一响应
class AuthController
{
public:
    explicit AuthController(service::AuthService& auth_service);

    void login(const drogon::HttpRequestPtr&                         req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void logout(const drogon::HttpRequestPtr&                         req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void refresh_token(const drogon::HttpRequestPtr&                         req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    service::AuthService& m_auth_service;
};
}  // namespace manager::controller