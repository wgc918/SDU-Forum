#include <drogon/drogon.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "app/app_context.h"
#include "config/app_config.h"
#include "log.h"

namespace
{

std::string locate_config(int argc, char** argv)
{
    if (const char* from_env = std::getenv("SDU_FORUM_CONFIG"); from_env != nullptr && *from_env)
        return from_env;

    if (argc > 1)
        return argv[1];

    for (const char* candidate :
         {"config/config-dev.ini", "../config/config-dev.ini", "../../config/config-dev.ini"})
    {
        std::ifstream probe(candidate);
        if (probe.is_open())
            return candidate;
    }

    return "config/config-dev.ini";
}

void init_logger(const infra::LoggingConfig& logging)
{
    if (!logging.file.empty())
    {
        std::filesystem::path log_path(logging.file);
        if (log_path.has_parent_path())
            std::filesystem::create_directories(log_path.parent_path());
    }

    LogConfig log_config;
    log_config.file_name = logging.file;
    log_config.level     = string_to_LogLevel(logging.level);
    log_config.console   = logging.console;
    log_config.max_lines = logging.max_lines;

    Logger::instance().init(log_config);
}

}  // namespace

int main(int argc, char** argv)
{
    try
    {
        const std::string config_path = locate_config(argc, argv);
        infra::AppConfig::instance().load(config_path);

        const auto& config = infra::AppConfig::instance();
        init_logger(config.logging());
        LOGI("%s", (std::string("configuration loaded: ") + config_path).c_str());

        app::AppContext context(config);
        context.init_infrastructure();
        context.register_routes();

        drogon::app().registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr&  req,
               const drogon::HttpResponsePtr& resp)
            {
                LOGI("%s", (std::string(req->methodString()) + " " + req->path() + " -> " +
                            std::to_string(static_cast<int>(resp->getStatusCode())))
                               .c_str());
            });

        drogon::app().addListener(config.server().address, config.server().port);
        drogon::app().setThreadNum(config.server().thread_num);

        LOGI("%s", (std::string("server listening on ") + config.server().address + ":" +
                    std::to_string(config.server().port))
                       .c_str());

        drogon::app().run();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "failed to start server: %s\n", e.what());
        return 1;
    }

    return 0;
}
