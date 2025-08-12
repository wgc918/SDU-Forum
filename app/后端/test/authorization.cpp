#include<iostream>
#include<crow.h>
#include"Database.h";
#include"config.h"
#include"authController.h"
#include<jwt-cpp/jwt.h>
#include"auth_middleware.h"
#include"image_proc.h"
#include<vector>

using namespace sql;
using namespace crow;
using namespace std;



int test()
{
	crow::SimpleApp app;

	


	CROW_ROUTE(app, "/test")
		.methods("GET"_method)
		([](const crow::request& req)
			{
				// ������ͷ��ȡ Token
				auto auth_header = req.get_header_value("Authorization");
				if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
					return crow::response(401, "No token provided");
				}

				// ��ȡ Token��ȥ�� "Bearer " ǰ׺��
				std::string token = auth_header.substr(7);

				try {
					// ��֤ token
					auto decoded = jwt::decode(token);

					// ������֤��
					auto verifier = jwt::verify()
						.allow_algorithm(jwt::algorithm::hs256{ ACCESS_JWT_SECRET })
						.with_issuer(ISSUER);

					// ��֤ token
					verifier.verify(decoded);

					//cout << "Token is valid!" << endl;
					//cout << "Payload: " << decoded.get_payload() << endl;

					crow::json::wvalue response;
					response["info"] = "Token is valid!";
					response["Payload"] = decoded.get_payload();
					return crow::response(response);
				}

				catch (const std::exception& e) {
					// ���������쳣
					cerr << "Error: " << e.what() << endl;
					cout << "Invalid or expired token" << endl;
					return crow::response(400, "fail");
				}
			});

	app.port(8080).multithreaded().run();



	return 0;
}



