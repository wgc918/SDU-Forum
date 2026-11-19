#include "auth_service.h"

#include "auth/token_signer.h"
#include "common/errors.h"
#include "log.h"
#include "repositories/admin_account_repository.h"
#include "repositories/token_repository.h"
#include "security/password_hasher.h"
#include "utils/utils.h"

namespace admin::service
{
AdminAuthService::AdminAuthService(repository::AdminAccountRepository& account_repo,
                                   repository::TokenRepository&        token_repo,
                                   const infra::JwtConfig&             jwt_config)
    : m_admin_account_repo(account_repo),
      m_token_repo(token_repo),
      m_access_signer(jwt_config.admin_access_secret, jwt_config.issuer),
      m_refresh_signer(jwt_config.admin_refresh_secret, jwt_config.issuer),
      m_access_ttl_seconds(jwt_config.access_token_ttl_seconds()),
      m_refresh_ttl_seconds(jwt_config.refresh_token_ttl_seconds())
{
}

/// @throws common::AppException
AdminLoginResult AdminAuthService::login(const std::string& admin_id, const std::string& password)
{
    auto account = m_admin_account_repo.find_admin_account_by_admin_id(admin_id);
    if (!(account && infra::verify_password(
                         password, {account->password_hash, account->salt, account->iterations})))
    {
        LOGW("login failed: admin_id=%s", admin_id.c_str());
        throw common::AppException(common::ErrorCode::InvalidCredentials,
                                   "admin id or password is incorrect");
    }

    LOGI("login succeeded: admin id=%s", admin_id.c_str());

    infra::SignedToken access =
        m_access_signer.sign(admin_id, infra::kAdminRole, m_access_ttl_seconds);
    infra::SignedToken refresh =
        m_refresh_signer.sign(admin_id, infra::kAdminRole, m_refresh_ttl_seconds);

    AdminLoginResult result;
    result.admin_id          = admin_id;
    result.access_token      = access.value;
    result.access_token_exp  = access.expires_at;
    result.refresh_token     = refresh.value;
    result.refresh_token_exp = refresh.expires_at;
    return result;
}

/// @throws common::AppException
void AdminAuthService::logout(const std::string& refresh_token, const std::string access_token)
{
    bool has_token = m_token_repo.find_refresh_token(
        utils::hmac_sha256_base64url(m_token_repo.hash_secret(), refresh_token));
    if (has_token)
        throw common::AppException(common::ErrorCode::Unauthorized, "old refresh token");
    auto payload = m_refresh_signer.verify(refresh_token);
    if (!payload)
        throw common::AppException(common::ErrorCode::Unauthorized,
                                   "invalid or expired refresh token");

    repository::TokenRecord refresh_token_recd;
    repository::TokenRecord access_token_recd;
    refresh_token_recd.token_hash =
        utils::hmac_sha256_base64url(m_token_repo.hash_secret(), refresh_token);
    refresh_token_recd.exp_at = payload->expires_at;
    access_token_recd.token_hash =
        utils::hmac_sha256_base64url(m_token_repo.hash_secret(), access_token);
    access_token_recd.exp_at =
        0;  // access_token 不存数据库供审计，仅缓存供吊销判断，缓存ttl默认为原ttl

    if (!m_token_repo.insert_refresh_token(refresh_token_recd) ||
        !m_token_repo.insert_access_token(access_token_recd))
        throw common::AppException(common::ErrorCode::DatabaseError, "failed to revoke token");
}

/// @throws common::AppException
/// @param refreshing 是否更新refresh_token
AdminLoginResult AdminAuthService::refresh_token(const std::string& refresh_token,
                                                 const std::string& access_token, bool refreshing)
{
    // 先判断 refresh_token 是否已被吊销(已在中间件判断过了)
    // auto token = m_token_repo.find_refresh_token(
    //     utils::hmac_sha256_base64url(m_token_repo.hash_secret(), refresh_token));
    // if (token)
    //     throw common::AppException(common::ErrorCode::Unauthorized, "old refresh token");

    // 这里验证其实是为了解析token
    auto payload = m_refresh_signer.verify(refresh_token);
    if (!payload)
        throw common::AppException(common::ErrorCode::Unauthorized,
                                   "invalid or expired refresh token");

    AdminLoginResult result;
    if (refreshing)
    {
        infra::SignedToken new_refresh_token =
            m_refresh_signer.sign(payload->sub, infra::kAdminRole, m_refresh_ttl_seconds);
        repository::TokenRecord refresh_token_record;
        refresh_token_record.token_hash =
            utils::hmac_sha256_base64url(m_token_repo.hash_secret(), refresh_token);
        refresh_token_record.exp_at = new_refresh_token.expires_at;
        // 吊销旧token
        m_token_repo.insert_refresh_token(refresh_token_record);

        result.admin_id          = payload->sub;
        result.refresh_token     = new_refresh_token.value;
        result.refresh_token_exp = new_refresh_token.expires_at;
    }

    infra::SignedToken new_access_token =
        m_access_signer.sign(payload->sub, infra::kAdminRole, m_access_ttl_seconds);
    repository::TokenRecord access_token_record;
    access_token_record.token_hash =
        utils::hmac_sha256_base64url(m_token_repo.hash_secret(), access_token);
    access_token_record.exp_at = new_access_token.expires_at;
    // 吊销旧token
    m_token_repo.insert_access_token(access_token_record);

    result.access_token     = new_access_token.value;
    result.access_token_exp = new_access_token.expires_at;
    return result;
}
}  // namespace admin::service