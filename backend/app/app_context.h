#pragma once

#include <memory>

#include "app/modules/admin/auth_module.h"
#include "app/modules/admin/user_module.h"
#include "config/app_config.h"
#include "modules/auth_module.h"
#include "redis-pool/RedisConnectionPool.h"
#include "repositories/account_repository.h"
#include "repositories/blacklist_repository.h"
#include "repositories/connection_provider.h"
#include "repositories/token_repository.h"
#include "services/admin/ban_service.h"

namespace dcp
{
class ConnectionPool;
}

namespace middleware
{
class AccessAuthMiddleware;
class RefreshAuthMiddleware;
class UserRoleMiddleware;
class AdminAccessAuthMiddleware;
class AdminRefreshAuthMiddleware;
class AdminRoleMiddle;
}  // namespace middleware

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
    const infra::AppConfig&   m_config;
    dcp::ConnectionPool*      m_db_pool;
    rcp::RedisConnectionPool* m_redis_pool;

    // 连接适配层：进程内共享，供各仓库复用
    repository::PooledDbConnectionProvider    m_db_provider;
    repository::PooledCacheConnectionProvider m_cache_provider;

    // 跨模块共享仓储：只构造一次
    repository::TokenRepository     m_token_repo;
    repository::BlacklistRepository m_blacklist_repo;
    repository::AccountRepository   m_account_repo;

    // 跨模块共享服务
    admin::service::BanService m_ban_service;

    // 全局中间件：只构造并注册一次
    std::shared_ptr<middleware::AccessAuthMiddleware>       m_access_middleware;
    std::shared_ptr<middleware::RefreshAuthMiddleware>      m_refresh_middleware;
    std::shared_ptr<middleware::UserRoleMiddleware>         m_user_role_middleware;
    std::shared_ptr<middleware::AdminAccessAuthMiddleware>  m_admin_access_middleware;
    std::shared_ptr<middleware::AdminRefreshAuthMiddleware> m_admin_refresh_middleware;
    std::shared_ptr<middleware::AdminRoleMiddle>            m_admin_role_middleware;

    // 模块：只注入共享依赖的引用
    std::shared_ptr<modules::AuthModule>      m_auth_module;
    std::shared_ptr<modules::AdminAuthModule> m_admin_auth_module;
    std::shared_ptr<modules::AdminUserModule> m_admin_user_module;
};

}  // namespace app
