#pragma once

#include <drogon/HttpAppFramework.h>

#include "../module.h"
#include "controllers/admin/user_controller.h"
#include "repositories/blacklist_repository.h"
#include "services/admin/user_service.h"

namespace app::modules
{
/// @brief 管理员用户管理模块：封禁、解封与用户状态查询
class AdminUserModule : public Module
{
public:
    /// @brief 构造管理员用户管理模块
    /// @param blacklist_repo 共享黑名单仓库
    explicit AdminUserModule(repository::BlacklistRepository& blacklist_repo);
    void register_routes(drogon::HttpAppFramework& app) override;

private:
    admin::service::AdminUserService       m_service;
    admin::controller::AdminUserController m_controller;
};
}  // namespace app::modules
