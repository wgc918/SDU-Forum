#pragma once
#include<iostream>
#include<string>
#include<crow.h>
#include"auth_utils.h"

class SearchController
{
private:
	Auth_utils auth_utils;

public:
	explicit SearchController(){}

	crow::response allTextSearch(const crow::request& req);

	crow::response tagSearch(const crow::request& req);

	crow::response top10_post(const crow::request& req);

	crow::response hotTag(const crow::request& req);
};