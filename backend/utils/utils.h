#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace utils
{
enum class BearerError
{
    None,
    Missing,     // 头缺失
    BadScheme,   // 非 "Bearer " 前缀
    EmptyToken,  // 前缀后 token 为空
};

/// 解析 Authorization 头中的 Bearer token
/// @return 去掉前缀并 trim 后的 token；失败返回 std::nullopt，原因写入 error
std::optional<std::string> parse_bearer(const std::string& authorization,
                                        BearerError*      error = nullptr);

/// 返回 BearerError 对应的响应文案
const char* bearer_error_message(BearerError error);

std::string trim(const std::string& value);

/// 计算 HMAC-SHA256 并 base64url 编码，用作 token 等敏感串的存储键
std::string hmac_sha256_base64url(const std::string& secret, const std::string& data);

std::string timestamp_to_string(uint64_t timestamp);
}  // namespace utils