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
    /* 初始化网络等资源。*/
    InitializeSdk();

    // 使用凭证初始化OSS客户端  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    /*指定新创建bucket的名称、存储类型和ACL。*/
    CreateBucketRequest request(bucketName_, StorageClass::IA, CannedAccessControlList::PublicReadWrite);


    /*创建Bucket。*/
    auto outcome = client.CreateBucket(request);

    if (!outcome.isSuccess())
    {
        /*异常处理。*/
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
//    /* 初始化网络等资源。*/
//    InitializeSdk();
//
//    // 使用凭证初始化OSS客户端  
//    ClientConfiguration conf;
//    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);
//
//
//    return false;
//}

bool imageToOss::isExists_bN(const std::string& bucketName_)
{
    /* 初始化网络等资源。*/
    InitializeSdk();

    // 使用凭证初始化OSS客户端  
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

    /*列举当前账号下的所有存储空间。*/
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
        /*异常处理。*/
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


    /* 初始化网络等资源。*/
    InitializeSdk();

    // 使用凭证初始化OSS客户端  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();

    PutObjectRequest request(bucketName, dirName, content);

    /* 创建目录。*/
    auto outcome = client.PutObject(request);

    if (!outcome.isSuccess())
    {
        /* 异常处理。*/
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

    /* 释放网络等资源。*/
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

    // 使用凭证初始化OSS客户端  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        /* 列举文件。*/
        ListObjectsRequest request(bucketName);
        request.setPrefix(dirName);
        request.setMarker(nextMarker);
        auto outcome = client.ListObjects(request);

        if (!outcome.isSuccess()) {
            /* 异常处理。*/
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
            /* 删除目录及目录下的所有文件。*/
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

    // 创建请求
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
            //oss实际上没有文件夹目录结构一说，只是把以/结尾的对象作为文件夹
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

    // 构造公共 URL
    std::string imagePath = dirName + imageId + ".jpg";
    std::string tempEndpoint = endpoint.substr(8);
    //std::cout << "地域名" << tempEndpoint << std::endl;
    // 生成公共 URL
    std::string publicUrl = "https://" + bucketName + "." + tempEndpoint + "/" + imagePath;

    // 返回公共 URL
    return publicUrl;
}

std::string imageToOss::getURL(const std::string dirName, const std::string& imageId)
{
    if (dirName[dirName.size() - 1] != '/')
    {
        std::cout << "The last character of the dirName must be '/'" << std::endl;
        return 0;
    }
    /* 初始化网络等资源。*/
    InitializeSdk();

    // 使用凭证初始化OSS客户端  
    ClientConfiguration conf;
    OssClient client(endpoint, accessKeyId, accessKeySecret, conf);

    std::string process = "image/resize,m_fixed," + width + "," + height;

    std::string imagePath = dirName + imageId + ".jpg";

    /* 生成带图片处理参数的文件签名URL。*/
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
        /* 异常处理 */
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

    /* 删除文件。*/
    auto outcome = client.DeleteObject(request);

    if (!outcome.isSuccess())
    {
        /* 异常处理。*/
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

    CopyObjectRequest request(bucketName, destinationImagePath);///设置复制对象目标路径
    request.setCopySource(bucketName, sourceImagePath);//设置被复制源

    auto outcome = client.CopyObject(request);

    // 检查outcome以确认操作是否成功  
    if (!outcome.isSuccess())
    {
        // 处理错误  
        std::cerr << "CopyObject failed: " << outcome.error().Message() << std::endl;
        return false;
    }
    else
    {
        //std::cout << "CopyObject succeeded" << std::endl;

        //复制成功后考虑删除源文件
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

    // 移动到文件末尾以获取文件大小  
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 分配足够的内存来存储整个文件  
    std::vector<unsigned char> buffer(size);

    // 读取文件内容到buffer  
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        std::cerr << "Failed to read file: " << filePath << std::endl;
        throw std::runtime_error("Read error");
    }

    file.close();
    return buffer;
}