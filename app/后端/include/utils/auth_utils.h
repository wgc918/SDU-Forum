#pragma once
#include<iostream>
#include<string>


class Auth_utils
{
public:
	Auth_utils(){}
     

    // 检查用户是否已注册
    bool is_user_registered(const std::string& user_id);

    // 验证用户凭证（用于登录）
    bool validate_credentials(const std::string& username, const std::string& password);

    
    int get_account_id(const std::string& user_id);

    std::string get_nickName(const std::string user_id);

    std::string get_user_id(int account_id);

    // 可选：验证密码复杂度
    static bool validate_password_strength(const std::string& password);
};
