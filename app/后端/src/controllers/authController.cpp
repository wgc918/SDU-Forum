#include "authController.h"
#include"Database.h"
#include"config.h"
#include<jwt-cpp/jwt.h>
#include"utils_hash.h"
#include"image_proc.h"
#include"base64.h"
#include<nlohmann/json.hpp>
using Json = nlohmann::json;



crow::response AuthController::Login(const crow::request& req)
{
	try
	{
		auto data = crow::json::load(req.body);
		if (!data)
		{
			return crow::response(400, "Invalid Json Data!");
		}
		std::string user_id = data["user_id"].s();
		std::string password = data["password"].s();

		//检查用户是否在黑名单中
		DataBase dd(HOSTNAME, USERNAME_MANAGE, DBNAME_MANAGE, PASSWORD, true);
		auto pstmt_ = dd.getConnection()->prepareStatement("select * from account_blacklist where user_id=?");
		pstmt_->setString(1, user_id);
		auto res_ = pstmt_->executeQuery();
		if (res_->next())
		{
			crow::json::wvalue response;
			response["login_state"] = false;
			response["is_blacklist"] = true;
			response["reason"] = res_->getString("reason");
			response["end_time"] = res_->getString("end_time");
			return crow::response(response);
		}


		//访问数据库对账号密码进行验证
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select password_hash,salt,iterations from account where user_id=?");
		pstmt->setString(1, user_id);
		auto res = pstmt->executeQuery();
		bool isLogin = false;
		if (res->next()&& Hash::verify_password(password,
			{res->getString("password_hash"),res->getString("salt"),res->getInt("iterations")}))
		{
			isLogin = true;
			//生成 Access Token
			auto access_token = jwt::create()
				.set_issuer(ISSUER)
				.set_type("JWT")
				.set_payload_claim("user_id", jwt::claim(user_id))
				.set_issued_at(std::chrono::system_clock::now())
				.set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(ACCESS_TOKEN_TIME_LIMIT))
				.sign(jwt::algorithm::hs256{ ACCESS_JWT_SECRET });

			//生成 Refresh Token
			auto refresh_token = jwt::create()
				.set_issuer(ISSUER)
				.set_type("JWT")
				.set_payload_claim("user_id", jwt::claim(user_id))
				.set_issued_at(std::chrono::system_clock::now())
				.set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(REFRESH_TOKEN_TIME_LIMIT))
				.sign(jwt::algorithm::hs256{ REFRESH_JWT_SECRET });

			
			// 解析 access_token 获取过期时间
			auto decoded_access = jwt::decode(access_token);
			auto access_exp = decoded_access.get_expires_at();

			// 解析 refresh_token 获取过期时间
			auto decoded_fresh = jwt::decode(refresh_token);
			auto refresh_exp = decoded_fresh.get_expires_at();

			// 返回双 Token
			crow::json::wvalue response;
			response["login_state"] = isLogin;
			response["is_blacklist"] = false;
			response["access_token"] = access_token;
			response["access_token_exp"] = std::chrono::duration_cast<std::chrono::seconds>(access_exp.time_since_epoch()).count();
			response["refresh_token"] = refresh_token;
			response["refresh_token_exp"] = std::chrono::duration_cast<std::chrono::seconds>(refresh_exp.time_since_epoch()).count();
			return crow::response{ response };

		}
		else
		{
			crow::json::wvalue response;
			response["login_state"] = isLogin;
			response["is_blacklist"] = false;
			return crow::response{ response };
		}

	}
	catch (const std::exception&e)
	{
		std::cerr << e.what() << std::endl;
		crow::json::wvalue response;
		response["login_state"] = false;
		response["is_blacklist"] = false;
		response["error_info"] = "Invalid credentials";
		return crow::response(500, response);
	}
	
}

crow::response AuthController::Register(const crow::request& req)
{
	try
	{
		auto data = crow::json::load(req.body);
		if (!data)
		{
			return crow::response(400, "Invalid Json Data!");
		}

		std::string user_id = data["user_id"].s();
		std::string password = data["password"].s();
		std::string verificationCode = data["verificationCode"].s();

		int Register_state;

		//检验账号是否已经被注册
		if (!auth_utils.is_user_registered(user_id)&& verificationCode== VERIFICATIONCODE)
		{
			auto passwordData = Hash::create_password_hash(password);
			DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
			auto pstmt = db.getConnection()->prepareStatement
			("insert into account (user_id,password_hash,salt,iterations) values (?,?,?,?)");

			pstmt->setString(1, user_id);
			pstmt->setString(2, passwordData.hash);
			pstmt->setString(3, passwordData.salt);
			pstmt->setInt(4, passwordData.iterations);
			pstmt->executeUpdate();

			int account_id = auth_utils.get_account_id(user_id);
			pstmt = db.getConnection()->prepareStatement("insert into user_profile (account_id) values (?)");
			pstmt->setInt(1, account_id);
			pstmt->executeUpdate();


			Register_state = 0;
			return crow::response(crow::json::wvalue({ {"Register_state",Register_state} ,{"error_info","OK"} }));
		}
		else
		{
			if (verificationCode != VERIFICATIONCODE)
			{
				Register_state = 1;
				return crow::response(200, crow::json::wvalue({ {"Register_state",Register_state} ,{"error_info","The verificationCode is invalid"} }));
			}
			else
			{
				Register_state = 2;
				return crow::response(200, crow::json::wvalue({ {"Register_state",Register_state} ,{"error_info","The user_id is existed"} }));
			}
		}
	}
	catch (std::exception& e)
	{
		return crow::response(500, crow::json::wvalue({ {"Register_state",false} ,{"error_info",std::string("Failed to create user") + e.what()} }));
	}
}

crow::response AuthController::Refresh_token(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}
	std::string token = data["refresh_token"].s();
	std::string user_id = data["user_id"].s();

	//std::cout << "token:" << token << endl;
	//std::cout << "user_id" << user_id << endl;
	
	try 
	{
		//检查用户是否在黑名单中
		DataBase dd(HOSTNAME, USERNAME_MANAGE, DBNAME_MANAGE, PASSWORD, true);
		auto pstmt_ = dd.getConnection()->prepareStatement("select * from account_blacklist where user_id=?");
		pstmt_->setString(1, user_id);
		auto res_ = pstmt_->executeQuery();
		if (res_->next())
		{
			crow::json::wvalue response;
			response["refresh_state"] = false;
			response["is_blacklist"] = true;
			response["reason"] = res_->getString("reason");
			response["end_time"] = res_->getString("end_time");
			return crow::response(response);
		}

		//先访问数据库，查询refresh_token 是否在黑名单中
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select token from token_blackList where user_id=? and token=?");
		pstmt->setString(1, user_id);
		pstmt->setString(2, token);
		auto res = pstmt->executeQuery();

		if (!res->next())
		{
			std::cout << "hzel" << endl;
			// 验证 token
			auto decoded = jwt::decode(token);

			// 创建验证器
			auto verifier = jwt::verify()
				.allow_algorithm(jwt::algorithm::hs256{ REFRESH_JWT_SECRET })
				.with_issuer(ISSUER);

			// 验证 token
			verifier.verify(decoded);

			//cout << "Token is valid!" << endl;
			//cout << "Payload: " << decoded.get_payload() << endl;

			//生成  新Access Token
			auto new_access_token = jwt::create()
				.set_issuer(ISSUER)
				.set_type("JWT")
				.set_payload_claim("user_id", jwt::claim(user_id))
				.set_issued_at(std::chrono::system_clock::now())
				.set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(ACCESS_TOKEN_TIME_LIMIT))
				.sign(jwt::algorithm::hs256{ ACCESS_JWT_SECRET });

			// 解析 new_access_token 获取过期时间
			auto decoded_access = jwt::decode(new_access_token);
			auto access_exp = decoded_access.get_expires_at();

			crow::json::wvalue response;
			response["refresh_state"] = true;
			response["access_token"] = new_access_token;
			response["access_token_exp"] = std::chrono::duration_cast<std::chrono::seconds>(access_exp.time_since_epoch()).count();
			return crow::response(response);
		}
		else
		{
			std::cout << res->getString("token") << endl;
			return crow::response(crow::json::wvalue({ {"refresh_state",false},{"error_info","refresh_token is invalid"} }));
		}

		
	}
	catch (const std::exception& e) 
	{
		// 捕获其他异常
		cerr << "Error: " << e.what() << endl;
		cout << "Invalid or expired token" << endl;
		return crow::response(500, crow::json::wvalue({{"refresh_state",false}}));
	}
}

crow::response AuthController::Push_token_to_blackList(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}

	std::string user_id = data["user_id"].s();
	std::string token = data["token"].s();

	try
	{
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("insert into token_blackList (user_id,token) values (?,?)");
		pstmt->setString(1, user_id);
		pstmt->setString(2, token);
		pstmt->executeUpdate();

		crow::json::wvalue response;
		response["push_state"] = true;
		return crow::response(response);
	}
	catch (std::exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		return crow::response(500, crow::json::wvalue({ {"push_state",false} }));
	}
}

crow::response AuthController::edit_user_profile(const crow::request& req)
{
	auto data = crow::json::load(req.body);
	if (!data)
	{
		return crow::response(400, "Invalid Json Data!");
	}

	try
	{
		std::string user_id = data["user_id"].s();
		std::string nickName = data["nickName"].s();
		std::string major = data["major"].s();
		std::string background_image = data["background_image"].s();
		std::string avatar_image = data["avatar_image"].s();
		int account_id = auth_utils.get_account_id(user_id);

		int state_code = 0;
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		imageToOss oss;

		if (!nickName.empty())
		{
			//std::string nickName = data["nickName"].s();
			if (nickName.length() > 20)
			{
				state_code = 1;
				return crow::response(400, crow::json::wvalue({{"state_code",state_code},{"error_info","Invalid nickName"}}));
			}
			
			auto pstmt = db.getConnection()->prepareStatement("update user_profile set nickName=? where account_id=?");
			pstmt->setString(1, nickName);
			pstmt->setInt(2, account_id);
			pstmt->executeUpdate();
		}

		if (!major.empty())
		{
			//std::string major = data["major"].s();
			if (major.length() > 30)
			{
				state_code = 2;
				return crow::response(400, crow::json::wvalue({ {"state_code",state_code},{"error_info","Invalid major"} }));
			}
			
			auto pstmt = db.getConnection()->prepareStatement("update user_profile set major=? where account_id=?");
			pstmt->setString(1, major);
			pstmt->setInt(2, account_id);
			pstmt->executeUpdate();
		}

		if (!background_image.empty())
		{
			//将base64格式的图片数据流解码
			std::string image_base64 = base64_decode(data["background_image"].s());
			//把图片上传到oss服务器并返回url
			bool sign=oss.submitImages(image_base64, background_dirName, std::to_string(account_id));//以用户id为背景图命名
			std::string background_image_url = oss.getURL_forever(background_dirName, std::to_string(account_id));
			if (!sign)
			{
				state_code = 3;
				return crow::response(400, crow::json::wvalue({ {"state_code",state_code},{"error_info","Fail to upload background_image"} }));
			}

			//将url保存到用户信息表中
			auto pstmt = db.getConnection()->prepareStatement("update user_profile set background_image_url=? where account_id=?");
			pstmt->setString(1, background_image_url);
			pstmt->setInt(2, account_id);
			pstmt->executeUpdate();
		}

		if (!avatar_image.empty())
		{
			//将base64格式的图片数据流解码
			std::string image_base64 = base64_decode(data["avatar_image"].s());
			//把图片上传到oss服务器并返回url
			bool sign = oss.submitImages(image_base64, avatar_dirName, std::to_string(account_id));
			std::string avatar_image_url = oss.getURL_forever(avatar_dirName, std::to_string(account_id));//以用户id为头像命名
			if (!sign)
			{
				state_code = 4;
				return crow::response(400, crow::json::wvalue({ {"state_code",state_code},{"error_info","Fail to upload avatar_image"} }));
			}
			// 将url保存到用户信息表中
			auto pstmt = db.getConnection()->prepareStatement("update user_profile set avatar_image_url=? where account_id=?");
			pstmt->setString(1, avatar_image_url);
			pstmt->setInt(2, account_id);
			pstmt->executeUpdate();
		}

		return crow::response(crow::json::wvalue({ {"state_code",state_code},{"error_info","OK"} }));
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << endl;
		return crow::response(500, crow::json::wvalue({ {"state_code",-1},{"error_info","Internal Server Error"} }));
	}
}

crow::response AuthController::get_user_profile(const crow::request& req)
{
	try
	{
		std::string user_id = req.url_params.get("user_id");
		int account_id = auth_utils.get_account_id(user_id);

		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select * from user_profile where account_id=?");
		pstmt->setInt(1, account_id);
		auto res = pstmt->executeQuery();
		if (res->next())
		{
			crow::json::wvalue response;
			response["nickName"] = res->getString("nickName");
			response["major"] = res->getString("major");
			response["join_date"] = res->getString("created_at");
			response["favorite_post_count"] = res->getInt("favorite_post_count");
			response["liked_count"] = res->getInt("liked_count");
			response["published_post_count"] = res->getInt("published_post_count");
			response["background_image_url"] = res->getString("background_image_url");
			response["avatar_image_url"] = res->getString("avatar_image_url");
			return crow::response(response);
		}
	}
	catch (std::exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		return crow::response(500, "Internal Server Error");
	}
}

crow::response AuthController::modify_password(const crow::request& req)
{
	try {
		auto data = crow::json::load(req.body);
		if (!data)
		{
			return crow::response(400, "Invalid Json Data!");
		}
		std::string user_id,oldPassword, newPassword;
		user_id = data["user_id"].s();
		oldPassword = data["oldPassword"].s();
		newPassword = data["newPassword"].s();
		//std::cout << newPassword << std::endl;
		DataBase db(HOSTNAME, USERNAME_USERDB, DBNAME_USERDB, PASSWORD, true);
		auto pstmt = db.getConnection()->prepareStatement("select password_hash,salt,iterations from account where user_id=?");
		pstmt->setString(1, user_id);
		auto res = pstmt->executeQuery();
		
		if (res->next() && Hash::verify_password(oldPassword,
			{ res->getString("password_hash"),res->getString("salt"),res->getInt("iterations") }))
		{
			auto newPasswordData = Hash::create_password_hash(newPassword);
			pstmt = db.getConnection()->prepareStatement("update account set password_hash=?,salt=? where user_id=?");
			pstmt->setString(1, newPasswordData.hash);
			pstmt->setString(2, newPasswordData.salt);
			pstmt->setString(3, user_id);
			pstmt->executeUpdate();

			crow::json::wvalue response;
			response["state"] = true;
			return crow::response(response);
		}
		else
		{
			crow::json::wvalue response;
			response["state"] = false;
			response["info"] = "OldPassword is not correct";
			return crow::response(response);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return crow::response(500, "Internal Server Error");
	}
}





