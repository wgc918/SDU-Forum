#pragma once
namespace config
{
#define VERIFICATIONCODE "sduforum"
#define HOSTNAME "*****"     //���ݿ�������
#define USERNAME_USERDB "myuserdb"        //���ݿ��û���
#define USERNAME_MANAGE "mymanage"
#define USERNAME_COMMENT "mycomment"
#define PASSWORD "*****"         //���ݿ�����
#define DBNAME_USERDB   "userdb"           //���ݿ���
#define DBNAME_COMMENT "comment"
#define DBNAME_MANAGE "manage"

#define ACCESS_JWT_SECRET "****"
#define REFRESH_JWT_SECRET "****"
#define ISSUER "SDU_Forum"
//#define _TOKEN_TYPE "JWT"
#define ACCESS_TOKEN_TIME_LIMIT 60
#define REFRESH_TOKEN_TIME_LIMIT 24*7


}
