#include "commentController.h"
#include"Database.h"

Comment::Comment(int id, int post_id)
{
    this->id = id;
    this->post_id = post_id;
    tableName = "comment_" + std::to_string(post_id);
}

Comment::Comment( int post_id, const std::string& nickName,int account_id, int father, const std::string& to_nickName, const std::string& text)
{
    this->post_id = post_id;
    this->nickName = nickName;
    this->father = father;
    this->to_nickName = to_nickName;
    this->text = text;
    this->account_id = account_id;

    tableName = "comment_" + std::to_string(post_id);
}

Comment::Comment( int post_id)
{
    this->post_id = post_id;
    tableName = "comment_" + std::to_string(post_id);
}

bool Comment::createTable()
{
    try
    {
        DataBase db(HOSTNAME, USERNAME_COMMENT, DBNAME_COMMENT, PASSWORD, false);
        std::string isExist = "show tables like '" + tableName + "'";
        std::string create = "create table " + tableName + "("
            "id int primary key auto_increment,"
            "nickName varchar(50),"
            "account_id int,"
            "releaseTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
            "father int,"
            "like_num int DEFAULT 0,"
            "to_nickName varchar(50),"
            "text text)";
        auto res = db.getStatement()->executeQuery(isExist);
        if (!res->next())
        {
            db.getStatement()->execute(create);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Comment::saveComment()
{
    try
    {
        DataBase db(HOSTNAME, USERNAME_COMMENT, DBNAME_COMMENT, PASSWORD, true);
        auto pstmt = db.getConnection()->prepareStatement("insert into " + tableName + " (nickName,account_id,father,to_nickName,text) values (?,?,?,?,?)");
        pstmt->setString(1, nickName);
        pstmt->setInt(2, account_id);
        pstmt->setInt(3, father);
        pstmt->setString(4, to_nickName);
        pstmt->setString(5, text);
        int rows=pstmt->executeUpdate();
        if (rows > 0)
        {
            //更新对应贴子的评论数
            DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
            auto pstmt = db.getConnection()->prepareStatement("update post set comment_count=comment_count+1 where id=?");
            pstmt->setInt(1, post_id);
            pstmt->executeUpdate();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

Json Comment::getFirstComment()
{
    try
    {
        Json data = Json::array();
        DataBase db(HOSTNAME, USERNAME_COMMENT, DBNAME_COMMENT, PASSWORD, false);
        auto res = db.getStatement()->executeQuery("select * from " + tableName + " where father=0");
        while (res->next())
        {
            //std::cout << 1111 << endl;
            Json info;
            info["id"] = res->getInt("id");
            info["nickName"] = res->getString("nickName");
            info["account_id"] = res->getInt("account_id");
            info["user_id"] = auth_utils.get_user_id(info["account_id"].get<int>());
            info["releaseTime"] = res->getString("releaseTime");
            info["text"] = res->getString("text");
            info["like_num"] = res->getInt("like_num");

            DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
            auto pstmt = db.getConnection()->prepareStatement("select avatar_image_url from user_profile where account_id=?");
            pstmt->setInt(1, info["account_id"].get<int>());
            auto rres = pstmt->executeQuery();
            if (rres->next())
            {
                //std::cout << 2 << endl;
                info["avatar_image_url"] = rres->getString("avatar_image_url");
            }

            data.push_back(info);
        }
        return data;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return Json::array();
    }
}

Json Comment::getChildrenComment(int father)
{
    try
    {
        //std::cout << father << endl;
       // std::cout << tableName << endl;
        Json data = Json::array();
        DataBase db(HOSTNAME, USERNAME_COMMENT, DBNAME_COMMENT, PASSWORD, true);
        auto pstmt = db.getConnection()->prepareStatement("select * from " + tableName + " where father=?");
        pstmt->setInt(1, father);
        auto res = pstmt->executeQuery();
        while (res->next())
        {
            
            Json info;
            info["id"] = res->getInt("id");
            info["nickName"] = res->getString("nickName");
            info["account_id"] = res->getInt("account_id");
            info["user_id"] = auth_utils.get_user_id(info["account_id"].get<int>());
            info["releaseTime"] = res->getString("releaseTime");
            info["father"] = res->getInt("father");
            info["to_nickName"] = res->getString("to_nickName");
            info["text"] = res->getString("text");
            info["like_num"] = res->getInt("like_num");

            DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
            auto pstmt = db.getConnection()->prepareStatement("select avatar_image_url from user_profile where account_id=?");
            pstmt->setInt(1, info["account_id"].get<int>());
            auto rres = pstmt->executeQuery();
            if (rres->next())
            {
                info["avatar_image_url"] = rres->getString("avatar_image_url");
            }

            data.push_back(info);
        }
        return data;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return Json::array();
    }
}

int Comment::like()
{
    try
    {
        DataBase db(HOSTNAME, USERNAME_COMMENT, DBNAME_COMMENT, PASSWORD, true);
        auto pstmt = db.getConnection()->prepareStatement("update " + tableName + " set like_num=like_num+1 where id=?");
        pstmt->setInt(1, id);
        pstmt->executeUpdate();

        pstmt = db.getConnection()->prepareStatement("select like_num from " + tableName + " where id=?");
        pstmt->setInt(1, id);
        auto res = pstmt->executeQuery();
        if (res->next())
        {
            return res->getInt("like_num");
        }

    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}

int Comment::unlike()
{
    try
    {
        DataBase db(HOSTNAME, USERNAME_COMMENT, DBNAME_COMMENT, PASSWORD, true);
        auto pstmt = db.getConnection()->prepareStatement("update " + tableName + " set like_num =GREATEST(0, like_num - 1) where id=?");
        pstmt->setInt(1, id);
        pstmt->executeUpdate();

        pstmt = db.getConnection()->prepareStatement("select like_num from " + tableName + " where id=?");
        pstmt->setInt(1, id);
        auto res = pstmt->executeQuery();
        if (res->next())
        {
            return res->getInt("like_num");
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}


crow::response CommentController::saveComment(const crow::request& req)
{
    auto data = crow::json::load(req.body);
    if (!data)
    {
        return crow::response(400, "Invalid Json Data!");
    }
    try
    {
        int post_id = data["post_id"].i();
        std::string user_id = data["user_id"].s();
        int father = data["father"].i();
        std::string to_nickName = data["to_nickName"].s();
        std::string text = data["text"].s();
        std::string nickName = auth_utils.get_nickName(user_id);
        int account_id = auth_utils.get_account_id(user_id);

        Comment cc(post_id, nickName,account_id, father, to_nickName, text);
        if (cc.createTable())
        {
            if (cc.saveComment())
            {
                return crow::response(crow::json::wvalue({ {"state",true} ,{"info","Comment save successfully"} }));
            }
        }
        return crow::response(crow::json::wvalue({ {"state",false} ,{"info","Failed to save comment"} }));
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return crow::response(500, "Internal Server Error");
    }
}

crow::response CommentController::getComment(const crow::request& req)
{
    try
    {
        int post_id;
        try
        {
            post_id = std::stoi(req.url_params.get("post_id"));
        }
        catch(std::exception&e)
        {
            return crow::response(400, "Invalid Data");
        }

        Comment cc(post_id);
        Json firstData = cc.getFirstComment();
        Json resData = Json::array();
        for (auto& val : firstData)
        {
            Json childrenData = cc.getChildrenComment(val["id"].get<int>());
            Json info;
            info["firstComment"] = val;
            info["children"] = childrenData;
            resData.push_back(info);
        }
        Json return_Data;
        return_Data["count"] = firstData.size();
        return_Data["results"] = resData;

        return crow::response(200, return_Data.dump(4));
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return crow::response(500, "Internal Server Error");
    }
}

crow::response CommentController::like(const crow::request& req)
{
    auto data = crow::json::load(req.body);
    if (!data)
    {
        return crow::response(400, "Invalid Json Data!");
    }
    try
    {
        int id = data["id"].i();
        int post_id = data["post_id"].i();
        Comment cc(id, post_id);
        int count = cc.like();
        if (count != -1)
        {
            crow::json::wvalue response;
            response["state"] = true;
            response["count"] = count;
            return crow::response(response);
        }
        else
        {
            crow::json::wvalue response;
            response["state"] = false;
            return crow::response(400,response);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return crow::response(500, "Internal Server Error");
    }
}

crow::response CommentController::unlike(const crow::request& req)
{
    auto data = crow::json::load(req.body);
    if (!data)
    {
        return crow::response(400, "Invalid Json Data!");
    }
    try
    {
        int id = data["id"].i();
        int post_id = data["post_id"].i();
        Comment cc(id, post_id);
        int count = cc.unlike();
        if (count != -1)
        {
            crow::json::wvalue response;
            response["state"] = true;
            response["count"] = count;
            return crow::response(response);
        }
        else
        {
            crow::json::wvalue response;
            response["state"] = false;
            return crow::response(400, response);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return crow::response(500, "Internal Server Error");
    }
}
