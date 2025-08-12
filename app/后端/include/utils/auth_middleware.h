#pragma once
#include<crow.h>
#include<jwt-cpp/jwt.h>
#include"config.h"

void verifier_token(const crow::request& req, crow::response& res, std::function<void()>& next);
