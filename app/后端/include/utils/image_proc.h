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


	bool newBucket(const std::string& bucketName_); //�����洢Ͱ

	//bool deleteBucket(const std::string& bucketName_);

	bool isExists_bN(const std::string& bucketName);  //�洢Ͱ�Ƿ����

	std::vector<std::string> listBucket();  //�г����д洢Ͱ


	bool newDir(const std::string& dirName);/* ��дĿ¼���ƣ�Ŀ¼������б�߽�β������exampledir/��  ע��     */

	bool deleteDir(const std::string& dirName);/* ��дĿ¼���ƣ�Ŀ¼������б�߽�β������exampledir/��  ע��    */

	bool isExists_dN(const std::string& dirName); //Ŀ¼�Ƿ����

	bool isExists_object(const std::string& dirName, const std::string& imageId); //�ļ��Ƿ����

	std::vector<std::string> listDir(const std::string& prefix);  //�г�����Ŀ¼

	std::string getURL_forever(const std::string dirName, const std::string& imageId);

	std::string getURL(const std::string dirName, const std::string& imageId);  //����ͼƬURL

	bool submitImages(const std::string& imageBinaryData, const std::string dirName, const std::string& imageId);   //�ϴ�ͼƬ   

	void setImagesStyle(int width, int height);  //����ͼƬ�ĳ����

	bool deleteImages(const std::string dirName, const std::string& imageId);   //ɾ��ͼƬ 

	bool moveImages(const std::string& sourceDir, const std::string& destinationDir, const std::string& sourceImageId, const std::string& destinationImageId);  //Ǩ��ͼƬ

private:
	std::string endpoint;  //����
	std::string bucketName; //OSS�洢Ͱ����

	//��������Կ
	std::string accessKeyId;
	std::string accessKeySecret;

	//// ʹ��ƾ֤��ʼ��OSS�ͻ���
	//ClientConfiguration conf;
	//OssClient client;   //����ִ��OSS�����Ķ���

	//ͼƬ�ĳ���
	std::string width = "w_100";
	std::string height = "h_150";
};



IMAGETOOSSCPP_API std::vector<unsigned char> getImageBinary(const std::string& filePath);





#endif // !IMAGEOPERATOR_H


