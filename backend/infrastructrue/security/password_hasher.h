#pragma once

#include <string>

namespace infra
{

struct PasswordHash
{
    std::string hash;  // 64 位十六进制 PBKDF2 输出
    std::string salt;  // 32 位十六进制盐
    int         iterations = 0;
};

/// 校验 PBKDF2-HMAC-SHA512 密码哈希，非法存储格式返回 false
bool verify_password(const std::string& password, const PasswordHash& stored);

/// 生成 PBKDF2-HMAC-SHA512 密码哈希与随机盐，失败时返回空结构
PasswordHash hash_password(const std::string& password, int iterations = 150000);

}  // namespace infra
