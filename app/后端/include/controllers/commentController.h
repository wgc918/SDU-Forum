#pragma once
#include<string>
#include<vector>
#include<iostream>
#include"config.h"
#include<crow.h>
#include"auth_utils.h"
#include<nlohmann/json.hpp>
using Json = nlohmann::json;


class Comment
{
public:
	Comment(int id,int post_id);
	Comment(int post_id, const std::string& nickName,int account_id, int father, const std::string& to_nickName, const std::string& text);
	Comment(int post_id);

	bool createTable();

	bool saveComment();

	Json getFirstComment();

	Json getChildrenComment(int father);

	int like();

	int unlike();

private:
	int post_id;	// 贴子id 评论表的唯一标识
	int father;		// 父评论id
	int id;	
	int account_id;
	std::string tableName;	// 评论表的表名
	std::string to_nickName;
	std::string nickName;
	std::string text;

	Auth_utils auth_utils;
};


class CommentController
{
private:
	Auth_utils auth_utils;
public:
	explicit CommentController() {}

	crow::response saveComment(const crow::request& req);

	crow::response getComment(const crow::request& req);

	crow::response like(const crow::request& req);

	crow::response unlike(const crow::request& req);
};