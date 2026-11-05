#include "app/app_context.h"

#include "conn-pool/ConnectionPool.h"
#include "log.h"
#include "middleware/auth_middleware.h"
#include "middleware/exception_handler.h"
#include "routes/routes.h"

namespace app
{

AppContext::AppContext(const infra::AppConfig& config)
    : m_config(config),
      m_db_pool(&dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL)),
      m_user_repository(*m_db_pool, config.auth().account_table),
      m_blacklist_repository(*m_db_pool, config.auth().blacklist_table),
      m_auth_service(m_user_repository, m_blacklist_repository, config.jwt()),
      m_auth_controller(m_auth_service),
      m_auth_middleware(std::make_shared<middleware::AuthMiddleware>(config.jwt().access_secret,
                                                                     config.jwt().issuer))
{
}

AppContext::~AppContext()
{
    if (m_db_pool != nullptr)
        m_db_pool->shutdown();
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

    LOGI("%s", (std::string("database connection pool initialized: ") + db.host + ":" +
                std::to_string(db.port) + "/" + db.dbname)
                   .c_str());
}

void AppContext::register_routes()
{
    middleware::install_exception_handler();
    drogon::app().registerMiddleware(m_auth_middleware);
    routes::register_auth_routes(drogon::app(), m_auth_controller);
    LOGI("routes registered");
}

}  // namespace app
