#pragma once

#include <drogon/drogon.h>

#include <functional>

#include "common/api_response.h"
#include "common/errors.h"
#include "log.h"

namespace middleware
{

/// 安装全局异常处理器：业务异常转 JSON 错误响应，未知异常统一 500 且不暴露内部细节
inline void install_exception_handler()
{
    drogon::app().setExceptionHandler(
        [](const std::exception& ex, const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            if (const auto* app_error = dynamic_cast<const common::AppException*>(&ex))
            {
                LOGW("%s", (std::string("business error on ") + req->path() + ": " +
                            common::error_code_name(app_error->code()) + " - " + app_error->what())
                               .c_str());
                callback(common::error_response(app_error->code(), app_error->what(),
                                                app_error->details()));
                return;
            }

            LOGE("%s",
                 (std::string("unhandled exception on ") + req->path() + ": " + ex.what()).c_str());
            callback(
                common::error_response(common::ErrorCode::InternalError, "internal server error"));
        });
}

}  // namespace middleware
