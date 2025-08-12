#pragma once
namespace config
{
#define VERIFICATIONCODE "sduforum"
#define HOSTNAME "*****"     //数据库主机名
#define USERNAME_USERDB "myuserdb"        //数据库用户名
#define USERNAME_MANAGE "mymanage"
#define USERNAME_COMMENT "mycomment"
#define PASSWORD "*****"         //数据库密码
#define DBNAME_USERDB   "userdb"           //数据库名
#define DBNAME_COMMENT "comment"
#define DBNAME_MANAGE "manage"

#define ACCESS_JWT_SECRET "****"
#define REFRESH_JWT_SECRET "****"
#define ISSUER "SDU_Forum"
//#define _TOKEN_TYPE "JWT"
#define ACCESS_TOKEN_TIME_LIMIT 60
#define REFRESH_TOKEN_TIME_LIMIT 24*7


}
