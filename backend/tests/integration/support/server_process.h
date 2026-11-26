//-----------------------------------------------------------------------------
// 文件: server_process.h
// 描述: 以子进程方式启动/停止被测服务，供接口层集成测试使用。
//-----------------------------------------------------------------------------
#pragma once

#include "http_client.h"

namespace it
{

/// 被测服务子进程管理
class ServerProcess
{
public:
    ServerProcess(int port, std::string log_path);
    ~ServerProcess();

    ServerProcess(const ServerProcess&)            = delete;
    ServerProcess& operator=(const ServerProcess&) = delete;

    /// 启动并等待端口可连接
    /// @throws std::runtime_error 启动失败或超时（附带日志尾部）
    void start();

    /// 优雅停止（SIGTERM，必要时 SIGKILL）
    void stop();

    bool running() const;
    int  port() const
    {
        return m_port;
    }

    /// 读取日志尾部，失败诊断使用
    std::string log_tail(size_t lines = 50) const;

private:
    bool wait_ready(int timeout_ms);

    int         m_port;
    std::string m_log_path;
    int         m_pid = -1;
};

/// 进程内唯一的被测服务实例（懒启动）
ServerProcess& server();

/// 绑定到被测服务的 HTTP 客户端
HttpClient& http();

}  // namespace it
