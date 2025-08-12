#include "image_proc.h"
#include<string>
#include <AlibabaCloud/oss/OssClient.h>
#include<alibabacloud/oss/model/CopyObjectRequest.h>
#include<alibabacloud/oss/OssRequest.h>
#include<alibabacloud/oss/client/ClientConfiguration.h>
#include<fstream>

using namespace AlibabaCloud::OSS;

imageToOss::imageToOss()
{
    endpoint = "https://oss-cn-qingdao.aliyuncs.com";
    bucketName = "sdu-forum";
    accessKeyId = "key";
    accessKeySecret = "key";
}

imageToOss::imageToOss(const std::string& endpoint, const std::string& bucketName,
    const std::string& accessKeyId, const std::string& accessKeySecret)
    :endpoint(endpoint), bucketName(bucketName), accessKeyId(accessKeyId), accessKeySecret(accessKeySecret)
{

}

imageToOss::imageToOss(const std::string& endpoint, const std::string& bucketName) :endpoint(endpoint), bucketName(bucketName)
{
    accessKeyId = "key";
    accessKeySecret = "key";
}

imageToOss::imageToOss(const std::string& bucketName) :bucketName(bucketName)
{
    endpoint = "https://oss-cn-qingdao.aliyuncs.com";
    accessKeyId = "key";
    accessKeySecret = "key";
}



bool imageToOss::newBucket(const std::string& bucketName_)
{
    /* ��ʼ���������Դ��*/
    InitializeSdk();

    // ʹ��ƾ֤��ʼ��OSS�ͻ���  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    /*ָ���´���bucket�����ơ��洢���ͺ�ACL��*/
    CreateBucketRequest request(bucketName_, StorageClass::IA, CannedAccessControlList::PublicReadWrite);


    /*����Bucket��*/
    auto outcome = client.CreateBucket(request);

    if (!outcome.isSuccess())
    {
        /*�쳣����*/
        std::cout << "CreateBucket fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;
        return 0;
    }
    return 1;
}

//bool imageToOss::deleteBucket(const std::string& bucketName_)
//{
//    /* ��ʼ���������Դ��*/
//    InitializeSdk();
//
//    // ʹ��ƾ֤��ʼ��OSS�ͻ���  
//    ClientConfiguration conf;
//    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);
//
//
//    return false;
//}

bool imageToOss::isExists_bN(const std::string& bucketName_)
{
    /* ��ʼ���������Դ��*/
    InitializeSdk();

    // ʹ��ƾ֤��ʼ��OSS�ͻ���  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    auto outcome = client.DoesBucketExist(bucketName_);
    if (outcome)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

std::vector<std::string> imageToOss::listBucket()
{
    std::vector<std::string> data;
    InitializeSdk();
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    /*�оٵ�ǰ�˺��µ����д洢�ռ䡣*/
    ListBucketsRequest request;
    auto outcome = client.ListBuckets(request);


    if (outcome.isSuccess())
    {
        for (auto result : outcome.result().Buckets())
        {
            data.push_back(result.Name());
        }
    }
    else
    {
        /*�쳣����*/
        std::cout << "ListBuckets fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;
        return vector<std::string>();
    }


    return data;
}

bool imageToOss::newDir(const std::string& dirName)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }


    /* ��ʼ���������Դ��*/
    InitializeSdk();

    // ʹ��ƾ֤��ʼ��OSS�ͻ���  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();

    PutObjectRequest request(bucketName, dirName, content);

    /* ����Ŀ¼��*/
    auto outcome = client.PutObject(request);

    if (!outcome.isSuccess())
    {
        /* �쳣����*/
        std::cout << "PutObject fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;

        //ShutdownSdk();
        return 0;
    }
    else
    {
        //ShutdownSdk();
        return 1;
    }

    /* �ͷ��������Դ��*/
    ShutdownSdk();
}

bool imageToOss::deleteDir(const std::string& dirName)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    bool sign = true;
    InitializeSdk();

    // ʹ��ƾ֤��ʼ��OSS�ͻ���  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        /* �о��ļ���*/
        ListObjectsRequest request(bucketName);
        request.setPrefix(dirName);
        request.setMarker(nextMarker);
        auto outcome = client.ListObjects(request);

        if (!outcome.isSuccess()) {
            /* �쳣����*/
            std::cout << "ListObjects fail" <<
                ",code:" << outcome.error().Code() <<
                ",message:" << outcome.error().Message() <<
                ",requestId:" << outcome.error().RequestId() << std::endl;
            sign = 0;
            break;
        }
        for (const auto& object : outcome.result().ObjectSummarys())
        {
            DeleteObjectRequest request(bucketName, object.Key());
            /* ɾ��Ŀ¼��Ŀ¼�µ������ļ���*/
            auto delResult = client.DeleteObject(request);
        }
        nextMarker = outcome.result().NextMarker();
        isTruncated = outcome.result().IsTruncated();
    } while (isTruncated);


    return sign;
}

bool imageToOss::isExists_dN(const std::string& dirName)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    InitializeSdk();
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    auto outcome = client.DoesObjectExist(bucketName, dirName);

    if (outcome)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

bool imageToOss::isExists_object(const std::string& dirName, const std::string& imageId)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    InitializeSdk();
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);
    std::string  imagePath = dirName + imageId + ".jpg";

    auto outcome = client.DoesObjectExist(bucketName, imagePath);

    if (outcome)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

std::vector<std::string> imageToOss::listDir(const std::string& prefix)
{
    InitializeSdk();
    std::vector<std::string> data;
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    // ��������
    ListObjectsRequest request(bucketName);
    request.setPrefix(prefix);

    auto outcome = client.ListObjects(request);

    if (outcome.isSuccess())
    {
        for (const auto& object : outcome.result().ObjectSummarys()) {
            //std::cout << "object" <<
            //    ",name:" << object.Key() <<
            //    ",size:" << object.Size() <<
            //    /*",lastmodify time:" << object.LastModified() <<*/ std::endl;
            //std::cout << object.Key() << std::endl;
            //ossʵ����û���ļ���Ŀ¼�ṹһ˵��ֻ�ǰ���/��β�Ķ�����Ϊ�ļ���
            if (object.Size() == 0)
            {
                //std::cout << object.Key() << std::endl;
                data.push_back(object.Key());
            }

        }
    }


    return data;
}

std::string imageToOss::getURL_forever(const std::string dirName, const std::string& imageId)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return std::string("invalid_directory");
    }

    // ���칫�� URL
    std::string imagePath = dirName + imageId + ".jpg";
    std::string tempEndpoint = endpoint.substr(8);
    //std::cout << "������" << tempEndpoint << std::endl;
    // ���ɹ��� URL
    std::string publicUrl = "https://" + bucketName + "." + tempEndpoint + "/" + imagePath;

    // ���ع��� URL
    return publicUrl;
}

std::string imageToOss::getURL(const std::string dirName, const std::string& imageId)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    /* ��ʼ���������Դ��*/
    InitializeSdk();

    // ʹ��ƾ֤��ʼ��OSS�ͻ���  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::string process = "image/resize,m_fixed," + width + "," + height;

    std::string imagePath = dirName + imageId + ".jpg";

    /* ���ɴ�ͼƬ����������ļ�ǩ��URL��*/
    GeneratePresignedUrlRequest request(bucketName, imagePath, Http::Get);

    request.setProcess(process);
    auto outcome = client.GeneratePresignedUrl(request);

    if (outcome.isSuccess())
    {
        //std::cout << "Generated presigned URL: " << outcome.result() << std::endl;
        return outcome.result();
    }
    else
    {
        std::cout << "Failed to generate presigned URL. Error code: " << outcome.error().Code()
            << ", Message: " << outcome.error().Message()
            << ", RequestId: " << outcome.error().RequestId() << std::endl;

        return std::string("failed");
    }

    ShutdownSdk();
}

bool imageToOss::submitImages(const std::string& imageBinaryData, const std::string dirName, const std::string& imageId)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    InitializeSdk();

    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << imageBinaryData;

    std::string imagePath = dirName + imageId + ".jpg";

    PutObjectRequest request(bucketName, imagePath, content);

    auto outcome = client.PutObject(request);

    if (!outcome.isSuccess())
    {
        /* �쳣���� */
        std::cout << "CreateBucket fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;

        return false;
    }
    else
    {
        return true;
    }

    ShutdownSdk();

}

void imageToOss::setImagesStyle(int width, int height)
{
    this->width = "w_" + std::to_string(width);
    this->height = "h_" + std::to_string(height);
}

bool imageToOss::deleteImages(const std::string dirName, const std::string& imageId)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    InitializeSdk();

    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::string imagePath = dirName + imageId + ".jpg";

    DeleteObjectRequest request(bucketName, imagePath);

    /* ɾ���ļ���*/
    auto outcome = client.DeleteObject(request);

    if (!outcome.isSuccess())
    {
        /* �쳣����*/
        std::cout << "DeleteObject fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;
        return false;
    }
    else
    {
        return true;
    }

    ShutdownSdk();

    return false;
}

bool imageToOss::moveImages(const std::string& sourceDir, const std::string& destinationDir, const std::string& sourceImageId, const std::string& destinationImageId)
{
    if (sourceDir[sourceDir.size() - 1] != '/' || destinationDir[destinationDir.size() - 1] != '/')
    {
        std::cout << "The last character of the sourceDir or the destinationDir must be '/'" << std::endl;
        return 0;
    }
    InitializeSdk();

    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::string sourceImagePath = sourceDir + sourceImageId + ".jpg";

    std::string destinationImagePath = destinationDir + destinationImageId + ".jpg";

    CopyObjectRequest request(bucketName, destinationImagePath);///���ø��ƶ���Ŀ��·��
    request.setCopySource(bucketName, sourceImagePath);//���ñ�����Դ

    auto outcome = client.CopyObject(request);

    // ���outcome��ȷ�ϲ����Ƿ�ɹ�  
    if (!outcome.isSuccess())
    {
        // �������  
        std::cerr << "CopyObject failed: " << outcome.error().Message() << std::endl;
        return false;
    }
    else
    {
        //std::cout << "CopyObject succeeded" << std::endl;

        //���Ƴɹ�����ɾ��Դ�ļ�
        deleteImages(sourceDir, sourceImageId);

        return true;
    }
}


std::vector<unsigned char> getImageBinary(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        throw std::runtime_error("File not found");
    }

    // �ƶ����ļ�ĩβ�Ի�ȡ�ļ���С  
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // �����㹻���ڴ����洢�����ļ�  
    std::vector<unsigned char> buffer(size);

    // ��ȡ�ļ����ݵ�buffer  
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        std::cerr << "Failed to read file: " << filePath << std::endl;
        throw std::runtime_error("Read error");
    }

    file.close();
    return buffer;
}