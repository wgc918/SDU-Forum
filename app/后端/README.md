# SDU-Forum 后端服务

基于C++ Crow框架的校园论坛后端服务，提供用户认证、帖子管理、评论系统等功能。

## 技术栈

- 编程语言: C++17
- Web框架: Crow
- 数据库: MySQL 8.0
- 服务器: 阿里云ECS
- 构建工具: CMake

## 目录结构

|-include
| |-controllers
| | |-authController.h
| | |-commentController.h
| | |-feedbackController.h
| | |-postController.h
| | |-searchController.h
| |  
| |-db  
| | |-Database.h
| |
| |-utils  
| | |-auth_middleware.h
| | |-auth_utils.h
| | |-base64.h
| | |-image_proc.h
| | |-utils_hash.h
| |
| |-config.h
| |-logger.h
|  
|-src  
| |-controllers
| | |-authController.cpp
| | |-commentController.cpp
| | |-feedbackController.cpp
| | |-postController.cpp
| | |-searchController.cpp
| |-utils  
| | |-auth_middleware.cpp
| | |-auth_utils.cpp
| | |-base64.cpp
| | |-image_proc.cpp
| | |-utils_hash.cpp
|  
|-main.cpp  
|  
|-test  
| |-authorization.cpp
| |-insert_emoji_to_db.cpp
| |-insert_emoji_to_db.h

## 主要功能模块

### 1. 用户认证模块
- 基于PBKDF2算法的密码哈希存储
- JWT Token认证机制
- 用户封禁/解封管理
- 用户资料管理

### 2. 帖子管理模块
- 帖子分类展示(5个分类)
- 帖子CRUD操作
- 标签系统
- 图片上传与处理

### 3. 评论系统
- 多级评论结构
- 评论点赞功能
- 树形结构展示

### 4. 搜索功能
- 全文搜索
- 标签搜索
- 热门帖子推荐

## 数据库设计

### 主要数据表
1. 用户表(users)
2. 用户资料表(user_profiles)
3. 黑名单表(blacklist)
4. 帖子表(posts)
5. 帖子图片表(post_images)
6. 标签表(tags)
7. 帖子-标签关联表(post_tags)
8. 评论表(comments)

