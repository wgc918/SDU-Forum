# SDU-Forum 前端应用

基于Flutter的校园论坛移动应用，提供用户注册登录、帖子浏览发布、评论互动等功能。

## 技术栈

- 框架: Flutter 3.0
- 状态管理: Provider
- HTTP客户端: Dio
- 持久化: SharedPreferences
- 图片处理: CachedNetworkImage

## 目录结构
|-core
| |-main_frame.dart
|-pages  
| |-auth
| | |-login_page.dart
| | |-register_page.dart
| |-forum  
| | |-content_page.dart
| | |-detail_page.dart
| | |-fullScreenImage_page.dart
| |-messages  
| | |-message_page.dart
| |-post_create  
| | |-post_create_page.dart
| |-profile  
| | |-about_page.dart
| | |-editProfile_page.dart
| | |-helpFeedback_page.dart
| | |-profile_page.dart
| | |-securitySettings_page.dart
| | |-settings_page.dart
| |-search  
| | |-search_page.dart
| |-home_page.dart  
| |  
|-utils
| |-tokenInterceptor.dart
| |-tokenManager.dart
| |-postController.cpp
| |-searchController.cpp
|-main.dart  
|  
|-test

## 主要功能页面

### 1. 认证页面
- 用户登录
- 用户注册

### 2. 论坛页面
- 帖子分类展示(5个tab)
- 帖子详情
- 图片全屏查看

### 3. 发布页面
- 帖子创建
- 图片上传
- 标签选择

### 4. 个人中心
- 个人资料展示与编辑
- 设置页面
- 帮助与反馈

### 5. 搜索页面
- 全文搜索
- 标签搜索
- 热门帖子推荐

## 核心功能实现

### 1. Token管理
- Access Token自动刷新
- 请求拦截器实现
- Token本地持久化

### 2. 状态管理
- 用户认证状态
- 主题偏好设置
- 应用全局状态

### 3. 图片处理

- 图片缓存
- 图片压缩上传
- 图片预览与缩放

## 开发环境配置

### 系统要求
- Flutter SDK 3.0+
- Dart 2.17+
- Android Studio/VSCode

