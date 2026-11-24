#pragma once

#include <cstdint>
#include <optional>

#include "common/ban_enums.h"
#include "repositories/blacklist_repository.h"

/// 因为登录接口没有经过中间件校验 all(全站封禁)
/// 所以需要单独校验all
/// Login仅限制“新登录”，不主动失效已有会话；已有 token 在过期前仍可继续使用，
/// 因此 Login 封禁必须在 refresh token 接口单独校验，防止旧会话通过 refresh 续命。
/// All：账号级全面封禁,通过在认证中间中校验该封禁类型，以实现立即性的封禁：本质是限制除login外的所有下一次请求

namespace admin::service
{
class BanService
{
public:
    BanService(repository::BlacklistRepository& blacklist_repo);

    /// 查询用户在某维度是否被禁（用于评论/发帖等拦截点）
    std::optional<repository::BlacklistEntry> is_banned(uint64_t     account_id,
                                                        ban::BanType type) const;

private:
    repository::BlacklistRepository& m_blacklist_repo;
};
}  // namespace admin::service