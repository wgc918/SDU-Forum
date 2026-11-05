#include <drogon/drogon.h>

using namespace drogon;

class MyMiddleware : public drogon::HttpMiddleware<MyMiddleware>
{
public:
    void invoke(const HttpRequestPtr& req, MiddlewareNextCallback&& nextCb,
                MiddlewareCallback&& mcb) override
    {
        // ============ before_handle ============
        before_handle(req);

        // ============ 交给下一环 ============
        nextCb(
            [this, req, mcb = std::move(mcb)](const HttpResponsePtr& resp) mutable
            {
                // ============ after_handle ============
                after_handle(req, resp);
                mcb(std::move(resp));
            });
    }

private:
    void before_handle(const HttpRequestPtr& req)
    {
        req->attributes()->insert("my-key", "my-value");
    }

    void after_handle(const HttpRequestPtr& req, const HttpResponsePtr& resp)
    {
        resp->addHeader("X-My-Middleware", "1");
    }
};