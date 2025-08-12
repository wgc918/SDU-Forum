#include<crow.h>
#include<jwt-cpp/jwt.h>
#include"config.h"

// 中间件函数 验证token有效性
void verifier_token(const crow::request& req, crow::response& res, std::function<void()>& next) {
    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
        res.code = 401;
        res.write("No token provided");
        res.end();
        return;
    }

    std::string token = auth_header.substr(7);
    try {
        auto decoded = jwt::decode(token);
        // 创建验证器
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ ACCESS_JWT_SECRET })
            .with_issuer(ISSUER);

        // 验证 token
        verifier.verify(decoded);
        next(); 
    }
    catch (...) {
        res.code = 401;
        res.write("Invalid token");
        res.end();
    }
    };