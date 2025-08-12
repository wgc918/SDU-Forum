#include "postController.h"
#include<iostream>
#include"Database.h"
#include"config.h"
#include<vector>
#include"base64.h"
#include"image_proc.h"
#include<nlohmann/json.hpp>
using Json = nlohmann::json;

Auth_utils PostController::auth_utils;


int insert_tag(const std::string& tag)
{
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("insert IGNORE into tag (name) values (?)");
	pstmt->setString(1, tag);
	pstmt->executeUpdate();

	pstmt = db.getConnection()->prepareStatement("update tag set `usage`=`usage`+1 where name=?");
	pstmt->setString(1, tag);
	pstmt->executeUpdate();

	pstmt = db.getConnection()->prepareStatement("select id from tag where name=?");
	pstmt->setString(1, tag);
	auto res = pstmt->executeQuery();

	if (res->next()) 
	{
		return res->getInt("id");
	}
	else {
		throw std::runtime_error("Failed to get tag id after insertion");
	}
}

int insert_post(const int author_id, const std::string& content)
{
	//将内容插入post表
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("insert into post (author_id,content) values (?,?)");
	pstmt->setInt(1, author_id);
	pstmt->setString(2, content);
	pstmt->executeUpdate();

	//返回post_id
	pstmt = db.getConnection()->prepareStatement("select id from post where author_id=? order by id desc limit 1");
	pstmt->setInt(1, author_id);
	auto res = pstmt->executeQuery();

	if (res->next()) 
	{
		return res->getInt("id");
	}
	else 
	{
		throw std::runtime_error("Failed to get post id after insertion");
	}
}

void relevant_tag_post(const int post_id, const int tag_id)
{
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("insert into post_tag (post_id,tag_id) values (?,?)");
	pstmt->setInt(1, post_id);
	pstmt->setInt(2, tag_id);
	pstmt->executeUpdate();
}

crow::response PostController::create_post(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}

	try
	{
		std::string user_id = data["user_id"].s();
		std::string content = data["content"].s();
		auto tag = data["tag"];
		auto images = data["images"];
		
		int author_id = auth_utils.get_account_id(user_id);
		int post_id;
		std::vector<std::string> image_url_list;
		std::vector<int>tag_id;
		

		post_id = insert_post(author_id, content);
		
		if (tag.size()!=0)
		{
			//如果 tag是之前没有的，插入 tag表
			for (auto& t : tag)
			{
				tag_id.push_back(insert_tag(t.s()));
			}
			//插入 tag_post表，将 tag与 post关联
			for (auto& tid : tag_id)
			{
				relevant_tag_post(post_id, tid);
			}
		}

		//存储贴子相关的图片
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		sql::PreparedStatement* pstmt;
		if (images.size() != 0)
		{
			imageToOss oss;
			std::string imageId;
			int i = 0;
			for (auto& img : images)
			{
				imageId = std::to_string(post_id) + std::to_string(i++);//以 post_id和 upload_order为图片命名
				oss.submitImages(base64_decode(img.s()),dirName,imageId);
				image_url_list.push_back(oss.getURL_forever(dirName, imageId));
			}
			//将图片的 url插入 post_image表
			i = 0;
			for (auto& url: image_url_list)
			{
				pstmt = db.getConnection()->prepareStatement("insert into post_image (post_id,image_url,upload_order) values (?,?,?)");
				pstmt->setInt(1, post_id);
				pstmt->setString(2, url);
				pstmt->setInt(3, i++);
				pstmt->executeUpdate();
			}
		}

		//更新用户的总发帖数
		pstmt = db.getConnection()->prepareStatement("update user_profile set published_post_count=published_post_count+1 where account_id=?");
		pstmt->setInt(1, author_id);
		pstmt->executeUpdate();

		return crow::response(200, "post_state: ok");
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::get_post_list(const crow::request& req)
{
	try
	{
		std::string type;
		int start, pageSize;
		try
		{
			type = req.url_params.get("type");
			start = std::stoi(req.url_params.get("start"));
			pageSize = std::stoi(req.url_params.get("pageSize"));
		}
		catch (std::exception& e)
		{
			return crow::response(400, "Invalid Data!");
		}
		

		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement(
			"SELECT DISTINCT p.* "
			"FROM post p "
			"WHERE p.id IN( "
			"SELECT pt.post_id "
			"FROM post_tag pt "
			"JOIN tag t ON pt.tag_id = t.id "
			"WHERE t.name = ? "
			") "
			"ORDER BY p.created_at DESC "
			"LIMIT ? , ?");
		pstmt->setString(1, type);
		pstmt->setInt(2, start);
		pstmt->setInt(3, pageSize);
		auto res=pstmt->executeQuery();
		
		Json ResData = Json::array();
		while(res->next())
		{
			Json info;
			info["id"] = res->getInt("id");
			info["author_id"] = res->getInt("author_id");
			info["user_id"] = PostController::auth_utils.get_user_id(info["author_id"].get<int>());
			info["content"] = res->getString("content");
			info["favorite_count"] = res->getInt("favorite_count");
			info["comment_count"] = res->getInt("comment_count");
			info["created_at"] = res->getString("created_at");

			pstmt = db.getConnection()->prepareStatement("select nickName,avatar_image_url from user_profile where account_id=?");
			pstmt->setInt(1, res->getInt("author_id"));
			auto rres = pstmt->executeQuery();
			if (rres->next())
			{
				info["nickName"] = rres->getString("nickName");
				info["avatar_image_url"] = rres->getString("avatar_image_url");
			}
			else
			{
				info["nickName"] = "";
				info["avatar_image_url"] = "";
			}
			ResData.push_back(info);

		}

		Json response;
		response["state"] = "ok";
		response["result"] = ResData;
		return crow::response(response.dump(4));

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

pair<Json,int> get_post_base_data(int post_id)
{
	Json json;
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("select * from post where id=?");
	pstmt->setInt(1, post_id);
	auto res = pstmt->executeQuery();
	if (res->next())
	{
		json["author_id"] = res->getInt("author_id");
		//json["user_id"]=PostController::auth_utils.get_user_id(json["author_id"].get<int>());
		json["content"] = res->getString("content");
		json["favorite_count"] = res->getInt("favorite_count");
		json["comment_count"] = res->getInt("comment_count");
		json["created_at"] = res->getString("created_at");

		return make_pair(json, json["author_id"]);
	}
	return make_pair(json, -1);
}

Json get_post_tags(int post_id)
{
	Json json=Json::array();
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("select t.name as name from tag t join post_tag pt on t.id=pt.tag_id where pt.post_id=?");
	pstmt->setInt(1, post_id);
	auto res = pstmt->executeQuery();
	while (res->next())
	{
		json.push_back(res->getString("name"));
	}
	return json;
}

Json get_post_image(int post_id)
{
	Json json = Json::array();
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("select image_url from post_image where post_id=?");
	pstmt->setInt(1, post_id);
	auto res = pstmt->executeQuery();
	while (res->next())
	{
		json.push_back(res->getString("image_url"));
	}
	return json;
}

Json get_user_profile(int author_id)
{
	Json json;
	DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
	auto pstmt = db.getConnection()->prepareStatement("select nickName,avatar_image_url from user_profile where account_id=?");
	pstmt->setInt(1, author_id);
	auto res = pstmt->executeQuery();
	if (res->next())
	{
		json["nickName"] = res->getString("nickName");
		json["avatar_image_url"] = res->getString("avatar_image_url");
	}
	return json;
}

crow::response PostController::get_post_details(const crow::request& req)
{
	try
	{
		int post_id;
		std::string user_id;
		try
		{
			post_id = std::stoi(req.url_params.get("post_id"));
			user_id = req.url_params.get("user_id");
		}
		catch (std::exception& e)
		{
			return crow::response(400, "Invalid Data!");
		}

		Json resData;

		//先获取贴子的基本信息
		auto tt = get_post_base_data(post_id);
		tt.first["user_id"] = user_id;
		resData["base_data"] = tt.first;
		//获取标签
		resData["tags"] = get_post_tags(post_id);
		//获取图片
		resData["images"] = get_post_image(post_id);
		//获取用户信息
		resData["user_profile"] = get_user_profile(tt.second);

		//获取用户是否点赞、收藏贴子信息
		int account_id = auth_utils.get_account_id(user_id);
		//std::cout << account_id << std::endl;
		//std::cout << post_id << std::endl;
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select * from user_like_post where account_id=? and post_id=?");
		pstmt->setInt(1, account_id);
		pstmt->setInt(2, post_id);
		auto res = pstmt->executeQuery();
		if (res->next())
		{
			resData["like_state"] = true;
		}
		else
		{
			resData["like_state"] = false;
		}

		pstmt = db.getConnection()->prepareStatement("select * from user_favorite_post where account_id=? and post_id=?");
		pstmt->setInt(1, account_id);
		pstmt->setInt(2, post_id);
		res = pstmt->executeQuery();
		if (res->next())
		{
			resData["favorite_state"] = true;
		}
		else
		{
			resData["favorite_state"] = false;
		}
		return crow::response(resData.dump(4));

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
	
}

crow::response PostController::get_post_user_list(const crow::request& req)
{
	try
	{
		string  user_id;
		try
		{
			user_id =req.url_params.get("user_id");
		}
		catch (std::exception& e)
		{
			return crow::response(400, "Invalid Data!");
		}

		Json resData=Json::array();
		int author_id = auth_utils.get_account_id(user_id);

		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select * from post where author_id =? order by id desc");
		pstmt->setInt(1, author_id);
		auto res = pstmt->executeQuery();
		while (res->next())
		{
			Json info;
			info["id"] = res->getInt("id");
			info["author_id"] = res->getInt("author_id");
			info["user_id"] = user_id;
			info["content"] = res->getString("content");
			info["favorite_count"] = res->getInt("favorite_count");
			info["comment_count"] = res->getInt("comment_count");
			info["created_at"] = res->getString("created_at");

			resData.push_back(info);
		}

		return crow::response(resData.dump(4));

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::delete_post(const crow::request& req)
{
	try
	{
		auto data = crow::json::load(req.body);
		int post_id = data["post_id"].i();
		int account_id = auth_utils.get_account_id(data["user_id"].s());
		 
		//删除服务器图片数据
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select upload_order from post_image where post_id=?");
		pstmt->setInt(1, post_id);
		auto res=pstmt->executeQuery();
		imageToOss oss;
		while (res->next())
		{
			std::string image_id = std::to_string(post_id) + std::to_string(res->getInt("upload_order"));
			oss.deleteImages(dirName, image_id);
		}

		//删除数据库数据
		pstmt = db.getConnection()->prepareStatement("delete from post where id=?");
		pstmt->setInt(1, post_id);
		int affected_rows = pstmt->executeUpdate();

		if (affected_rows > 0) 
		{
			//更新用户的总发帖数
			pstmt = db.getConnection()->prepareStatement("update user_profile set published_post_count=GREATEST(published_post_count-1,0) where account_id=?");
			pstmt->setInt(1, account_id);
			pstmt->executeUpdate();
			return crow::response(200, "Post deleted successfully");
		}
		else {
			return crow::response(404, "Post not found");
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::get_post_favorite_list(const crow::request& req)
{
	try
	{
		int account_id;
		try
		{
			account_id = auth_utils.get_account_id(req.url_params.get("user_id"));
		}
		catch (std::exception&e)
		{
			return crow::response(400, "Invalid Data!");
		}
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement(
			"SELECT "
			"p.id AS post_id, "                     
			"p.author_id AS author_id, "
			"p.content AS content, "
			"p.favorite_count AS favorite_count, "
			"p.comment_count AS comment_count, "
			"p.created_at AS post_created_at, "      // 重命名
			"ufp.created_at AS favorite_created_at " // 区分两个created_at
			"FROM post p "
			"JOIN user_favorite_post ufp ON ufp.post_id = p.id "
			"WHERE ufp.account_id=? order by ufp.created_at desc"
		);
		pstmt->setInt(1, account_id);
		auto res = pstmt->executeQuery();

		Json resData = Json::array();
		while (res->next())
		{
			Json info;
			info["id"] = res->getInt("post_id");                  
			info["author_id"] = res->getInt("author_id");
			info["user_id"] = PostController::auth_utils.get_user_id(info["author_id"].get<int>());
			info["content"] = res->getString("content");
			info["favorite_count"] = res->getInt("favorite_count");
			info["comment_count"] = res->getInt("comment_count");
			info["created_at"] = res->getString("post_created_at");    
			info["add_at"] = res->getString("favorite_created_at");

			pstmt = db.getConnection()->prepareStatement("select nickName,avatar_image_url from user_profile where account_id=?");
			pstmt->setInt(1, info["author_id"].get<int>());
			auto rres = pstmt->executeQuery();
			if (rres->next())
			{
				info["nickName"] = rres->getString("nickName");
				info["avatar_image_url"] = rres->getString("avatar_image_url");
			}
			else
			{
				info["nickName"] = "";
				info["avatar_image_url"] = "";
			}
			resData.push_back(info);
		}

		Json response;
		response["state"] = "ok";
		response["result"] = resData;
		return crow::response(response.dump(4));
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::add_post_favorite(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	try
	{
		std::string user_id = data["user_id"].s();
		int post_id = data["post_id"].i();
		int account_id = auth_utils.get_account_id(user_id);

		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("insert into user_favorite_post (account_id,post_id) values (?,?)");
		pstmt->setInt(1, account_id);
		pstmt->setInt(2, post_id);
		int affected_rows = pstmt->executeUpdate();

		if (affected_rows > 0)
		{
			//更新用户的总的收藏数
			pstmt = db.getConnection()->prepareStatement("update user_profile set favorite_post_count=favorite_post_count+1 where account_id=?");
			pstmt->setInt(1, account_id);
			pstmt->executeUpdate();
			return crow::response(200, crow::json::wvalue({ {"state",true}, { "info","Post add to favorite_post successfully" } }));
		}
		else 
		{
			return crow::response(200, crow::json::wvalue({ {"state",false}, { "info","Failed to add post to favorites" } }));
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::sub_post_favorite(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	try
	{
		std::string user_id = data["user_id"].s();
		int post_id = data["post_id"].i();
		int account_id = auth_utils.get_account_id(user_id);

		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("delete from user_favorite_post where account_id=? and post_id=?");
		pstmt->setInt(1, account_id);
		pstmt->setInt(2, post_id);
		pstmt->executeUpdate();
		//更新用户的总的收藏数
		pstmt = db.getConnection()->prepareStatement("update user_profile set favorite_post_count=GREATEST(favorite_post_count-1,0) where account_id=?");
		pstmt->setInt(1, account_id);
		pstmt->executeUpdate();

		return crow::response(200, crow::json::wvalue({ {"state",true}, { "info","Remove post from favorite_post successfully" } }));
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::like(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	try
	{
		int post_id = data["post_id"].i();
		int account_id = auth_utils.get_account_id(data["user_id"].s());
		int author_id = auth_utils.get_account_id(data["author_user_id"].s());
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("update post set favorite_count=favorite_count+1 where id=?");
		pstmt->setInt(1, post_id);
		pstmt->executeUpdate();
		
		//给user_like_post表中插入记录
		pstmt=db.getConnection()->prepareStatement("insert into user_like_post (account_id,post_id) values (?,?)");
		pstmt->setInt(1, account_id);
		pstmt->setInt(2, post_id);
		pstmt->executeUpdate();

		//更新对应贴子对应的用户的总的点赞数
		pstmt = db.getConnection()->prepareStatement("update user_profile set liked_count=liked_count+1 where account_id=?");
		pstmt->setInt(1, author_id);
		pstmt->executeUpdate();

		pstmt = db.getConnection()->prepareStatement("select favorite_count from post where id=?");
		pstmt->setInt(1, post_id);
		auto res=pstmt->executeQuery();
		if (res->next())
		{
			crow::json::wvalue response;
			response["state"] = true;
			response["favorite_count"] = res->getInt("favorite_count");
			return crow::response(response);
		}
		crow::json::wvalue response;
		response["state"] = false;
		return crow::response(response);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response PostController::unlike(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	try
	{
		int post_id = data["post_id"].i();
		int account_id = auth_utils.get_account_id(data["user_id"].s());
		int author_id = auth_utils.get_account_id(data["author_user_id"].s());
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("update post set favorite_count=GREATEST(favorite_count-1,0) where id=?");
		pstmt->setInt(1, post_id);
		pstmt->executeUpdate();

		pstmt = db.getConnection()->prepareStatement("delete from user_like_post where account_id=? and post_id=?");
		pstmt->setInt(1, account_id);
		pstmt->setInt(2, post_id);
		pstmt->executeUpdate();

		//更新对应贴子对应的用户的总的点赞数
		pstmt = db.getConnection()->prepareStatement("update user_profile set liked_count=GREATEST(liked_count-1,0) where account_id=?");
		pstmt->setInt(1, author_id);
		pstmt->executeUpdate();

		pstmt = db.getConnection()->prepareStatement("select favorite_count from post where id=?");
		pstmt->setInt(1, post_id);
		auto res = pstmt->executeQuery();
		if (res->next())
		{
			crow::json::wvalue response;
			response["state"] = true;
			response["favorite_count"] = res->getInt("favorite_count");
			return crow::response(response);
		}
		crow::json::wvalue response;
		response["state"] = false;
		return crow::response(response);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}
