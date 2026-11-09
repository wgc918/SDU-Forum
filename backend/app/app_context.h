#pragma once

#include <memory>

#include "config/app_config.h"
#include "modules/auth_module.h"
#include "repositories/account_repository.h"
#include "repositories/blacklist_repository.h"
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

    /// 注册业务路由
    void register_routes();

    /// 注册中间件
    void register_middlewares();

private:
    const infra::AppConfig& m_config;
    dcp::ConnectionPool*    m_db_pool;

    repository::AccountRepository        m_account_repository;
    repository::BlacklistRepository      m_blacklist_repository;
    std::shared_ptr<modules::AuthModule> m_auth_module;
};

}  // namespace app
