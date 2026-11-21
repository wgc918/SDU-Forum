#include "repositories/blacklist_repository.h"

#include <jdbc/cppconn/exception.h>

#include <memory>
#include <utility>

#include "common/errors.h"
#include "conn-pool/ConnectionPool.h"
#include "conn-pool/pooledConnGuard.h"
#include "log.h"

namespace repository
{

BlacklistRepository::BlacklistRepository(IDbConnectionProvider& db_provider,
                                         std::string            blacklist_table)
    : m_db_provider(db_provider), m_blacklist_table(std::move(blacklist_table))
{
}

}  // namespace repository
