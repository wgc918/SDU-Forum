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

// 放在这里不是很好，但是先放这了
constexpr char kUserRole[]  = "sduforum_user";
constexpr char kAdminRole[] = "sduforum_admin";

struct TokenPayload
{
    std::string sub;
    std::string issuer;
    std::string role;
    int64_t     issued_at  = 0;
    int64_t     expires_at = 0;
};

/// HS256 JWT 签发/校验器：access token 与 refresh token 各持有一个实例（密钥不同）
class TokenSigner
{
public:
    TokenSigner(std::string secret, std::string issuer);
    // 后续引入后台管理api时，要在token中添加role进行区分权限
    SignedToken sign(const std::string& sub, const std::string& role, int64_t ttl_seconds) const;

    /// 校验签名、issuer 与有效期；任何一项不通过都返回 std::nullopt
    std::optional<TokenPayload> verify(const std::string& token) const;

    /// 考虑新增解析token和查询token ttl 的接口

private:
    std::string m_secret;
    std::string m_issuer;
};

}  // namespace infra
