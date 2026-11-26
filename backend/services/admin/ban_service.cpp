
#include "ban_service.h"

namespace admin::service
{

BanService::BanService(repository::BlacklistRepository& blacklist_repo)
    : m_blacklist_repo(blacklist_repo)
{
}

std::optional<repository::BlacklistEntry> BanService::is_banned(uint64_t     account_id,
                                                                ban::BanType type) const
{
    return m_blacklist_repo.get_detail(account_id, type);
}

}  // namespace admin::service