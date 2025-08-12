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
	
	// 登录
	crow::response Login(const crow::request& req);

	// 注册
	crow::response Register(const crow::request& req);

	// 刷新 token
	crow::response Refresh_token(const crow::request& req);

	// token 黑名单
	crow::response Push_token_to_blackList(const crow::request& req);

	//编辑用户资料
	crow::response edit_user_profile(const crow::request& req);

	//返回用户详细信息
	crow::response get_user_profile(const crow::request& req);

	//修改密码
	crow::response modify_password(const crow::request& req);
	
};