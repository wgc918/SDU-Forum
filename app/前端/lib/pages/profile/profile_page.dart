import 'dart:convert';
import 'package:intl/intl.dart';
import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/forum/detail_page.dart';
import 'package:sduforum/pages/profile/settings_page.dart';

class ProfilePage extends StatefulWidget {
  final String user_id;
  final bool isCurrentUser;
  const ProfilePage(
      {Key? key, required this.user_id, required this.isCurrentUser})
      : super(key: key);

  @override
  State<ProfilePage> createState() => _ProfilePageState();
}

class _ProfilePageState extends State<ProfilePage> {
  UserProfile? _user;
  bool _isLoading = true;
  String _errorMessage = '';

  String _errorMessage_post = '';

  bool postLoading = true;
  bool collectionLoading = true;

  int _selectedTabIndex = 0;
  final List<ProfileTabItem> _tabs = [
    ProfileTabItem(icon: Icons.edit_note, label: '我的帖子'),
    ProfileTabItem(icon: Icons.favorite_border, label: '我的收藏'),
  ];

  final List<PostItem> _myPosts = [];

  final List<PostItem> _myCollections = [];

  @override
  void initState() {
    super.initState();
    _fetchUserProfile();
    _fetchMyPost();
  }

  Future<void> _fetchUserProfile() async {
    setState(() {
      _isLoading = true;
      _errorMessage = '';
    });

    try {
      Dio dio = Dio();
      final response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/user/get_user_profile',
        queryParameters: {'user_id': widget.user_id},
      );
      //print(response.statusCode);
      print('成功$response.data');
      if (response.statusCode == 200) {
        var jsonData =
            response.data is String ? jsonDecode(response.data) : response.data;
        setState(() {
          _user = UserProfile.fromJson(jsonData);
          _isLoading = false;
        });
        //print('成功$response.data');
      } else {
        setState(() {
          _isLoading = false;
          _errorMessage = '加载用户信息失败，请稍后再试。';
        });
      }
    } catch (e) {
      //print('加载用户信息失败: $e');
      setState(() {
        _isLoading = false;
        _errorMessage = '加载用户信息失败，请稍后再试。';
      });
      return;
    }
  }

  Future<void> _fetchMyPost() async {
    setState(() {
      postLoading = true;
    });

    try {
      Dio dio = Dio();
      final response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/user/get_post_user_list',
        queryParameters: {'user_id': widget.user_id},
      );
      if (response.statusCode == 200) {
        var jsonData =
            response.data is String ? jsonDecode(response.data) : response.data;
        List<PostItem> posts = (jsonData as List).map((item) {
          return PostItem(
            id: item['id'],
            author_id: item['author_id'],
            user_id: item['user_id'].toString(),
            content: item['content'],
            time: item['created_at'],
            likeCount: item['favorite_count'],
            commentCount: item['comment_count'],
            avatar: ' ',
            addTime: ' ',
            nickName: ' ',
          );
        }).toList();
        setState(() {
          _myPosts.clear();
          _myPosts.addAll(posts);
          postLoading = false;
        });
      } else {
        setState(() {
          postLoading = false;
          //print(111122);
          _errorMessage_post = '加载帖子失败，请稍后再试。';
        });
      }
    } catch (e) {
      setState(() {
        postLoading = false;
        //print(222233);
        //print(e);
        _errorMessage_post = '加载帖子失败，请稍后再试。';
      });
      return;
    }
  }

  Future<void> _fetchMyCollection() async {
    setState(() {
      collectionLoading = true;
    });
    try {
      Dio dio = Dio();
      var response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/user/get_post_favorite_list',
        queryParameters: {'user_id': widget.user_id},
      );
      if (response.statusCode == 200) {
        var jsonData =
            response.data is String ? jsonDecode(response.data) : response.data;
        List<PostItem> collections = (jsonData['result'] as List).map((item) {
          //print(item);
          return PostItem(
            id: item['id'],
            author_id: item['author_id'],
            user_id: item['user_id'].toString(),
            content: item['content'],
            time: item['created_at'],
            likeCount: item['favorite_count'],
            commentCount: item['comment_count'],
            avatar: item['avatar_image_url'] == ''
                ? 'https://sdu-forum.oss-cn-qingdao.aliyuncs.com/test/avatar_defaut.jpg'
                : item['avatar_image_url'],
            addTime: item['add_at'],
            nickName: item['nickName'] == '' ? '匿名用户' : item['nickName'],
          );
        }).toList();
        setState(() {
          _myCollections.clear();
          _myCollections.addAll(collections);
          collectionLoading = false;
        });
      } else {
        setState(() {
          print(1111);
          print(response.statusCode);
          collectionLoading = false;
          _errorMessage_post = '加载收藏失败，请稍后再试。';
        });
      }
    } catch (e) {
      setState(() {
        print(2222);
        print(e);
        collectionLoading = false;
        _errorMessage_post = '加载收藏失败，请稍后再试。';
      });
      return;
    }
  }

  Future<void> _deletePost(int postId) async {
    try {
      Dio dio = Dio();
      var response = await dio.post(
        'http://120.26.127.37:8080/sdu_forum/api/user/delete_post',
        data: {'post_id': postId, 'user_id': widget.user_id},
      );
      if (response.statusCode == 200) {
        setState(() {
          _myPosts.removeWhere((post) => post.id == postId);
          _myCollections.removeWhere((post) => post.id == postId);
        });
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('帖子已删除')),
        );
      } else {
        setState(() {
          _errorMessage_post = '删除帖子失败，请稍后再试。';
        });
      }
    } catch (e) {
      print('删除帖子失败: $e');
      setState(() {
        _errorMessage_post = '删除帖子失败，请稍后再试。';
      });
      return;
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_isLoading) {
      return const Scaffold(
        body: Center(child: CircularProgressIndicator()),
      );
    }

    if (_errorMessage.isNotEmpty) {
      return Scaffold(
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(_errorMessage),
              ElevatedButton(
                onPressed: _fetchUserProfile,
                child: const Text('重试'),
              ),
            ],
          ),
        ),
      );
    }

    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final textTheme = theme.textTheme;

    return Scaffold(
      body: RefreshIndicator(
          onRefresh: _fetchUserProfile,
          child: CustomScrollView(
            slivers: [
              SliverAppBar(
                expandedHeight: 280,
                pinned: true,
                flexibleSpace: FlexibleSpaceBar(
                  background: _buildProfileHeader(context),
                  collapseMode: CollapseMode.pin,
                ),
                actions: [
                  if (widget.isCurrentUser)
                    IconButton(
                      icon: const Icon(Icons.settings),
                      onPressed: () {
                        // 跳转到设置页面
                        Navigator.push(
                            context,
                            MaterialPageRoute(
                                builder: (_) => SettingsPage(
                                      user_id: widget.user_id,
                                      profile: _user!,
                                    )));
                      },
                    ),
                ],
              ),
              SliverToBoxAdapter(
                child: Padding(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        _user!.major,
                        style: textTheme.bodyLarge?.copyWith(
                          color: colorScheme.onSurface.withOpacity(0.8),
                        ),
                      ),
                      const SizedBox(height: 16),
                      _buildStatsRow(),
                      const SizedBox(height: 24),
                    ],
                  ),
                ),
              ),
              SliverPadding(
                padding: const EdgeInsets.symmetric(horizontal: 20),
                sliver: SliverGrid(
                  gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: 2,
                    mainAxisSpacing: 8,
                    crossAxisSpacing: 8,
                    childAspectRatio: 3.5,
                  ),
                  delegate: SliverChildBuilderDelegate(
                    (context, index) {
                      return _buildProfileTab(index);
                    },
                    childCount: _tabs.length,
                  ),
                ),
              ),
              const SliverToBoxAdapter(
                child: SizedBox(height: 16),
              ),
              SliverPadding(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                sliver: _selectedTabIndex == 0
                    ? _buildPostList(_myPosts)
                    : _buildPostList(_myCollections),
              ),
            ],
          )),
    );
  }

  Widget _buildProfileHeader(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final textTheme = theme.textTheme;

    return Stack(
      children: [
        Container(
          height: 180,
          decoration: BoxDecoration(
            image: DecorationImage(
              image: NetworkImage(
                  "${_user!.backgroundImageUrl}?t=${DateTime.now().millisecondsSinceEpoch}"),
              fit: BoxFit.cover,
            ),
          ),
        ),
        Positioned(
          top: 100,
          left: 0,
          right: 0,
          child: Column(
            children: [
              Stack(
                alignment: Alignment.bottomRight,
                children: [
                  Container(
                    width: 120,
                    height: 120,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      border: Border.all(
                        color: colorScheme.surface,
                        width: 4,
                      ),
                      boxShadow: [
                        BoxShadow(
                          color: Colors.black.withOpacity(0.1),
                          blurRadius: 8,
                          offset: const Offset(0, 4),
                        ),
                      ],
                    ),
                    child: ClipOval(
                      child: Image.network(
                        "${_user!.avatarImageUrl}?t=${DateTime.now().millisecondsSinceEpoch}",
                        fit: BoxFit.cover,
                      ),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 10),
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Text(
                    _user!.nickName,
                    style: textTheme.headlineSmall?.copyWith(
                      fontWeight: FontWeight.bold,
                      color: colorScheme.onSurface,
                    ),
                  ),
                ],
              ),

              // 修改后的加入日期显示 - 增加上边距
              Padding(
                padding: const EdgeInsets.only(top: 4),
                child: Text(
                  _user!.joinDate,
                  style: textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurface.withOpacity(0.8), // 提高不透明度
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildStatsRow() {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        _buildStatItem(
          value: _user!.publishedPostCount.toString(),
          label: '帖子',
          color: colorScheme.primary,
        ),
        _buildStatItem(
          value: _user!.likedCount.toString(),
          label: '获赞',
          color: Colors.red,
        ),
        _buildStatItem(
          value: _user!.favoritePostCount.toString(),
          label: '收藏',
          color: Colors.amber,
        ),
      ],
    );
  }

  Widget _buildStatItem({
    required String value,
    required String label,
    required Color color,
  }) {
    final theme = Theme.of(context);
    final textTheme = theme.textTheme;

    return Column(
      children: [
        Text(
          value,
          style: textTheme.titleLarge?.copyWith(
            fontWeight: FontWeight.bold,
            color: color,
          ),
        ),
        Text(
          label,
          style: textTheme.bodySmall?.copyWith(
            color: theme.colorScheme.onSurface.withOpacity(0.6),
          ),
        ),
      ],
    );
  }

  Widget _buildProfileTab(int index) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final tab = _tabs[index];

    return GestureDetector(
      onTap: () {
        if (index == 0) {
          _fetchMyPost();
        } else {
          _fetchMyCollection();
        }
        setState(() {
          _selectedTabIndex = index;
        });
      },
      child: Container(
        decoration: BoxDecoration(
          color: _selectedTabIndex == index
              ? colorScheme.primary.withOpacity(0.1)
              : colorScheme.surfaceVariant.withOpacity(0.3),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: _selectedTabIndex == index
                ? colorScheme.primary
                : Colors.transparent,
            width: 1.5,
          ),
        ),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              tab.icon,
              size: 24,
              color: _selectedTabIndex == index
                  ? colorScheme.primary
                  : colorScheme.onSurface.withOpacity(0.7),
            ),
            const SizedBox(height: 4),
            Text(
              tab.label,
              style: theme.textTheme.bodyMedium?.copyWith(
                color: _selectedTabIndex == index
                    ? colorScheme.primary
                    : colorScheme.onSurface.withOpacity(0.7),
                fontWeight: _selectedTabIndex == index
                    ? FontWeight.bold
                    : FontWeight.normal,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPostList(List<PostItem> posts) {
    if (postLoading && collectionLoading) {
      return const SliverToBoxAdapter(
        child: Center(child: CircularProgressIndicator()),
      );
    }
    if (posts.isEmpty) {
      return SliverToBoxAdapter(
        child: Center(
          child: Text(
            _selectedTabIndex == 0 ? '暂无帖子' : '暂无收藏',
            style: TextStyle(
              color: Colors.grey.shade600,
              fontSize: 16,
            ),
          ),
        ),
      );
    }
    if (_errorMessage_post.isNotEmpty) {
      return SliverToBoxAdapter(
        child: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(_errorMessage_post),
              ElevatedButton(
                onPressed: () {
                  if (_selectedTabIndex == 0) {
                    _fetchMyPost();
                  } else {
                    _fetchMyCollection();
                  }
                },
                child: const Text('重试'),
              ),
            ],
          ),
        ),
      );
    }
    return SliverList(
      delegate: SliverChildBuilderDelegate(
        (context, index) {
          final post = posts[index];
          return _buildPostItem(post);
        },
        childCount: posts.length,
      ),
    );
  }

  Widget _buildPostItem(PostItem post) {
    Theme.of(context);
    String url, nickName, time;
    bool isPost = false;
    if (_selectedTabIndex == 0) {
      url =
          "${_user!.avatarImageUrl}?t=${DateTime.now().millisecondsSinceEpoch}";
      nickName = _user!.nickName;
      time = post.time;
      isPost = true;
    } else {
      url = "${post.avatar}?t=${DateTime.now().millisecondsSinceEpoch}";
      nickName = post.nickName;
      time = post.addTime;
    }
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
                  backgroundImage: NetworkImage(url),
                  radius: 20,
                ),
                const SizedBox(width: 12),
                Text(
                  nickName,
                  style: const TextStyle(
                    fontWeight: FontWeight.bold,
                    fontSize: 16,
                  ),
                ),
                const Spacer(),
                Text(
                  formatTime(time),
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
                      postId: post.id,
                      user_id: post.user_id,
                      current_user_id: widget.user_id,
                    ),
                  ),
                );
              },
              style: TextButton.styleFrom(
                padding: EdgeInsets.zero,
                minimumSize: Size.zero,
                tapTargetSize: MaterialTapTargetSize.shrinkWrap,
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    post.content,
                    style: TextStyle(
                      fontSize: 14,
                      height: 1.4,
                      color: Colors.black87.withOpacity(0.8),
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                // TextButton(
                //   onPressed: () {
                //     // 标签点击事件
                //   },
                //   style: TextButton.styleFrom(
                //     padding: EdgeInsets.zero,
                //     minimumSize: Size.zero,
                //     tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                //   ),
                //   child: Text(
                //     "#校园生活#",
                //     style: TextStyle(
                //       color: colorScheme.primary,
                //       fontSize: 13,
                //     ),
                //   ),
                // ),
                Icon(
                  Icons.favorite_border,
                  size: 18,
                  color: Colors.grey.shade500,
                ),
                const SizedBox(width: 4),
                Text(
                  post.likeCount.toString(),
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
                  post.commentCount.toString(),
                  style: TextStyle(
                    color: Colors.grey.shade500,
                    fontSize: 13,
                  ),
                ),
                const Spacer(),
                Icon(
                  _selectedTabIndex == 0
                      ? Icons.bookmark_border
                      : Icons.bookmark,
                  size: 18,
                  color: Colors.grey.shade500,
                ),

                if(widget.isCurrentUser&&isPost)
                const SizedBox(width: 8),
                if (widget.isCurrentUser&&isPost)
                IconButton(
                  icon: const Icon(Icons.delete),
                  iconSize: 18,
                  color: Colors.grey.shade500,
                  onPressed: () {
                    _showDeleteConfirmationDialog(context, post.id);
                  },
                ),

              ],
            ),
          ],
        ),
      ),
    );
  }

  void _showDeleteConfirmationDialog(BuildContext context, int postId) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text("确认删除"),
          content: const Text("确定要删除这条内容吗？"),
          actions: [
            TextButton(
              child: const Text("取消"),
              onPressed: () {
                Navigator.of(context).pop();
              },
            ),
            TextButton(
              child: const Text("删除"),
              onPressed: () {
                _deletePost(postId); // 调用删除方法
                Navigator.of(context).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class UserProfile {
  final String nickName;
  final String major;
  final String avatarImageUrl;
  final String joinDate;
  final String backgroundImageUrl;
  final int publishedPostCount;
  final int likedCount;
  final int favoritePostCount;

  UserProfile({
    required this.nickName,
    required this.major,
    required this.avatarImageUrl,
    required this.joinDate,
    required this.backgroundImageUrl,
    required this.publishedPostCount,
    required this.likedCount,
    required this.favoritePostCount,
  });

  factory UserProfile.fromJson(Map<String, dynamic> json) {
    return UserProfile(
      nickName: json['nickName'] == '' ? '未设置昵称' : json['nickName'],
      major: json['major'] == '' ? '未编辑个人描述' : json['major'],
      joinDate: formatJoinDate(json['join_date']),
      avatarImageUrl: json['avatar_image_url'] == ''
          ? 'https://sdu-forum.oss-cn-qingdao.aliyuncs.com/test/avatar_defaut.jpg'
          : json['avatar_image_url'],
      backgroundImageUrl: json['background_image_url'] == ''
          ? 'https://sdu-forum.oss-cn-qingdao.aliyuncs.com/test/bg_defaut.jpg'
          : json['background_image_url'],
      publishedPostCount: json['published_post_count'],
      likedCount: json['liked_count'],
      favoritePostCount: json['favorite_post_count'],
    );
  }
}

class ProfileTabItem {
  final IconData icon;
  final String label;

  ProfileTabItem({
    required this.icon,
    required this.label,
  });
}

class PostItem {
  final int id;
  final int author_id;
  final String user_id;
  final String nickName;
  final String content;
  final String avatar;
  final String time;
  final String addTime;
  final int likeCount;
  final int commentCount;

  PostItem({
    required this.id,
    required this.author_id,
    required this.user_id,
    required this.nickName,
    required this.content,
    required this.avatar,
    required this.time,
    required this.addTime,
    required this.likeCount,
    required this.commentCount,
  });
}

String formatJoinDate(String dateString) {
  DateTime dateTime = DateTime.parse(dateString);
  // 格式化为 年-月 字符串
  String formattedDate = DateFormat('yyyy-MM').format(dateTime);
  return '$formattedDate加入';
}

String formatTime(String timeString) {
  try {
    DateTime postTime = DateTime.parse(timeString);
    DateTime now = DateTime.now();
    Duration difference = now.difference(postTime);

    if (difference.inDays > 100) {
      return postTime.toString().substring(0, 10);
    }
    if (difference.inDays > 30) {
      return "${(difference.inDays / 30).floor()}月前";
    }

    if (difference.inDays > 0) {
      return "${difference.inDays}天前";
    } else if (difference.inHours > 0) {
      return "${difference.inHours}小时前";
    } else if (difference.inMinutes > 0) {
      return "${difference.inMinutes}分钟前";
    } else {
      return "刚刚";
    }
  } catch (e) {
    return timeString;
  }
}
