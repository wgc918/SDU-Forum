#pragma once
#include<iostream>
#include<string>
#include<crow.h>

class FeedbackController
{
public:
	explicit FeedbackController() {}

	crow::response getFeedback(const crow::request& req);

	crow::response saveFeedback(const crow::request& req);
};