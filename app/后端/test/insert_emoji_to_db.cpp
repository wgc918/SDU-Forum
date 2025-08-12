#include"insert_emoji_to_db.h"



crow::response test_post(const crow::request&req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	try
	{
		std::string text = data["text"].s();
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("insert into test (text) values (?)");
		pstmt->setString(1, text);
		int affected_rows=pstmt->executeUpdate();
		if (affected_rows > 0)
		{
			return crow::response("ok");
		}
		else
		{
			return crow::response("fail");
		}

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}


crow::response test_get(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	try
	{
		int id = data["id"].i();
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select * from test where id=?");
		pstmt->setInt(1, id);
		auto res = pstmt->executeQuery();
		if (res->next())
		{
			crow::json::wvalue response;
			response["text"] = res->getString("text");
			return crow::response(response);
		}
		return crow::response("fail");

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}