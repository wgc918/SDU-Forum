#pragma once
#include<iostream>
#include<crow.h>
#include"config.h"

#include"Database.h"



crow::response test_post(const crow::request& req);

crow::response test_get(const crow::request& req);