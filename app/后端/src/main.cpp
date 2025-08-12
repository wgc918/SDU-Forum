#include<iostream>
#include<sstream>
#include<crow.h>
#include<jdbc/cppconn/statement.h>
#include<jdbc/cppconn/resultset.h>
#include<jdbc/cppconn/exception.h>
#include<jdbc/mysql_driver.h>
#include<jdbc/mysql_connection.h>
#include<jdbc/cppconn/prepared_statement.h>
#include"Database.h";
#include"config.h"
#include"authController.h"
#include<jwt-cpp/jwt.h>
#include"auth_middleware.h"
#include"image_proc.h"
#include<vector>
#include"postController.h"
#include"searchController.h"
#include"commentController.h"
#include"insert_emoji_to_db.h"
#include"feedbackController.h"
#include "Logger.h"



int main()
{
	
	// 设置MySQL日志处理器
	//MySQLHandler mysql_handler(HOSTNAME, USERNAME_USERDB, PASSWORD, DBNAME_MANAGE);
	//crow::logger::setHandler(&mysql_handler);
	//crow::logger::setLogLevel(crow::LogLevel::Info); // 设置日志级别
	 // 初始化MySQL日志处理器
	/*try {
		MySQLHandler mysql_handler(HOSTNAME, USERNAME_USERDB, PASSWORD, DBNAME_MANAGE);
		crow::logger::setHandler(&mysql_handler);
		crow::logger::setLogLevel(crow::LogLevel::Info);
	}
	catch (std::exception& e) {
		std::cerr << "Failed to initialize MySQL logger: " << e.what() << std::endl;
		return 1;
	}*/

	crow::SimpleApp app;

	AuthController auth;
	CROW_ROUTE(app, "/sdu_forum/api/user/login")
		.methods("POST"_method)
		([&auth](const crow::request& req)
			{
				return auth.Login(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/user/register")
		.methods("POST"_method)
		([&auth](const crow::request& req)
			{
				return auth.Register(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/user/refresh_token")
		.methods("POST"_method)
		([&auth](const crow::request& req)
			{
				return auth.Refresh_token(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/user/push_token_to_blackList")
		.methods("POST"_method)
		([&auth](const crow::request& req)
			{
				return auth.Push_token_to_blackList(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/get_user_profile")
		.methods("GET"_method)
		([&auth](const crow::request& req)
			{
				return auth.get_user_profile(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/edit_user_profile")
		.methods("POST"_method)
		([&auth](const crow::request& req)
			{
				return auth.edit_user_profile(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/modify_password")
		.methods("POST"_method)
		([&auth](const crow::request& req)
			{
				return auth.modify_password(req);
			});

	PostController ptc;
	CROW_ROUTE(app, "/sdu_forum/api/post/create")
		.methods("POST"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.create_post(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/get_post_list")
		.methods("GET"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.get_post_list(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/get_post_details")
		.methods("GET"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.get_post_details(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/get_post_user_list")
		.methods("GET"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.get_post_user_list(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/user/get_post_favorite_list")
		.methods("GET"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.get_post_favorite_list(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/user/delete_post")
		.methods("POST"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.delete_post(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/post/add_post_favorite")
		.methods("POST"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.add_post_favorite(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/post/sub_post_favorite")
		.methods("POST"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.sub_post_favorite(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/post/like")
		.methods("POST"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.like(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/post/unlike")
		.methods("POST"_method)
		([&ptc](const crow::request& req)
			{
				return ptc.unlike(req);
			});

	/*CROW_ROUTE(app, "/test_post")
		.methods("POST"_method)
		([](const crow::request& req)
			{
				return test_post(req);
			});
	CROW_ROUTE(app, "/test_get")
		.methods("POST"_method)
		([](const crow::request& req)
			{
				return test_get(req);
			});*/

	CROW_ROUTE(app, "/sdu_forum/api/system/statu")
		.methods("GET"_method)
		([&](const crow::request& req)
			{
				crow::response res;
				
				res.set_header("Access-Control-Allow-Origin", "*"); // 设置允许跨域请求的源
				res.set_header("Content-Type", "application/json");
				return res;
			});


	CommentController cc;
	CROW_ROUTE(app, "/sdu_forum/api/comment/getComment")
		.methods("GET"_method)
		([&cc](const crow::request& req)
			{
				return cc.getComment(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/comment/saveComment")
		.methods("POST"_method)
		([&cc](const crow::request& req)
			{
				return cc.saveComment(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/comment/likeComment")
		.methods("POST"_method)
		([&cc](const crow::request& req)
			{
				return cc.like(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/comment/unlikeComment")
		.methods("POST"_method)
		([&cc](const crow::request& req)
			{
				return cc.unlike(req);
			});
	

	SearchController sc;
	CROW_ROUTE(app, "/sdu_forum/api/search/all_text")
		.methods("GET"_method)
		([&sc](const crow::request& req)
			{
				return sc.allTextSearch(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/search/tag")
		.methods("GET"_method)
		([&sc](const crow::request& req)
			{
				return sc.tagSearch(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/search/top10")
		.methods("GET"_method)
		([&sc](const crow::request& req)
			{
				return sc.top10_post(req);
			});

	CROW_ROUTE(app, "/sdu_forum/api/search/get_hot_tag")
		.methods("GET"_method)
		([&sc](const crow::request& req)
			{
				return sc.hotTag(req);
			});

	FeedbackController fdc;
	CROW_ROUTE(app, "/sdu_forum/api/feedback/get")
		.methods("GET"_method)
		([&fdc](const crow::request& req)
			{
				
				return fdc.getFeedback(req);
			});
	CROW_ROUTE(app, "/sdu_forum/api/feedback/submit")
		.methods("POST"_method)
		([&fdc](const crow::request& req)
			{
				return fdc.saveFeedback(req);
			});



	app.port(8080).multithreaded().run();

	
    
	return 0;
}



