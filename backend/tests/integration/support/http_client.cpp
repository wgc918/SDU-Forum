//-----------------------------------------------------------------------------
// 文件: http_client.cpp
// 描述: 轻量 HTTP/1.1 客户端实现（POSIX socket + jsoncpp）。
//-----------------------------------------------------------------------------
#include "http_client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace it
{
namespace
{

std::string to_lower(std::string value)
{
    for (auto& ch : value)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return value;
}

std::string trim(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return "";
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

/// 判断响应体是否已经完整接收（避免依赖服务端关闭连接）
bool body_complete(const std::string& raw)
{
    const auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    const std::string head = raw.substr(0, header_end);
    int               status = 0;
    {
        std::istringstream status_stream(head);
        std::string        version;
        status_stream >> version >> status;
    }
    if (status == 204 || status == 304)
        return true;

    const std::string lower = to_lower(head);
    const std::string body  = raw.substr(header_end + 4);

    if (lower.find("transfer-encoding:") != std::string::npos &&
        lower.find("chunked") != std::string::npos)
    {
        return body.compare(0, 5, "0\r\n\r\n") == 0 ||
               body.find("\r\n0\r\n\r\n") != std::string::npos;
    }

    const auto length_pos = lower.find("content-length:");
    if (length_pos != std::string::npos)
    {
        const auto eol = lower.find("\r\n", length_pos);
        const auto val = trim(head.substr(length_pos + 15, eol - (length_pos + 15)));
        try
        {
            return body.size() >= static_cast<size_t>(std::stoul(val));
        }
        catch (...)
        {
            return true;
        }
    }

    return false;  // 无长度信息时只能读到 EOF
}

/// 解析 chunked 传输编码
std::string decode_chunked(const std::string& raw)
{
    std::string out;
    size_t      pos = 0;
    while (pos < raw.size())
    {
        const auto line_end = raw.find("\r\n", pos);
        if (line_end == std::string::npos)
            break;

        const std::string size_line = raw.substr(pos, line_end - pos);
        const size_t      ext_pos   = size_line.find(';');
        const std::string hex       = trim(ext_pos == std::string::npos ? size_line
                                                                        : size_line.substr(0, ext_pos));
        size_t            chunk_size = 0;
        try
        {
            chunk_size = std::stoul(hex, nullptr, 16);
        }
        catch (...)
        {
            break;
        }

        pos = line_end + 2;
        if (chunk_size == 0)
            break;
        if (pos + chunk_size > raw.size())
            break;

        out.append(raw, pos, chunk_size);
        pos += chunk_size + 2;  // 跳过 chunk 末尾 CRLF
    }
    return out;
}

}  // namespace

HttpClient::HttpClient(std::string host, int port) : m_host(std::move(host)), m_port(port) {}

HttpResponse HttpClient::get(const std::string& path,
                             const std::map<std::string, std::string>& headers)
{
    return request("GET", path, "", headers);
}

HttpResponse HttpClient::post(const std::string& path, const std::string& body,
                              const std::map<std::string, std::string>& headers)
{
    return request("POST", path, body, headers);
}

HttpResponse HttpClient::request(const std::string& method, const std::string& path,
                                 const std::string&                        body,
                                 const std::map<std::string, std::string>& headers)
{
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (::getaddrinfo(m_host.c_str(), std::to_string(m_port).c_str(), &hints, &result) != 0)
        throw std::runtime_error("http: getaddrinfo failed for " + m_host);

    int fd = -1;
    for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next)
    {
        fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;

        timeval tv{};
        tv.tv_sec = 5;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;
        ::close(fd);
        fd = -1;
    }
    ::freeaddrinfo(result);

    if (fd < 0)
        throw std::runtime_error("http: cannot connect to " + m_host + ":" +
                                 std::to_string(m_port));

    std::ostringstream request;
    request << method << " " << path << " HTTP/1.1\r\n"
            << "Host: " << m_host << ":" << m_port << "\r\n"
            << "Connection: close\r\n";
    for (const auto& [key, value] : headers)
        request << key << ": " << value << "\r\n";
    if (!body.empty())
        request << "Content-Length: " << body.size() << "\r\n";
    request << "\r\n" << body;

    const std::string raw_request = request.str();
    size_t            sent        = 0;
    while (sent < raw_request.size())
    {
        const ssize_t n = ::send(fd, raw_request.data() + sent, raw_request.size() - sent, 0);
        if (n <= 0)
        {
            ::close(fd);
            throw std::runtime_error("http: send failed");
        }
        sent += static_cast<size_t>(n);
    }

    std::string raw_response;
    char        buffer[8192];
    while (true)
    {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n == 0)
            break;
        if (n < 0)
        {
            ::close(fd);
            throw std::runtime_error("http: recv timeout/failed");
        }
        raw_response.append(buffer, static_cast<size_t>(n));
        if (body_complete(raw_response))
            break;
    }
    ::close(fd);

    const auto header_end = raw_response.find("\r\n\r\n");
    if (header_end == std::string::npos)
        throw std::runtime_error("http: malformed response");

    const std::string head    = raw_response.substr(0, header_end);
    const std::string raw_body = raw_response.substr(header_end + 4);

    HttpResponse response;
    std::istringstream stream(head);
    std::string        status_line;
    std::getline(stream, status_line);
    {
        std::istringstream status_stream(status_line);
        std::string        version;
        status_stream >> version >> response.status;
    }

    std::string line;
    while (std::getline(stream, line))
    {
        line = trim(line);
        if (line.empty())
            continue;
        const auto colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        response.headers[to_lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
    }

    const auto transfer_it = response.headers.find("transfer-encoding");
    if (transfer_it != response.headers.end() &&
        to_lower(transfer_it->second).find("chunked") != std::string::npos)
    {
        response.body = decode_chunked(raw_body);
    }
    else
    {
        response.body = raw_body;
        const auto length_it = response.headers.find("content-length");
        if (length_it != response.headers.end())
        {
            const size_t length = static_cast<size_t>(std::stoul(length_it->second));
            if (response.body.size() > length)
                response.body.resize(length);
        }
    }

    if (!response.body.empty())
    {
        Json::CharReaderBuilder builder;
        std::string            errors;
        std::istringstream     json_stream(response.body);
        Json::parseFromStream(builder, json_stream, &response.json, &errors);
    }

    return response;
}

std::map<std::string, std::string> bearer(const std::string& token)
{
    return {{"Authorization", "Bearer " + token}, {"Content-Type", "application/json"}};
}

}  // namespace it
