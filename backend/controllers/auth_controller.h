#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>

namespace service
{
class AuthService;
}

namespace controller
{

/// HTTP 适配层：解析请求、参数校验、调用 Service、返回统一响应
class AuthController
{
public:
    explicit AuthController(service::AuthService& auth_service);

    void login(const drogon::HttpRequestPtr&                         req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    service::AuthService& m_auth_service;
};

}  // namespace controller
