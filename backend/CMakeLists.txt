cmake_minimum_required(VERSION 3.10)
project(SDU-ForumApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置编译选项
if(MSVC)
    # MSVC 编译器选项
    add_compile_options(
        /W4                     # 启用警告级别4
        #/WX                     # 将警告视为错误
        /EHsc                   # 启用标准异常处理
        /utf-8                  # 使用UTF-8编码
    )
else()
    # GCC/Clang 编译器选项
    add_compile_options(
        -Wall                   # 启用所有警告
        -Wextra                 # 启用额外警告
        #-Werror                 # 将警告视为错误
        -pedantic               # 启用严格标准检查
    )
endif()



include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    /usr/local/include
    /usr/include/openssl
    /usr/include/x86_64-linux-gnu/curl
)

add_executable(test test.cpp)

target_link_libraries(test
    # Drogon
    jsoncpp
    ssl
    crypto
    uuid
    pthread

    # ImageOss
    /usr/local/lib/libalibabacloud-oss-cpp-sdk.so
    /usr/lib/x86_64-linux-gnu/libssl.so
    /usr/lib/x86_64-linux-gnu/libcrypto.so
    /usr/lib/x86_64-linux-gnu/libcurl.so
    dl

    # Redis
    redis++ 
    hiredis
    
    # Mysql
    mysqlcppconn

)

