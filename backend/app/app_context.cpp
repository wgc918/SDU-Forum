#include "app/app_context.h"

#include <drogon/HttpAppFramework.h>

#include <memory>

#include "app/modules/auth_module.h"
#include "config/app_config.h"
#include "conn-pool/ConnectionPool.h"
#include "log.h"
#include "middleware/exception_handler.h"
#include "redis-pool/RedisConnectionFactory.h"
#include "redis-pool/RedisConnectionPool.h"

namespace app
{

AppContext::AppContext(const infra::AppConfig& config)
    : m_config(config),
      m_db_pool(&dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL)),
      m_redis_pool(&rcp::RedisConnectionPool::instance()),
      m_auth_module(std::make_shared<modules::AuthModule>(m_config, m_db_pool, m_redis_pool))
{
}

AppContext::~AppContext()
{
    if (m_db_pool != nullptr)
        m_db_pool->shutdown();
    if (m_redis_pool != nullptr)
        m_redis_pool->shutdown();
}

void AppContext::init_infrastructure()
{
    const auto& db = m_config.database();

    dcp::ConnectionConfig connection_config;
    connection_config.db_name   = db.dbname;
    connection_config.user_name = db.user;
    connection_config.password  = db.password;
    connection_config.ip        = db.host;
    connection_config.port      = db.port;

    m_db_pool->init(connection_config, db.pool_min, db.pool_max, db.max_idle_time,
                    db.connection_timeout, db.op_num);

    LOGI("database connection pool initialized: %s:%d/%s", db.host.c_str(), db.port,
         db.dbname.c_str());

    const auto&                redis_db = m_config.redis();
    rcp::RedisConnectionConfig redis_config;
    redis_config.host     = redis_db.host;
    redis_config.port     = redis_db.port;
    redis_config.password = redis_db.password;
    redis_config.db       = redis_db.db;
    m_redis_pool->init(redis_config);
    LOGI("redis connection pool initialized: %s:%d/%d", redis_config.host.c_str(),
         redis_config.port, redis_config.db);
}

void AppContext::register_routes()
{
    m_auth_module->register_routes(drogon::app());
    LOGI("routes registered");
}

void AppContext::register_middlewares()
{
    middleware::install_exception_handler();
    m_auth_module->register_middlewares(drogon::app());
    LOGI("middlewares registered");
}

}  // namespace app
