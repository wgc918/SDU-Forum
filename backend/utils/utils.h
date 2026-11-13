#pragma once
#include <cstdint>
#include <string>

namespace utils
{
constexpr std::size_t kBearerPrefixLength = 7;  // "Bearer "

bool has_bearer_prefix(const std::string& value);
std::string trim(const std::string& value);

std::string hmac_sha256(const std::string& secret, const std::string& data);

std::string timestamp_to_string(uint64_t timestamp);
}  // namespace utils