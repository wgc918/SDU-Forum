//-----------------------------------------------------------------------------
// 文件: http_client.h
// 描述: 轻量 HTTP/1.1 客户端，用于黑盒调用被测服务（无第三方依赖）。
//-----------------------------------------------------------------------------
#pragma once

#include <json/json.h>

#include <map>
#include <string>

namespace it
{

/// HTTP 响应
struct HttpResponse
{
    int                                status = 0;
    std::string                        body;
    std::map<std::string, std::string> headers;
    Json::Value                        json;

    /// 响应体是否为合法 JSON 对象/数组
    bool has_json() const
    {
        return !json.isNull();
    }
};

/// 同步 HTTP 客户端（短连接）
class HttpClient
{
public:
    HttpClient(std::string host, int port);

    HttpResponse get(const std::string& path,
                     const std::map<std::string, std::string>& headers = {});

    HttpResponse post(const std::string& path, const std::string& body,
                      const std::map<std::string, std::string>& headers = {});

    HttpResponse request(const std::string& method, const std::string& path,
                         const std::string& body = "",
                         const std::map<std::string, std::string>& headers = {});

private:
    std::string m_host;
    int         m_port;
};

/// 构造 Authorization 请求头
std::map<std::string, std::string> bearer(const std::string& token);

}  // namespace it
