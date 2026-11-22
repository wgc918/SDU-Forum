#pragma once

namespace drogon
{
class HttpAppFramework;
}

namespace app
{
/// @brief 业务模块接口：仅负责注册模块自身路由
class Module
{
public:
    virtual ~Module() = default;

    /// @brief 注册模块路由
    /// @param app Drogon 应用框架
    virtual void register_routes(drogon::HttpAppFramework& app) = 0;
};
}  // namespace app