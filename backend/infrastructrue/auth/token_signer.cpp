#include "auth/token_signer.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <json/json.h>

namespace infra
{
namespace
{

constexpr char kAlgorithm[] = "HS256";

std::string base64_url_encode(const std::string& input)
{
    std::string out;
    out.resize(static_cast<std::size_t>(4 * ((input.size() + 2) / 3)) + 1);

    int written = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out.data()),
                                  reinterpret_cast<const unsigned char*>(input.data()),
                                  static_cast<int>(input.size()));
    if (written < 0)
        throw std::runtime_error("failed to base64 encode jwt segment");

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

bool base64_url_decode(const std::string& input, std::string& output)
{
    std::string base64 = input;
    for (char& ch : base64)
    {
        if (ch == '-')
            ch = '+';
        else if (ch == '_')
            ch = '/';
    }

    if (base64.size() % 4 == 1)
        return false;

    std::size_t padding = (4 - base64.size() % 4) % 4;
    base64.append(padding, '=');

    std::vector<unsigned char> buffer(base64.size() / 4 * 3);
    int written = EVP_DecodeBlock(buffer.data(),
                                  reinterpret_cast<const unsigned char*>(base64.data()),
                                  static_cast<int>(base64.size()));
    if (written < 0 || static_cast<std::size_t>(written) < padding)
        return false;

    output.assign(reinterpret_cast<char*>(buffer.data()),
                  static_cast<std::size_t>(written) - padding);
    return true;
}

std::string compact_json(const Json::Value& value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

std::string hmac_sha256(const std::string& secret, const std::string& data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;

    if (HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
             reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest,
             &digest_len) == nullptr)
    {
        throw std::runtime_error("failed to sign jwt");
    }

    return std::string(reinterpret_cast<char*>(digest), digest_len);
}

int64_t unix_now()
{
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

bool is_numeric(const Json::Value& value)
{
    return value.isInt() || value.isUInt() || value.isInt64() || value.isUInt64();
}

bool parse_json(const std::string& text, Json::Value& output)
{
    Json::CharReaderBuilder builder;
    std::string             errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    return reader->parse(text.data(), text.data() + text.size(), &output, &errors);
}

}  // namespace

TokenSigner::TokenSigner(std::string secret, std::string issuer)
    : m_secret(std::move(secret)), m_issuer(std::move(issuer))
{
}

SignedToken TokenSigner::sign(const std::string& user_id, int64_t ttl_seconds) const
{
    Json::Value header;
    header["alg"] = kAlgorithm;
    header["typ"] = "JWT";

    const int64_t now     = unix_now();
    const int64_t expires = now + ttl_seconds;

    Json::Value payload;
    payload["iss"]     = m_issuer;
    payload["iat"]     = Json::Int64(now);
    payload["exp"]     = Json::Int64(expires);
    payload["user_id"] = user_id;

    std::string unsigned_token = base64_url_encode(compact_json(header)) + "." +
                                 base64_url_encode(compact_json(payload));

    std::string signature = base64_url_encode(hmac_sha256(m_secret, unsigned_token));

    return SignedToken{unsigned_token + "." + signature, expires};
}

std::optional<TokenPayload> TokenSigner::verify(const std::string& token) const
{
    const std::size_t first_dot = token.find('.');
    if (first_dot == std::string::npos)
        return std::nullopt;

    const std::size_t second_dot = token.find('.', first_dot + 1);
    if (second_dot == std::string::npos || token.find('.', second_dot + 1) != std::string::npos)
        return std::nullopt;

    const std::string header_segment    = token.substr(0, first_dot);
    const std::string payload_segment   = token.substr(first_dot + 1, second_dot - first_dot - 1);
    const std::string signature_segment = token.substr(second_dot + 1);

    if (header_segment.empty() || payload_segment.empty() || signature_segment.empty())
        return std::nullopt;

    // 先校验签名，签名通过之前不信任任何 payload 内容
    const std::string expected_signature =
        base64_url_encode(hmac_sha256(m_secret, header_segment + "." + payload_segment));

    if (expected_signature.size() != signature_segment.size() ||
        CRYPTO_memcmp(expected_signature.data(), signature_segment.data(),
                      expected_signature.size()) != 0)
    {
        return std::nullopt;
    }

    std::string header_json;
    std::string payload_json;
    if (!base64_url_decode(header_segment, header_json) ||
        !base64_url_decode(payload_segment, payload_json))
    {
        return std::nullopt;
    }

    Json::Value header;
    Json::Value payload;
    if (!parse_json(header_json, header) || !header.isObject())
        return std::nullopt;
    if (!header["alg"].isString() || header["alg"].asString() != kAlgorithm)
        return std::nullopt;

    if (!parse_json(payload_json, payload) || !payload.isObject())
        return std::nullopt;
    if (!payload["iss"].isString() || payload["iss"].asString() != m_issuer)
        return std::nullopt;
    if (!payload["user_id"].isString() || payload["user_id"].asString().empty())
        return std::nullopt;
    if (!is_numeric(payload["exp"]))
        return std::nullopt;

    const int64_t expires_at = payload["exp"].asInt64();
    if (expires_at < unix_now())
        return std::nullopt;

    TokenPayload result;
    result.user_id    = payload["user_id"].asString();
    result.issuer     = payload["iss"].asString();
    result.expires_at = expires_at;
    result.issued_at  = is_numeric(payload["iat"]) ? payload["iat"].asInt64() : 0;
    return result;
}

}  // namespace infra
