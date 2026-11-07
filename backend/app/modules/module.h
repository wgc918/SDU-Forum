#pragma once

namespace drogon
{
class HttpAppFramework;
}

namespace app
{
class Module
{
public:
    virtual ~Module() = default;

    virtual void register_routes(drogon::HttpAppFramework& app) = 0;

    virtual void register_middlewares(drogon::HttpAppFramework& app) {};
};
}  // namespace app