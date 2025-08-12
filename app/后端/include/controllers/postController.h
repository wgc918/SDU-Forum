#pragma once
#include<crow.h>
#include"auth_utils.h"
#include<string>


class PostController
{
private:
	std::string dirName = "post_images/";
public:
	explicit PostController(){}
	static Auth_utils auth_utils;

	//��������
	crow::response create_post(const crow::request& req);

	//��ҳ�������Ӽ�Ҫ��Ϣ
	crow::response get_post_list(const crow::request& req);

	//������������
	crow::response get_post_details(const crow::request& req);

	//�����û��������������Ӽ�Ҫ��Ϣ
	crow::response get_post_user_list(const crow::request& req);

	//ɾ������
	crow::response delete_post(const crow::request& req);

	//�����û��ղص��������ӵļ�Ҫ��Ϣ
	crow::response get_post_favorite_list(const crow::request& req);

	//�ղ�����
	crow::response add_post_favorite(const crow::request& req);

	//ȡ���ղ�
	crow::response sub_post_favorite(const crow::request& req);

	//��������
	crow::response like(const crow::request& req);

	//ȡ������
	crow::response unlike(const crow::request& req);

};