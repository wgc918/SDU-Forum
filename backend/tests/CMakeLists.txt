# ---------------------------------------------------------------------------
# TokenRepository 单元测试：快速、无外部依赖
# ---------------------------------------------------------------------------
add_executable(test_token_repository
    support/test_main.cpp
    unit/repositories/test_token_repository.cpp
)

target_include_directories(test_token_repository PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 第三方单头文件按 SYSTEM 引入，避免其内部实现触发项目警告
target_include_directories(test_token_repository SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(test_token_repository PRIVATE sdu_app)

add_test(NAME TokenRepositoryUnit COMMAND test_token_repository)
set_tests_properties(TokenRepositoryUnit PROPERTIES
    LABELS "fast;unit"
    TIMEOUT 10
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# AccountRepository 单元测试：快速、无外部依赖
# ---------------------------------------------------------------------------
add_executable(test_account_repository
    support/test_main.cpp
    unit/repositories/test_account_repository.cpp
)

target_include_directories(test_account_repository PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_include_directories(test_account_repository SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(test_account_repository PRIVATE sdu_app)

add_test(NAME AccountRepositoryUnit COMMAND test_account_repository)
set_tests_properties(AccountRepositoryUnit PROPERTIES
    LABELS "fast;unit"
    TIMEOUT 10
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# AdminAccountRepository 单元测试：快速、无外部依赖
# ---------------------------------------------------------------------------
add_executable(test_admin_account_repository
    support/test_main.cpp
    unit/repositories/test_admin_account_repository.cpp
)

target_include_directories(test_admin_account_repository PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_include_directories(test_admin_account_repository SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(test_admin_account_repository PRIVATE sdu_app)

add_test(NAME AdminAccountRepositoryUnit COMMAND test_admin_account_repository)
set_tests_properties(AdminAccountRepositoryUnit PROPERTIES
    LABELS "fast;unit"
    TIMEOUT 10
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# Utils 单元测试：时间格式化等纯函数
# ---------------------------------------------------------------------------
add_executable(test_utils
    support/test_main.cpp
    unit/utils/test_utils.cpp
)

target_include_directories(test_utils PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_include_directories(test_utils SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(test_utils PRIVATE sdu_app)

add_test(NAME UtilsUnit COMMAND test_utils)
set_tests_properties(UtilsUnit PROPERTIES
    LABELS "fast;unit"
    TIMEOUT 10
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
