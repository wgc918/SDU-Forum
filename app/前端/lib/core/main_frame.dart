import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/forum/content_page.dart';
import 'package:sduforum/pages/messages/message_page.dart';
import 'package:sduforum/pages/post_create/post_create_page.dart';
import 'package:sduforum/pages/profile/profile_page.dart';
import 'package:sduforum/pages/search/search_page.dart';

class MainFramePage extends StatelessWidget {
  final String account;
  final int? initialTabIndex;
  final Dio dio;

  const MainFramePage(
      {super.key, required this.account, this.initialTabIndex = 0,
    required this.dio,
  });

  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: 5,
      initialIndex: initialTabIndex!,
      child: Scaffold(
        appBar: AppBar(
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Image.asset(
                'asset/images/logo.png',
                height: 40,
              ),
              const SizedBox(width: 10),
              RichText(
                text: TextSpan(
                  children: [
                    TextSpan(
                      text: 'SDU',
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                        color: Colors.blue.shade800,
                      ),
                    ),
                    TextSpan(
                      text: '校园论坛',
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                        color: Colors.grey.shade800,
                      ),
                    ),
                    TextSpan(
                      text: '(青岛)',
                      style: TextStyle(
                        fontSize: 16,
                        color: Colors.grey.shade600,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
          centerTitle: true,
          elevation: 0,
          backgroundColor: Colors.white,
          foregroundColor: Colors.blue.shade800,
          shape: const Border(
            bottom: BorderSide(
              color: Colors.black12,
              width: 0.5,
            ),
          ),
        ),
        body: TabBarView(
          physics: const NeverScrollableScrollPhysics(), // 禁止左右滑动切换
          children: [
            _buildForumTab(),
            _buildSearchTab(),
            _buildPostTab(),
            _buildMessageTab(),
            _buildProfileTab(account),
          ],
        ),
        bottomNavigationBar: Container(
          decoration: BoxDecoration(
            border: Border(
              top: BorderSide(
                color: Colors.grey.shade300,
                width: 0.5,
              ),
            ),
          ),
          child: TabBar(
            labelColor: Theme.of(context).colorScheme.primary,
            unselectedLabelColor: Colors.grey,
            indicatorColor: Colors.transparent,
            labelStyle: const TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.w500,
            ),
            tabs: const [
              Tab(icon: Icon(Icons.forum_outlined), text: '论坛'),
              Tab(icon: Icon(Icons.search_outlined), text: '搜索'),
              Tab(icon: Icon(Icons.add_circle_outline), text: '发布'),
              Tab(icon: Icon(Icons.chat_outlined), text: '消息'),
              Tab(icon: Icon(Icons.person_outline), text: '我的'),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildForumTab() {
    return ContentPage(user_id: account);
  }

  Widget _buildSearchTab() {
    return SearchPage(user_id: account);
  }

  Widget _buildPostTab() {
    return PostCreatePage(userId: account);
  }

  Widget _buildMessageTab() {
    return const MessagePage();
  }

  Widget _buildProfileTab(String account) {
    return  ProfilePage(user_id:account, isCurrentUser: true);
  }
}
