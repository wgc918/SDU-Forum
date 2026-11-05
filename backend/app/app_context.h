#pragma once

#include <memory>

#include "config/app_config.h"
#include "controllers/auth_controller.h"
#include "repositories/blacklist_repository.h"
#include "repositories/user_repository.h"
#include "services/auth_service.h"

namespace dcp
{
class ConnectionPool;
}

namespace middleware
{
class AuthMiddleware;
}

namespace app
{

/// 依赖装配容器：持有连接池与各层对象，并按依赖顺序初始化
class AppContext
{
public:
    explicit AppContext(const infra::AppConfig& config);
    ~AppContext();

    AppContext(const AppContext&)            = delete;
    AppContext& operator=(const AppContext&) = delete;

    /// 初始化数据库连接池等基础设施
    void init_infrastructure();

    /// 注册异常处理中间件与业务路由
    void register_routes();

private:
    const infra::AppConfig& m_config;
    dcp::ConnectionPool*    m_db_pool;

    repository::UserRepository      m_user_repository;
    repository::BlacklistRepository m_blacklist_repository;
    service::AuthService            m_auth_service; 
    controller::AuthController      m_auth_controller;
    std::shared_ptr<middleware::AuthMiddleware> m_auth_middleware;
};

}  // namespace app
