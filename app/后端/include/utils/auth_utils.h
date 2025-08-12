#pragma once
#include<iostream>
#include<string>


class Auth_utils
{
public:
	Auth_utils(){}
     

    // ����û��Ƿ���ע��
    bool is_user_registered(const std::string& user_id);

    // ��֤�û�ƾ֤�����ڵ�¼��
    bool validate_credentials(const std::string& username, const std::string& password);

    
    int get_account_id(const std::string& user_id);

    std::string get_nickName(const std::string user_id);

    std::string get_user_id(int account_id);

    // ��ѡ����֤���븴�Ӷ�
    static bool validate_password_strength(const std::string& password);
};
