#pragma once
#include <string>
#include <memory>
#include<crow.h>
#include <regex>
#include<jdbc/cppconn/statement.h>
#include<jdbc/cppconn/resultset.h>
#include<jdbc/cppconn/exception.h>
#include<jdbc/mysql_driver.h>
#include<jdbc/mysql_connection.h>
#include<jdbc/cppconn/prepared_statement.h>
#define _CRT_SECURE_NO_WARNINGS

class MySQLHandler : public crow::ILogHandler {
public:
    MySQLHandler(const std::string& host, const std::string& user,
        const std::string& password, const std::string& database)
    {
        try {
            driver = sql::mysql::get_mysql_driver_instance();
            con = driver->connect(host, user, password);
            con->setSchema(database);

            // 创建表（如果不存在）
            sql::Statement* stmt = con->createStatement();
            stmt->execute(
                "CREATE TABLE IF NOT EXISTS crow_logs ("
                "id INT AUTO_INCREMENT PRIMARY KEY,"
                "log_time DATETIME NOT NULL,"
                "log_level VARCHAR(10) NOT NULL,"
                "request_id VARCHAR(50),"
                "ip_address VARCHAR(50),"
                "port INT,"
                "http_method VARCHAR(10),"
                "endpoint VARCHAR(255),"
                "status_code INT,"
                "response_time_ms INT,"
                "query_params TEXT,"
                "message TEXT,"
                "is_request BOOLEAN,"
                "full_url TEXT"
                ")");
            delete stmt;
        }
        catch (sql::SQLException& e) {
            std::cerr << "MySQL Connection Error: " << e.what() << std::endl;
            throw;
        }
    }

    ~MySQLHandler() {
        if (con) {
            delete con;
        }
    }

    void log(std::string message, crow::LogLevel level) override {
        std::lock_guard<std::mutex> lock(db_mutex);

        try {
            // 获取当前时间
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm;
            localtime_s(&now_tm, &now_time_t);

            std::ostringstream timestamp;
            timestamp << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

            // 解析日志消息
            LogData log_data = parse_log_message(message, level);

            // 准备SQL语句
            sql::PreparedStatement* pstmt = con->prepareStatement(
                "INSERT INTO crow_logs (log_time, log_level, request_id, ip_address, port, "
                "http_method, endpoint, status_code, response_time_ms, query_params, "
                "message, is_request, full_url) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            // 设置参数
            pstmt->setString(1, timestamp.str());
            pstmt->setString(2, log_level_to_string(level));
            pstmt->setString(3, log_data.request_id);

            if (!log_data.ip_address.empty()) {
                pstmt->setString(4, log_data.ip_address);
            }
            else {
                pstmt->setNull(4, sql::DataType::VARCHAR);
            }

            if (log_data.port > 0) {
                pstmt->setInt(5, log_data.port);
            }
            else {
                pstmt->setNull(5, sql::DataType::INTEGER);
            }

            pstmt->setString(6, log_data.http_method);
            pstmt->setString(7, log_data.endpoint);

            if (log_data.status_code > 0) {
                pstmt->setInt(8, log_data.status_code);
            }
            else {
                pstmt->setNull(8, sql::DataType::INTEGER);
            }

            if (log_data.response_time >= 0) {
                pstmt->setInt(9, log_data.response_time);
            }
            else {
                pstmt->setNull(9, sql::DataType::INTEGER);
            }

            pstmt->setString(10, log_data.query_params);
            pstmt->setString(11, log_data.message);
            pstmt->setBoolean(12, log_data.is_request);
            pstmt->setString(13, log_data.full_url);

            // 执行
            pstmt->executeUpdate();
            delete pstmt;
        }
        catch (sql::SQLException& e) {
            std::cerr << "MySQL Error [" << e.getErrorCode() << "]: "
                << e.what() << "\nOriginal log: " << message << std::endl;
        }
        catch (std::exception& e) {
            std::cerr << "Log Processing Error: " << e.what()
                << "\nOriginal log: " << message << std::endl;
        }
    }

private:
    struct LogData {
        std::string request_id;
        std::string ip_address;
        int port = 0;
        std::string http_method;
        std::string endpoint;
        int status_code = 0;
        int response_time = -1;
        std::string query_params;
        std::string message;
        bool is_request = false;
        std::string full_url;
    };

    sql::mysql::MySQL_Driver* driver;
    sql::Connection* con;
    std::mutex db_mutex;

    LogData parse_log_message(const std::string& message, crow::LogLevel level) {
        LogData data;
        data.message = message;

        try {
            // 解析请求日志
            if (message.find("Request:") != std::string::npos) {
                data.is_request = true;

                // 格式: Request: IP:端口 请求ID HTTP版本 方法 路径
                std::regex req_regex(R"(Request: ([\d\.]+):(\d+) (\w+) HTTP/(\d\.\d) (GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS) (.+))");
                std::smatch matches;

                if (std::regex_search(message, matches, req_regex) && matches.size() >= 6) {
                    data.ip_address = matches[1].str();
                    data.port = std::stoi(matches[2].str());
                    data.request_id = matches[3].str();
                    data.http_method = matches[5].str();
                    data.full_url = matches[6].str();

                    // 分离端点和查询参数
                    size_t qmark = data.full_url.find('?');
                    if (qmark != std::string::npos) {
                        data.endpoint = data.full_url.substr(0, qmark);
                        data.query_params = data.full_url.substr(qmark + 1);
                    }
                    else {
                        data.endpoint = data.full_url;
                    }
                }
            }
            // 解析响应日志
            else if (message.find("Response:") != std::string::npos) {
                data.is_request = false;

                // 格式: Response: 请求ID 路径 状态码 响应时间
                std::regex res_regex(R"(Response: (\w+) (.+) (\d+) (\d+))");
                std::smatch matches;

                if (std::regex_search(message, matches, res_regex) && matches.size() >= 5) {
                    data.request_id = matches[1].str();
                    data.full_url = matches[2].str();
                    data.status_code = std::stoi(matches[3].str());
                    data.response_time = std::stoi(matches[4].str());

                    // 分离端点和查询参数
                    size_t qmark = data.full_url.find('?');
                    if (qmark != std::string::npos) {
                        data.endpoint = data.full_url.substr(0, qmark);
                        data.query_params = data.full_url.substr(qmark + 1);
                    }
                    else {
                        data.endpoint = data.full_url;
                    }
                }
            }
        }
        catch (std::exception& e) {
            // 解析失败时保留原始消息
            data.message = "[PARSING ERROR] " + message;
        }

        return data;
    }

    std::string log_level_to_string(crow::LogLevel level) {
        switch (level) {
        case crow::LogLevel::Debug: return "DEBUG";
        case crow::LogLevel::Info: return "INFO";
        case crow::LogLevel::Warning: return "WARNING";
        case crow::LogLevel::Error: return "ERROR";
        case crow::LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
        }
    }
};