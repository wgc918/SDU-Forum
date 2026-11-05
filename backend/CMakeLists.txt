cmake_minimum_required(VERSION 3.16)
project(SDUForumServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /EHsc /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Jsoncpp REQUIRED)
find_package(Threads REQUIRED)

set(INFRA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/infrastructrue)

# ---------------------------------------------------------------------------
# 基础设施静态库：数据库连接池、INI、日志、配置、JWT、密码哈希
# ---------------------------------------------------------------------------
add_library(sdu_infra STATIC
    ${INFRA_ROOT}/database-connection-pool/src/ConnectionPool.cpp
    ${INFRA_ROOT}/database-connection-pool/src/MySQLConnection.cpp
    ${INFRA_ROOT}/database-connection-pool/src/MySQLFactory.cpp
    ${INFRA_ROOT}/database-connection-pool/src/MySQLHealthChecker.cpp
    ${INFRA_ROOT}/database-connection-pool/src/MySQLResultSet.cpp
    ${INFRA_ROOT}/database-connection-pool/src/pooledConnGuard.cpp
    ${INFRA_ROOT}/ini/src/ini.cpp
    ${INFRA_ROOT}/ini/src/value.cpp
    ${INFRA_ROOT}/log/src/log.cpp
    ${INFRA_ROOT}/config/app_config.cpp
    ${INFRA_ROOT}/security/password_hasher.cpp
    ${INFRA_ROOT}/auth/token_signer.cpp
)

target_include_directories(sdu_infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${INFRA_ROOT}
    ${INFRA_ROOT}/database-connection-pool/include
    ${INFRA_ROOT}/ini/include
    ${INFRA_ROOT}/log/include
)

target_link_libraries(sdu_infra PUBLIC
    mysqlcppconn
    OpenSSL::Crypto
    Threads::Threads
)

# ---------------------------------------------------------------------------
# 应用可执行文件：Drogon 通信框架 + 分层业务代码
# ---------------------------------------------------------------------------
add_executable(sdu_forum_server
    app/app.cpp
    app/app_context.cpp
    controllers/auth_controller.cpp
    services/auth_service.cpp
    repositories/user_repository.cpp
    repositories/blacklist_repository.cpp
    routes/auth_routes.cpp
    middleware/auth_middleware.cpp
)

target_link_libraries(sdu_forum_server PRIVATE
    sdu_infra
    Drogon::Drogon
    JsonCpp::JsonCpp
)
