//-----------------------------------------------------------------------------
// 文件: server_process.cpp
// 描述: 被测服务子进程管理实现。
//-----------------------------------------------------------------------------
#include "server_process.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "it_env.h"

namespace it
{
namespace
{

bool port_connectable(int port)
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (::getaddrinfo("127.0.0.1", std::to_string(port).c_str(), &hints, &result) != 0)
        return false;

    const int fd = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bool      ok = false;
    if (fd >= 0)
    {
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 200 * 1000;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        ok = ::connect(fd, result->ai_addr, result->ai_addrlen) == 0;
        ::close(fd);
    }
    ::freeaddrinfo(result);
    return ok;
}

std::string tail_of_file(const std::string& path, size_t lines)
{
    std::ifstream in(path);
    if (!in)
        return "<no server log>";

    std::vector<std::string> buffer;
    std::string              line;
    while (std::getline(in, line))
    {
        buffer.push_back(line);
        if (buffer.size() > lines)
            buffer.erase(buffer.begin());
    }

    std::ostringstream out;
    for (const auto& item : buffer)
        out << item << '\n';
    return out.str();
}

}  // namespace

ServerProcess::ServerProcess(int port, std::string log_path)
    : m_port(port), m_log_path(std::move(log_path))
{
}

ServerProcess::~ServerProcess()
{
    stop();
}

void ServerProcess::start()
{
    if (m_pid > 0)
        return;

    std::filesystem::create_directories(std::filesystem::path(m_log_path).parent_path());

    // 子进程需要的配置与日志路径
    ::setenv("SDU_FORUM_CONFIG", config_path().c_str(), 1);
    ::setenv("SDU_IT_SERVER_PORT", std::to_string(m_port).c_str(), 1);
    ::setenv("SDU_IT_LOG_FILE", m_log_path.c_str(), 1);

    const std::string binary = server_binary();
    if (::access(binary.c_str(), X_OK) != 0)
        throw std::runtime_error("integration: server binary not executable: " + binary);

    m_pid = ::fork();
    if (m_pid < 0)
        throw std::runtime_error("integration: fork failed");

    if (m_pid == 0)
    {
        ::chdir(SDU_IT_SOURCE_DIR);
        const int fd = ::open(m_log_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0)
        {
            ::dup2(fd, STDOUT_FILENO);
            ::dup2(fd, STDERR_FILENO);
            ::close(fd);
        }
        ::execl(binary.c_str(), binary.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }

    if (!wait_ready(30000))
    {
        const std::string log = log_tail(80);
        stop();
        throw std::runtime_error("integration: server failed to start on port " +
                                 std::to_string(m_port) + "\n--- server log tail ---\n" + log);
    }
}

bool ServerProcess::wait_ready(int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (port_connectable(m_port))
            return true;

        int status = 0;
        if (::waitpid(m_pid, &status, WNOHANG) == m_pid)
        {
            m_pid = -1;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

void ServerProcess::stop()
{
    if (m_pid <= 0)
        return;

    ::kill(m_pid, SIGTERM);
    for (int i = 0; i < 50; ++i)
    {
        int status = 0;
        if (::waitpid(m_pid, &status, WNOHANG) == m_pid)
        {
            m_pid = -1;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ::kill(m_pid, SIGKILL);
    ::waitpid(m_pid, nullptr, 0);
    m_pid = -1;
}

bool ServerProcess::running() const
{
    return m_pid > 0;
}

std::string ServerProcess::log_tail(size_t lines) const
{
    return tail_of_file(m_log_path, lines);
}

ServerProcess& server()
{
    load_config();
    static ServerProcess instance(server_port(),
                                  std::string(SDU_IT_SOURCE_DIR) + "/build/reports/it-server-" +
                                      std::to_string(server_port()) + ".log");
    instance.start();
    return instance;
}

HttpClient& http()
{
    static HttpClient client("127.0.0.1", server_port());
    return client;
}

}  // namespace it
