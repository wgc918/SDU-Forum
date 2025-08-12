#pragma once
#include<crow.h>
#include"auth_utils.h"
#include<string>


class PostController
{
private:
	std::string dirName = "post_images/";
public:
	explicit PostController(){}
	static Auth_utils auth_utils;

	//发布贴子
	crow::response create_post(const crow::request& req);

	//按页返回贴子简要信息
	crow::response get_post_list(const crow::request& req);

	//返回贴子详情
	crow::response get_post_details(const crow::request& req);

	//返回用户发布的所有贴子简要信息
	crow::response get_post_user_list(const crow::request& req);

	//删除贴子
	crow::response delete_post(const crow::request& req);

	//返回用户收藏的所有贴子的简要信息
	crow::response get_post_favorite_list(const crow::request& req);

	//收藏贴子
	crow::response add_post_favorite(const crow::request& req);

	//取消收藏
	crow::response sub_post_favorite(const crow::request& req);

	//点赞贴子
	crow::response like(const crow::request& req);

	//取消点赞
	crow::response unlike(const crow::request& req);

};