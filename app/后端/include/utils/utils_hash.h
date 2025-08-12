#pragma once
#include<openssl/evp.h>
#include<openssl/rand.h>
#include<openssl/err.h>
#include<string>
#include<iomanip>
#include<sstream>
#include<vector>
#include<stdexcept>
#include<iostream>
#include<chrono>


namespace Hash
{
	struct PasswordData
	{
		std::string hash; //哈希值
		std::string salt; //盐值
		int iterations;  //迭代次数
	};

	void handle_openssl_error();
	

	PasswordData create_password_hash(const std::string& password);
	
	bool verify_password(const std::string& password, const PasswordData& stored);
	
}
