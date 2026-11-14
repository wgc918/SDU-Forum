#include "utils.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <ctime>
#include <optional>

namespace utils
{
namespace
{

constexpr std::size_t kBearerPrefixLength = 7;  // "Bearer "

bool has_bearer_prefix(const std::string& value)
{
    static const char kExpected[] = "bearer ";

    if (value.size() < kBearerPrefixLength)
        return false;

    for (std::size_t i = 0; i < kBearerPrefixLength; ++i)
    {
        if (std::tolower(static_cast<unsigned char>(value[i])) != kExpected[i])
            return false;
    }
    return true;
}

}  // namespace

std::optional<std::string> parse_bearer(const std::string& authorization,
                                        BearerError*       error)
{
    auto fail = [error](BearerError e) -> std::optional<std::string>
    {
        if (error)
            *error = e;
        return std::nullopt;
    };

    if (authorization.empty())
        return fail(BearerError::Missing);
    if (!has_bearer_prefix(authorization))
        return fail(BearerError::BadScheme);

    std::string token = trim(authorization.substr(kBearerPrefixLength));
    if (token.empty())
        return fail(BearerError::EmptyToken);
    return token;
}

const char* bearer_error_message(BearerError error)
{
    switch (error)
    {
        case BearerError::Missing:
            return "authorization header is required";
        case BearerError::BadScheme:
            return "authorization header must use the Bearer scheme";
        case BearerError::EmptyToken:
            return "bearer token must not be empty";
        case BearerError::None:
            break;
    }
    return "";
}

std::string trim(const std::string& value)
{
    std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string base64_url_encode(const std::string& input)
{
    std::string out;
    out.resize(static_cast<std::size_t>(4 * ((input.size() + 2) / 3)) + 1);

    int written = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out.data()),
                                  reinterpret_cast<const unsigned char*>(input.data()),
                                  static_cast<int>(input.size()));
    if (written < 0)
        return "";

    out.resize(static_cast<std::size_t>(written));

    for (char& ch : out)
    {
        if (ch == '+')
            ch = '-';
        else if (ch == '/')
            ch = '_';
    }

    while (!out.empty() && out.back() == '=')
        out.pop_back();

    return out;
}

std::string hmac_sha256(const std::string& secret, const std::string& data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;

    if (HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
             reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest,
             &digest_len) == nullptr)
    {
        return "";
    }

    return base64_url_encode(std::string(reinterpret_cast<char*>(digest), digest_len));
}

// 默认使用 UTC 时间；如需本地时间，把 gmtime_r 换成 localtime_r
// 格式 YYYY-MM-DD HH:MM:SS
std::string timestamp_to_string(uint64_t timestamp)
{
    std::time_t t = static_cast<std::time_t>(timestamp);

    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &t);  // Windows
#else
    gmtime_r(&t, &tm_buf);  // POSIX
#endif

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", tm_buf.tm_year + 1900,
                  tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    return std::string(buf);
}
}  // namespace utils