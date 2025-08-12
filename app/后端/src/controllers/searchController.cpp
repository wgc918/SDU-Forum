#include "searchController.h"
#include"Database.h"
#include"config.h"
#include<crow.h>
#include<nlohmann/json.hpp>

using Json = nlohmann::json;


crow::response SearchController::allTextSearch(const crow::request& req)
{
    try
    {
        std::string key;
		int start, pageSize;
        try
        {
            key = req.url_params.get("key");
			start = std::stoi(req.url_params.get("start"));
			pageSize = std::stoi(req.url_params.get("pageSize"));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }

        DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
        auto pstmt = db.getConnection()->prepareStatement(
            "SELECT * FROM post WHERE content LIKE ? LIMIT ? , ?");
        pstmt->setString(1, "%" + key + "%");
		pstmt->setInt(2, start);
		pstmt->setInt(3, pageSize);
        auto res = pstmt->executeQuery();

        Json resData = Json::array();
        while (res->next())
        {
            Json info;
            info["id"] = res->getInt("id");
            info["author_id"] = res->getInt("author_id");
			info["user_id"] = auth_utils.get_user_id(info["author_id"].get<int>());
            info["content"] = res->getString("content");
            info["favorite_count"] = res->getInt("favorite_count");
            info["comment_count"] = res->getInt("comment_count");
            info["created_at"] = res->getString("created_at");

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
    }
}

crow::response SearchController::tagSearch(const crow::request& req)
{
	try
	{
		std::string tag;
		int start, pageSize;
		try
		{
			tag = req.url_params.get("tag");
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
			"ORDER BY p.id DESC "
			"LIMIT ? , ?");
		pstmt->setString(1, tag);
		pstmt->setInt(2, start);
		pstmt->setInt(3, pageSize);
		auto res = pstmt->executeQuery();
		
		pstmt = db.getConnection()->prepareStatement("update tag set `usage`=`usage`+1 where name=?");
		pstmt->setString(1, tag);
		pstmt->executeUpdate();

		Json ResData = Json::array();
		while (res->next())
		{
			Json info;
			info["id"] = res->getInt("id");
			info["author_id"] = res->getInt("author_id");
			info["user_id"] = auth_utils.get_user_id(info["author_id"].get<int>());
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

crow::response SearchController::top10_post(const crow::request& req)
{
	try
	{
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, false);
		auto res = db.getStatement()->executeQuery("select * from post order by (favorite_count * 0.6 + comment_count * 0.4) desc limit 0,10");
		Json ResData = Json::array();
		while (res->next())
		{
			Json info;
			info["id"] = res->getInt("id");
			info["author_id"] = res->getInt("author_id");
			info["user_id"] = auth_utils.get_user_id(info["author_id"].get<int>());
			info["content"] = res->getString("content");
			info["favorite_count"] = res->getInt("favorite_count");
			info["comment_count"] = res->getInt("comment_count");
			info["created_at"] = res->getString("created_at");

			auto pstmt = db.getConnection()->prepareStatement("select nickName,avatar_image_url from user_profile where account_id=?");
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

crow::response SearchController::hotTag(const crow::request& req)
{
	try
	{
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, false);
		auto res = db.getStatement()->executeQuery("select name from tag order by `usage` desc limit 0,6");
		Json data = Json::array();
		while (res->next())
		{
			data.push_back(res->getString("name"));
		}

		Json resData;
		resData["state"] = "ok";
		resData["result"] = data;
		return crow::response(resData.dump(4));
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}
