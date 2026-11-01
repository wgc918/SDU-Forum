#include <chrono>
#include <drogon/drogon.h>
#include <iomanip>
#include <iostream>
using namespace drogon;

std::string getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

int main() {
  // 添加全局中间件记录所有请求
  app().registerPostHandlingAdvice(
      [](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
        // 这个函数会在每个请求处理完成后调用
        std::cout << "[" << getCurrentTime() << "] " << req->methodString()
                  << " " << req->path() << " " << resp->getStatusCode() << " "
                  << "Client: " << req->peerAddr().toIpPort() << std::endl;
      });

  // 注册路由
  app().registerHandler(
      "/hello", [](const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody("Hello World");
        callback(resp);
      });

  app().registerHandler(
      "/json", [](const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback) {
        Json::Value json;
        json["message"] = "Hello JSON";
        json["status"] = "success";
        auto resp = HttpResponse::newHttpJsonResponse(json);
        callback(resp);
      });

  app().addListener("0.0.0.0", 8080);
  app().run();
}