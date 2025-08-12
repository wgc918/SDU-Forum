#include "auth_utils.h"
#include"Database.h"
#include"config.h"

bool Auth_utils::is_user_registered(const std::string& user_id)
{
    DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
    auto pstmt = db.getConnection()->prepareStatement("select * from account where user_id=?");
    pstmt->setString(1, user_id);
    auto res=pstmt->executeQuery();
    if (res->next())
    {
        return true;
    }
    

    return false;
}

bool Auth_utils::validate_credentials(const std::string& username, const std::string& password)
{
    return false;
}

int Auth_utils::get_account_id(const std::string& user_id)
{
    DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
    auto pstmt = db.getConnection()->prepareStatement("select id from account where user_id=?");
    pstmt->setString(1, user_id);
    auto res = pstmt->executeQuery();
    if (res->next())
    {
        return res->getInt("id");
    }
    return -1;
}

std::string Auth_utils::get_nickName(const std::string user_id)
{
    int account = get_account_id(user_id);
    DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
    auto pstmt = db.getConnection()->prepareStatement("select nickName from user_profile where account_id=?");
    pstmt->setInt(1, account);
    auto res = pstmt->executeQuery();
    if (res->next())
    {
        return res->getString("nickName");
    }
    return std::string();
}

std::string Auth_utils::get_user_id(int account_id)
{
    DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
    auto pstmt = db.getConnection()->prepareStatement("select user_id from account where id=?");
    pstmt->setInt(1, account_id);
    auto res = pstmt->executeQuery();
    if (res->next())
    {
        return res->getString("user_id");
    }
    return std::string();
}
