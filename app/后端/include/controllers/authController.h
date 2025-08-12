#pragma once
#include<iostream>
#include<string>
#include<crow.h>
#include"auth_utils.h"

class AuthController
{
private:
	Auth_utils auth_utils;
	std::string avatar_dirName = "avatar_images/";
	std::string background_dirName = "background_images/";
	
public:
	explicit AuthController(){}
	
	// ��¼
	crow::response Login(const crow::request& req);

	// ע��
	crow::response Register(const crow::request& req);

	// ˢ�� token
	crow::response Refresh_token(const crow::request& req);

	// token ������
	crow::response Push_token_to_blackList(const crow::request& req);

	//�༭�û�����
	crow::response edit_user_profile(const crow::request& req);

	//�����û���ϸ��Ϣ
	crow::response get_user_profile(const crow::request& req);

	//�޸�����
	crow::response modify_password(const crow::request& req);
	
};