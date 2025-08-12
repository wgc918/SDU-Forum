import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/forum/detail_page.dart';
import 'dart:convert';

class ContentPage extends StatefulWidget {
  final String user_id;
  const ContentPage({super.key, required this.user_id});

  @override
  State<ContentPage> createState() => _ContentPageState();
}

class _ContentPageState extends State<ContentPage>
    with TickerProviderStateMixin {
  final List<String> _categories = ["二手闲置", "打听求助", "学习搭子", "社团活动", "校园招聘"];

  final List<IconData> _categoryIcons = [
    Icons.shopping_cart_outlined,
    Icons.help_outline,
    Icons.school_outlined,
    Icons.people_outline,
    Icons.work_outlined,
  ];

  final List<Color> _categoryColors = [
    Colors.purple,
    Colors.blue,
    Colors.green,
    Colors.orange,
    Colors.red,
  ];

  late List<List<Map<String, dynamic>>> _posts;
  late List<bool> _isLoading;
  late List<bool> _isLoadingMore;
  late List<int> _currentPages;
  late List<ScrollController> _scrollControllers;
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: _categories.length, vsync: this);
    _tabController.addListener(_handleTabSelection);
    _posts = List.generate(_categories.length, (index) => []); // 初始化每个标签页的空列表
    _isLoading = List.generate(_categories.length, (index) => true);
    _isLoadingMore = List.generate(_categories.length, (index) => false);
    _currentPages = List.generate(_categories.length, (index) => 1);

    // 初始化 _scrollControllers
    _scrollControllers = List.generate(
      _categories.length,
      (index) => ScrollController(),
    );

    // 添加监听器
    for (int i = 0; i < _scrollControllers.length; i++) {
      _scrollControllers[i].addListener(() {
        _scrollListener(i);
      });
    }

    _loadPosts(0);
  }

  void _handleTabSelection() {
    if (!_tabController.indexIsChanging) {
      final index = _tabController.index;
      // 如果当前Tab的数据为空，则加载数据
      if (_posts[index].isEmpty) {
        _loadPosts(index);
      }
    }
  }

  @override
  void dispose() {
    for (var controller in _scrollControllers) {
      controller.dispose();
    }
    _tabController.removeListener(_handleTabSelection);
    _tabController.dispose();
    super.dispose();
  }

  void _scrollListener(int tabIndex) {
    final scrollController = _scrollControllers[tabIndex];
    if (!scrollController.hasClients ||
        _isLoadingMore[tabIndex] ||
        _isLoading[tabIndex]) {
      return;
    }

    final maxScroll = scrollController.position.maxScrollExtent;
    final currentScroll = scrollController.position.pixels;

    // 当滚动到距离底部300像素时加载更多
    if (maxScroll - currentScroll <= 300) {
      _loadMorePosts(tabIndex);
    }
  }

  Future<void> _loadPosts(int tabIndex, {bool refresh = false}) async {
    if (refresh) {
      _currentPages[tabIndex] = 1;
      setState(() {
        _posts[tabIndex] = []; // 清空当前标签页的帖子列表
      });
    }

    setState(() {
      if (refresh) {
        _isLoading[tabIndex] = true;
      } else {
        _isLoadingMore[tabIndex] = true;
      }
    });

    try {
      //print('开始${(_currentPages[tabIndex] - 1) * 10}');
      Dio dio = Dio();
      final response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/user/get_post_list',
        queryParameters: {
          'type': _categories[tabIndex],
          'start': (_currentPages[tabIndex] - 1) * 10,
          'pageSize': 10,
        },
      );
      //print('请求结果: ${response.data}');

      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == 'ok') {
          setState(() {
            if (refresh || _currentPages[tabIndex] == 1) {
              _posts[tabIndex] = List.from(data['result']);
            } else {
              _posts[tabIndex].addAll(List.from(data['result']));
            }
            _isLoading[tabIndex] = false;
            _isLoadingMore[tabIndex] = false;
          });
        }
      }
    } catch (e) {
      setState(() {
        _isLoading[tabIndex] = false;
        _isLoadingMore[tabIndex] = false;
        // 加载失败时回退页码
        if (!refresh) {
          _currentPages[tabIndex]--;
        }
      });
    }
  }

  Future<void> _loadMorePosts(int tabIndex) async {
    if (_isLoadingMore[tabIndex] || _isLoading[tabIndex]) return;

    // 检查是否已经滚动到底部
    final scrollController = _scrollControllers[tabIndex];
    if (scrollController.position.pixels <
        scrollController.position.maxScrollExtent - 200) {
      return;
    }

    setState(() {
      _currentPages[tabIndex]++;
    });

    await _loadPosts(tabIndex);
  }

  Future<void> _refreshPosts(int tabIndex) async {
    await _loadPosts(tabIndex, refresh: true);
  }

  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: _categories.length,
      child: Scaffold(
        appBar: AppBar(
          elevation: 0,
          toolbarHeight: 28,
          backgroundColor: Colors.white,
          bottom: PreferredSize(
            preferredSize: const Size.fromHeight(48),
            child: Container(
              decoration: BoxDecoration(
                color: Colors.white,
                boxShadow: [
                  BoxShadow(
                    color: Colors.grey.withOpacity(0.1),
                    spreadRadius: 1,
                    blurRadius: 3,
                    offset: const Offset(0, 1),
                  ),
                ],
              ),
              child: TabBar(
                isScrollable: false,
                controller: _tabController,
                labelColor: Colors.blue.shade700,
                unselectedLabelColor: Colors.grey.shade600,
                indicatorSize: TabBarIndicatorSize.label,
                indicatorWeight: 3,
                indicatorColor: Colors.blue.shade700,
                labelStyle: const TextStyle(
                  fontWeight: FontWeight.bold,
                  fontSize: 12,
                ),
                unselectedLabelStyle: const TextStyle(
                  fontWeight: FontWeight.normal,
                ),
                tabs: List.generate(
                  _categories.length,
                  (index) => Tab(
                    icon: Icon(_categoryIcons[index],
                        color: _categoryColors[index]),
                    text: _categories[index],
                  ),
                ),
              ),
            ),
          ),
        ),
        body: TabBarView(
          controller: _tabController,
          physics: const BouncingScrollPhysics(),
          children: List.generate(
            _categories.length,
            (index) => _buildTabContent(index),
          ),
        ),
      ),
    );
  }

  Widget _buildTabContent(int tabIndex) {
    return RefreshIndicator(
      onRefresh: () => _refreshPosts(tabIndex),
      child: _isLoading[tabIndex]
          ? const Center(child: CircularProgressIndicator())
          : CustomScrollView(
              controller: _scrollControllers[tabIndex],
              slivers: [
                SliverPadding(
                  padding: const EdgeInsets.all(16),
                  sliver: SliverList(
                    delegate: SliverChildBuilderDelegate(
                      (context, index) {
                        // 如果是最后一个item且正在加载更多，显示加载指示器
                        if (index >= _posts[tabIndex].length) {
                          return _buildLoader(tabIndex);
                        }
                        return _buildContentCard(
                          _categories[tabIndex],
                          _categoryIcons[tabIndex],
                          _posts[tabIndex][index],
                        );
                      },
                      childCount: _posts[tabIndex].length +
                          (_isLoadingMore[tabIndex] ? 1 : 0),
                    ),
                  ),
                ),
              ],
            ),
    );
  }

  Widget _buildLoader(int tabIndex) {
    if (_isLoadingMore[tabIndex]) {
      return const Padding(
        padding: EdgeInsets.symmetric(vertical: 16),
        child: Center(child: CircularProgressIndicator()),
      );
    } else if (_posts[tabIndex].isEmpty) {
      return const Center(child: Text('暂无内容'));
    } else {
      return const Padding(
        padding: EdgeInsets.symmetric(vertical: 16),
        child: Center(child: Text('没有更多内容了')),
      );
    }
  }

  Widget _buildContentCard(
      String category, IconData icon, Map<String, dynamic> post) {
    //print(post);
    return Card(
      elevation: 1,
      margin: const EdgeInsets.only(bottom: 12),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                CircleAvatar(
                  radius: 20,
                  backgroundColor: Colors.blue.shade50,
                  backgroundImage: NetworkImage(post['avatar_image_url']==''
                      ? 'https://sdu-forum.oss-cn-qingdao.aliyuncs.com/test/avatar_defaut.jpg'
                      : post['avatar_image_url']),
                ),
                const SizedBox(width: 12),
                Text(
                  post['nickName']==''? '匿名用户':post['nickName'],
                  style: const TextStyle(
                    fontWeight: FontWeight.bold,
                    fontSize: 16,
                  ),
                ),
                const Spacer(),
                Text(
                  _formatTime(post['created_at']),
                  style: TextStyle(
                    color: Colors.grey.shade500,
                    fontSize: 12,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            TextButton(
              onPressed: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => PostDetailPage(
                      postId: post['id'],
                      user_id: post['user_id'].toString(),
                      current_user_id: widget.user_id,
                    ),
                  ),
                );
              },
              child: Text(
                post['content'] ?? '无内容',
                style: const TextStyle(
                  fontSize: 15,
                  height: 1.4,
                  color: Colors.black87,
                  
                ),
                maxLines: 2,
                overflow: TextOverflow.ellipsis,
              ),
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                TextButton(
                  onPressed: () {
                    print("Tag clicked");
                  },
                  child: Text(
                    "#$category#",
                    style: TextStyle(
                      color: Colors.blue.shade700,
                      fontSize: 13,
                    ),
                  ),
                ),
                Icon(
                  Icons.favorite_border,
                  size: 18,
                  color: Colors.grey.shade500,
                ),
                const SizedBox(width: 4),
                Text(
                  post['favorite_count'].toString(),
                  style: TextStyle(
                    color: Colors.grey.shade500,
                    fontSize: 13,
                  ),
                ),
                const SizedBox(width: 16),
                Icon(
                  Icons.chat_bubble_outline,
                  size: 18,
                  color: Colors.grey.shade500,
                ),
                const SizedBox(width: 4),
                Text(
                  post['comment_count'].toString(),
                  style: TextStyle(
                    color: Colors.grey.shade500,
                    fontSize: 13,
                  ),
                ),
                const Spacer(),
                Icon(
                  Icons.bookmark_border,
                  size: 18,
                  color: Colors.grey.shade500,
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  String _formatTime(String? timeString) {
    if (timeString == null) return '未知时间';

    try {
      final dateTime = DateTime.parse(timeString);
      final now = DateTime.now();
      final difference = now.difference(dateTime);

      if (difference.inDays > 0) {
        return '${difference.inDays}天前';
      } else if (difference.inHours > 0) {
        return '${difference.inHours}小时前';
      } else if (difference.inMinutes > 0) {
        return '${difference.inMinutes}分钟前';
      } else {
        return '刚刚';
      }
    } catch (e) {
      return '未知时间';
    }
  }
}
