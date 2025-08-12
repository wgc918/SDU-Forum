#ifndef IMAGEOPERATOR_H
#define IMAGEOPERATOR_H

#include<iostream>
#include<vector>


#define IMAGETOOSSCPP_API __declspec(dllexport)

class IMAGETOOSSCPP_API imageToOss
{
public:
	imageToOss();

	imageToOss(const std::string& endpoint, const std::string& bucketName);

	imageToOss(const std::string& bucketName);

	imageToOss(const std::string& endpoint, const std::string& bucketName, const std::string& accessKeyId, const std::string& accessKeySecret);


	bool newBucket(const std::string& bucketName_); //创建存储桶

	//bool deleteBucket(const std::string& bucketName_);

	bool isExists_bN(const std::string& bucketName);  //存储桶是否存在

	std::vector<std::string> listBucket();  //列出所有存储桶


	bool newDir(const std::string& dirName);/* 填写目录名称，目录需以正斜线结尾，例如exampledir/。  注意     */

	bool deleteDir(const std::string& dirName);/* 填写目录名称，目录需以正斜线结尾，例如exampledir/。  注意    */

	bool isExists_dN(const std::string& dirName); //目录是否存在

	bool isExists_object(const std::string& dirName, const std::string& imageId); //文件是否存在

	std::vector<std::string> listDir(const std::string& prefix);  //列出所有目录

	std::string getURL_forever(const std::string dirName, const std::string& imageId);

	std::string getURL(const std::string dirName, const std::string& imageId);  //返回图片URL

	bool submitImages(const std::string& imageBinaryData, const std::string dirName, const std::string& imageId);   //上传图片   

	void setImagesStyle(int width, int height);  //设置图片的长宽比

	bool deleteImages(const std::string dirName, const std::string& imageId);   //删除图片 

	bool moveImages(const std::string& sourceDir, const std::string& destinationDir, const std::string& sourceImageId, const std::string& destinationImageId);  //迁移图片

private:
	std::string endpoint;  //地域
	std::string bucketName; //OSS存储桶名称

	//阿里云密钥
	std::string accessKeyId;
	std::string accessKeySecret;

	//// 使用凭证初始化OSS客户端
	//ClientConfiguration conf;
	//OssClient client;   //用于执行OSS操作的对象

	//图片的长宽
	std::string width = "w_100";
	std::string height = "h_150";
};



IMAGETOOSSCPP_API std::vector<unsigned char> getImageBinary(const std::string& filePath);





#endif // !IMAGEOPERATOR_H


