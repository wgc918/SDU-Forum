#include "feedbackController.h"
#include"Database.h"
#include"config.h"
#include<nlohmann/json.hpp>

using Json = nlohmann::json;

crow::response FeedbackController::getFeedback(const crow::request& req)
{
    try
    {
        int start, pageSize;
        try
        {
            start = std::stoi(req.url_params.get("start"));
            pageSize = std::stoi(req.url_params.get("pageSize"));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return crow::response(400, "Invalid Data");
        }
        DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
        auto pstmt = db.getConnection()->prepareStatement("select * from feedback order by id desc limit ?,?");
        pstmt->setInt(1, start);
        pstmt->setInt(2, pageSize);
        auto res = pstmt->executeQuery();
        Json data = Json::array();
        while (res->next())
        {
            Json info;
            info["user_id"] = res->getString("user_id");
            info["feedback"] = res->getString("feedback");

            data.push_back(info);
        }
        return crow::response(data.dump(4));

    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return crow::response(500, "Internal Server Error");
    }
}

crow::response FeedbackController::saveFeedback(const crow::request& req)
{
    try
    {
        auto data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Invalid Json Data!");
        }

        std::string user_id = data["user_id"].s();
        std::string feedback = data["feedback"].s();

        DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
        auto pstmt=db.getConnection()->prepareStatement("insert into feedback (user_id,feedback) values (?,?)");
        pstmt->setString(1, user_id);
        pstmt->setString(2, feedback);
        pstmt->executeUpdate();

        return crow::response(200);

    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return crow::response(500, "Internal Server Error");
    }
}
