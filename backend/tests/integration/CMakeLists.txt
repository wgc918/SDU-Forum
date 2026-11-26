# ---------------------------------------------------------------------------
# 集成测试：分层（L4 组件 / L3 数据 / L2 服务 / L1 接口）
# 依赖真实 MySQL；Redis 缺失时相关用例通过 doctest::skip 跳过
# ---------------------------------------------------------------------------

set(IT_SUPPORT
    ${CMAKE_CURRENT_SOURCE_DIR}/support/it_env.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/support/it_support.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/support/http_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/support/server_process.cpp
)

# sdu_add_integration_test(<name> <source> [HTTP] [REDIS] [NO_DB])
function(sdu_add_integration_test name source)
    cmake_parse_arguments(ARG "HTTP;REDIS;NO_DB" "" "" ${ARGN})

    add_executable(test_it_${name}
        ${source}
        ${IT_SUPPORT}
        ${CMAKE_CURRENT_SOURCE_DIR}/../support/test_main.cpp
    )

    target_include_directories(test_it_${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
    )

    target_include_directories(test_it_${name} SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )

    target_link_libraries(test_it_${name} PRIVATE sdu_app)
    add_dependencies(test_it_${name} sdu_forum_server)

    target_compile_definitions(test_it_${name} PRIVATE
        SDU_IT_SERVER_BIN="$<TARGET_FILE:sdu_forum_server>"
        SDU_IT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )

    set(labels "integration")
    if(ARG_REDIS)
        list(APPEND labels "redis")
    endif()
    if(ARG_HTTP)
        list(APPEND labels "http")
    endif()
    if(NOT ARG_NO_DB)
        list(APPEND labels "db")
    endif()

    add_test(NAME IT_${name} COMMAND test_it_${name})
    set_tests_properties(IT_${name} PROPERTIES
        LABELS "${labels}"
        TIMEOUT 180
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT "SDU_FORUM_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/config/config-int.ini"
    )

    if(ARG_HTTP)
        set_tests_properties(IT_${name} PROPERTIES RUN_SERIAL TRUE)
    endif()

    if(NOT ARG_NO_DB)
        set_tests_properties(IT_${name} PROPERTIES
            FIXTURES_REQUIRED IntegrationDb
            RESOURCE_LOCK sdu_it_db
        )
    endif()
endfunction()

# ---------------------------------------------------------------------------
# 夹具：建表（每个 it_ 表均先 DROP 再 CREATE）
# ---------------------------------------------------------------------------
add_test(NAME IntegrationDbSetup
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup_db.sh
)
set_tests_properties(IntegrationDbSetup PROPERTIES
    LABELS "integration;setup"
    FIXTURES_SETUP IntegrationDb
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# L4 组件层
# ---------------------------------------------------------------------------
sdu_add_integration_test(config ${CMAKE_CURRENT_SOURCE_DIR}/infra/test_it_config.cpp NO_DB)
sdu_add_integration_test(db_pool ${CMAKE_CURRENT_SOURCE_DIR}/infra/test_it_db_pool.cpp)
sdu_add_integration_test(redis_pool ${CMAKE_CURRENT_SOURCE_DIR}/infra/test_it_redis_pool.cpp REDIS)

# ---------------------------------------------------------------------------
# L3 数据层
# ---------------------------------------------------------------------------
sdu_add_integration_test(account_repo ${CMAKE_CURRENT_SOURCE_DIR}/repositories/test_it_account_repo.cpp)
sdu_add_integration_test(token_repo ${CMAKE_CURRENT_SOURCE_DIR}/repositories/test_it_token_repo.cpp)
sdu_add_integration_test(blacklist_repo ${CMAKE_CURRENT_SOURCE_DIR}/repositories/test_it_blacklist_repo.cpp)

# ---------------------------------------------------------------------------
# L2 服务层
# ---------------------------------------------------------------------------
sdu_add_integration_test(auth_flow ${CMAKE_CURRENT_SOURCE_DIR}/services/test_it_auth_flow.cpp)
sdu_add_integration_test(admin_auth_flow ${CMAKE_CURRENT_SOURCE_DIR}/services/test_it_admin_auth_flow.cpp)
sdu_add_integration_test(admin_user_flow ${CMAKE_CURRENT_SOURCE_DIR}/services/test_it_admin_user_flow.cpp)

# ---------------------------------------------------------------------------
# L1 接口层
# ---------------------------------------------------------------------------
sdu_add_integration_test(api_user_auth ${CMAKE_CURRENT_SOURCE_DIR}/api/test_it_api_user_auth.cpp HTTP)
sdu_add_integration_test(api_admin_auth ${CMAKE_CURRENT_SOURCE_DIR}/api/test_it_api_admin_auth.cpp HTTP)
sdu_add_integration_test(api_admin_users ${CMAKE_CURRENT_SOURCE_DIR}/api/test_it_api_admin_users.cpp HTTP)
sdu_add_integration_test(api_contract ${CMAKE_CURRENT_SOURCE_DIR}/api/test_it_api_contract.cpp HTTP)
