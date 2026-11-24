#pragma once

#include <cstdint>
#include <optional>

#include "common/ban_enums.h"
#include "repositories/blacklist_repository.h"

namespace admin::service
{
class BanService
{
public:
    BanService(repository::BlacklistRepository& blacklist_repo);

    /// 查询用户在某维度是否被禁（用于评论/发帖等拦截点）
    std::optional<repository::BlacklistEntry> is_banned(uint64_t account_id, ban::BanType type) const;

private:
    repository::BlacklistRepository& m_blacklist_repo;
};
}  // namespace admin::service