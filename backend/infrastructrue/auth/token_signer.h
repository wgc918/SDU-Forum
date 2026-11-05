#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace infra
{

struct SignedToken
{
    std::string value;
    int64_t     expires_at;  // Unix 秒
};

struct TokenPayload
{
    std::string user_id;
    std::string issuer;
    int64_t     issued_at  = 0;
    int64_t     expires_at = 0;
};

/// HS256 JWT 签发/校验器：access token 与 refresh token 各持有一个实例（密钥不同）
class TokenSigner
{
public:
    TokenSigner(std::string secret, std::string issuer);

    SignedToken sign(const std::string& user_id, int64_t ttl_seconds) const;

    /// 校验签名、issuer 与有效期；任何一项不通过都返回 std::nullopt
    std::optional<TokenPayload> verify(const std::string& token) const;

private:
    std::string m_secret;
    std::string m_issuer;
};

}  // namespace infra
