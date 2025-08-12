import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/forum/detail_page.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:dio/dio.dart';

class SearchPage extends StatefulWidget {
  final String user_id;
  const SearchPage({super.key, required this.user_id});
  @override
  State<SearchPage> createState() => _SearchPageState();
}

class _SearchPageState extends State<SearchPage> {
  final TextEditingController _searchController = TextEditingController();
  final FocusNode _focusNode = FocusNode();
  bool _isSearching = false;
  List<String> _searchHistory = [];
  final List<String> _hotKeywords = ["二手闲置", "打听求助", "学习搭子", "社团活动", "校园招聘"];
  List<dynamic> _searchResults = [];

  late SharedPreferences _prefs;

  List<dynamic> _hotPosts = [];

  @override
  void initState() {
    super.initState();

    _initPreferences();

    _fetchTopPosts();

    _getHotTags();
  }

  Future<void> _initPreferences() async {
    _prefs = await SharedPreferences.getInstance();
    _loadSearchHistory();
  }

  void _loadSearchHistory() {
    final history = _prefs.getStringList('searchHistory') ?? [];
    setState(() {
      _searchHistory = history;
    });
  }

  @override
  void dispose() {
    _searchController.dispose();
    _focusNode.dispose();
    super.dispose();
  }

  void _getHotTags() async {
    try {
      Dio dio = Dio();
      var response = await dio
          .get('http://120.26.127.37:8080/sdu_forum/api/search/get_hot_tag');
      if (response.statusCode == 200) {
        setState(() {
          var data = response.data is String
              ? jsonDecode(response.data)
              : response.data;
          if (data['state'] == 'ok') {
            _hotKeywords.clear();
            _hotKeywords.addAll(List<String>.from(data['result']));
          }
        });
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('获取热门标签失败: $e')),
      );
      print('获取热门标签失败: $e');
    }
  }

  void _search(String keyword) async {
    if (keyword.isEmpty) return;

    setState(() {
      _isSearching = true;
      //_searchResults.clear(); // 清空之前的搜索结果
    });

    try {
      Dio dio = Dio();
      var response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/search/all_text',
        queryParameters: {'key': keyword, 'start': 0, 'pageSize': 30},
      );
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == 'ok') {
          setState(() {
            _searchResults = data['result'];
          });
        }
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('搜索失败: $e')),
      );
      print('搜索失败: $e');
      return;
    }

    if (!_searchHistory.contains(keyword)) {
      _searchHistory.insert(0, keyword);

      if (_searchHistory.length > 10) {
        _searchHistory = _searchHistory.sublist(0, 10);
      }

      await _prefs.setStringList('searchHistory', _searchHistory);
    }
  }

  Future<void> _fetchTopPosts() async {
    try {
      Dio dio = Dio();
      var response =
          await dio.get('http://120.26.127.37:8080/sdu_forum/api/search/top10');
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == 'ok') {
          setState(() {
            _hotPosts = data['result'];
          });
        }
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('获取热门帖子失败: $e')),
      );

      print('获取热门帖子失败:$e');
    }
  }

  void _clearSearch() {
    setState(() {
      _searchController.clear();
      _isSearching = false;
    });
  }

  void _removeHistoryItem(int index) async {
    setState(() {
      _searchHistory.removeAt(index);
    });
    await _prefs.setStringList('searchHistory', _searchHistory);
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => FocusScope.of(context).unfocus(),
      behavior: HitTestBehavior.translucent,
      child: Scaffold(
        appBar: AppBar(
          title: _buildSearchBar(),
          automaticallyImplyLeading: false,
        ),
        body: _isSearching ? _buildSearchResults() : _buildSearchSuggestions(),
      ),
    );
  }

  Widget _buildSearchBar() {
    return Container(
      height: 40,
      decoration: BoxDecoration(
        color: Colors.grey[200],
        borderRadius: BorderRadius.circular(20),
      ),
      child: TextField(
        controller: _searchController,
        focusNode: _focusNode,
        autofocus: !_isSearching,
        decoration: InputDecoration(
          hintText: "搜索...",
          border: InputBorder.none,
          prefixIcon: const Icon(Icons.search, color: Colors.grey),
          suffixIcon: _searchController.text.isNotEmpty
              ? IconButton(
                  icon: const Icon(Icons.close, color: Colors.grey),
                  onPressed: _clearSearch,
                )
              : null,
        ),
        onSubmitted: _search,
      ),
    );
  }

  Widget _buildSearchSuggestions() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (_searchHistory.isNotEmpty) ...[
            const Text("历史搜索", style: TextStyle(fontWeight: FontWeight.bold)),
            const SizedBox(height: 8),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: _searchHistory.asMap().entries.map((entry) {
                int index = entry.key;
                String keyword = entry.value;
                return InkWell(
                  onTap: () {
                    _searchController.text = keyword;
                    _search(keyword);
                  },
                  child: Chip(
                    label: Text(keyword),
                    onDeleted: () => _removeHistoryItem(index),
                  ),
                );
              }).toList(),
            ),
            const Divider(height: 32),
          ],
          const Text("热门搜索", style: TextStyle(fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: _hotKeywords
                .map((keyword) => ActionChip(
                      label: Text(keyword),
                      onPressed: () {
                        _searchController.text = keyword;
                        _searchByTag(keyword);
                      },
                    ))
                .toList(),
          ),
          const SizedBox(height: 24),
          const Text("热门帖子",
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16)),
          const SizedBox(height: 12),
          ..._hotPosts.map((post) => _buildPostItem(post)).toList(),
        ],
      ),
    );
  }

  // 构建单个帖子项
  Widget _buildPostItem(dynamic post) {
    print(post);
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
      child: InkWell(
        borderRadius: BorderRadius.circular(10),
        onTap: () {
          // 跳转到帖子详情页
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
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  CircleAvatar(
                    backgroundImage: NetworkImage(post['avatar_image_url']),
                    radius: 16,
                  ),
                  const SizedBox(width: 8),
                  Text(
                    post['nickName'],
                    style: const TextStyle(fontWeight: FontWeight.bold),
                  ),
                  const Spacer(),
                  Text(
                    _formatTime(post['created_at']),
                    style: TextStyle(color: Colors.grey[600], fontSize: 12),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Text(
                post['content'],
                style: const TextStyle(fontSize: 16),
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  Icon(Icons.remove_red_eye, size: 16, color: Colors.grey[600]),
                  const SizedBox(width: 4),
                  Text(
                    '${post['favorite_count'] * 2 + post['comment_count'] * 3}',
                    style: TextStyle(color: Colors.grey[600], fontSize: 12),
                  ),
                  const SizedBox(width: 12),
                  Icon(Icons.favorite_border,
                      size: 16, color: Colors.grey[600]),
                  const SizedBox(width: 4),
                  Text(
                    '${post['favorite_count']}',
                    style: TextStyle(color: Colors.grey[600], fontSize: 12),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildSearchResults() {
    if (_searchResults.isEmpty) {
      return Center(
        child: Text(
          "没有找到相关结果",
          style: TextStyle(color: Colors.grey[600], fontSize: 16),
        ),
      );
    }
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: _searchResults.length,
      itemBuilder: (context, index) {
        final post = _searchResults[index];
        return _buildPostItem(post);
      },
    );
  }

  void _searchByTag(String tag) async {
    setState(() {
      _isSearching = true;
    });
    try {
      Dio dio = Dio();
      var response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/search/tag',
        queryParameters: {'tag': tag, 'start': 0, 'pageSize': 30},
      );
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == 'ok') {
          setState(() {
            _searchResults = data['result'];
          });
        }
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('搜索失败: $e')),
      );
      print('搜索失败: $e');
    }
  }

  String _formatTime(String timeString) {
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
}
