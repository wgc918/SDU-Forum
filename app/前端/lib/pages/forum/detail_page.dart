import 'dart:convert';

import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/forum/fullScreenImage_page.dart';
import 'package:sduforum/pages/profile/profile_page.dart';

class PostDetail {
  final int authorId;
  int commentCount;
  final String content;
  final String createdAt;
  int favoriteCount;
  bool isLiked;
  bool isFavorited;
  final List<String> images;
  final List<String> tags;
  final String avatarImageUrl;
  final String nickName;

  PostDetail({
    required this.authorId,
    required this.commentCount,
    required this.content,
    required this.createdAt,
    required this.favoriteCount,
    required this.isLiked,
    required this.isFavorited,
    required this.images,
    required this.tags,
    required this.avatarImageUrl,
    required this.nickName,
  });

  factory PostDetail.fromJson(Map<String, dynamic> json) {
    final baseData = json['base_data'];
    final userProfile = json['user_profile'];

    return PostDetail(
      authorId: baseData['author_id'],
      commentCount: baseData['comment_count'],
      content: baseData['content'],
      createdAt: baseData['created_at'],
      favoriteCount: baseData['favorite_count'],
      isLiked: json['like_state'],
      isFavorited: json['favorite_state'],
      images: List<String>.from(json['images'] ?? []),
      tags: List<String>.from(json['tags'] ?? []),
      avatarImageUrl: userProfile['avatar_image_url'],
      nickName: userProfile['nickName'],
    );
  }
}

class Comment {
  final int id;
  final String nickName;
  final String user_id;
  final String text;
  final String releaseTime;
  final String? to_nickName;
  final int? parentId;
  final String avatar;
  int likeNum;
  bool isLiked;
  final List<Comment> replies;

  Comment({
    required this.id,
    required this.nickName,
    required this.user_id,
    required this.text,
    required this.releaseTime,
    this.to_nickName,
    this.parentId,
    required this.isLiked,
    required this.likeNum,
    required this.avatar,
    required this.replies,
  });

  factory Comment.fromJson(Map<String, dynamic> json) {
    return Comment(
      id: json['id'],
      nickName: json['nickName']==''? '匿名用户' : json['nickName'],
      user_id: json['user_id'].toString(),
      text: json['text'],
      releaseTime: json['releaseTime'],
      to_nickName: json['to_nickName'] ?? 'null',
      parentId: json['father'] ?? 0,
      likeNum: json['like_num'] ?? 0,
      avatar: json['avatar_image_url'],
      isLiked: false,
      replies: [],
    );
  }
}

class PostDetailPage extends StatefulWidget {
  final int postId;
  final String user_id;
  final String current_user_id;
  const PostDetailPage(
      {super.key,
      required this.postId,
      required this.user_id,
      required this.current_user_id});

  @override
  State<PostDetailPage> createState() => _PostDetailPageState();
}

class _PostDetailPageState extends State<PostDetailPage> {
  PostDetail? postDetail;
  List<Comment> comments = [];
  bool isLoading = true;
  bool hasError = false;

  //贴子是否被点赞和收藏
  bool _isLiked = false;
  bool _isFavorited = false;

  final TextEditingController _commentController = TextEditingController();
  String _commentText = '';

  Comment? _replyingToComment;

  @override
  void initState() {
    super.initState();
    loadPostData();
    loadComments();
  }

  @override
  void dispose() {
    super.dispose();
    _commentController.dispose();
  }

  void loadPostData() async {
    try {
      Dio dio = Dio();
      Response response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/user/get_post_details',
        queryParameters: {
          'post_id': widget.postId,
          'user_id': widget.current_user_id,
        },
      );
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        setState(() {
          postDetail = PostDetail.fromJson(data);
          _isLiked = data['like_state'] ?? false;
          _isFavorited = data['favorite_state'] ?? false;
          isLoading = false;
        });
      }
    } catch (e) {
      // 处理异常
      setState(() {
        isLoading = false;
        hasError = true;
      });
      print("加载数据失败: $e");
    }
  }

  void loadComments() async {
    try {
      Dio dio = Dio();
      Response response = await dio.get(
        'http://120.26.127.37:8080/sdu_forum/api/comment/getComment',
        queryParameters: {
          'post_id': widget.postId,
        },
      );
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        List<Comment> loadedComments = [];

        for (var commentGroup in data['results']) {
          // 添加主评论
          var firstComment = Comment.fromJson(commentGroup['firstComment']);

          // 添加回复评论
          List<Comment> replies = [];
          for (var reply in commentGroup['children']) {
            replies.add(Comment.fromJson(reply));
          }

          loadedComments.add(Comment(
            id: firstComment.id,
            nickName: firstComment.nickName,
            user_id: firstComment.user_id,
            text: firstComment.text,
            releaseTime: firstComment.releaseTime,
            parentId: 0,
            replies: replies,
            avatar: firstComment.avatar,
            likeNum: firstComment.likeNum,
            isLiked: false,
          ));
        }
        setState(() {
          comments = loadedComments;
        });
      }
    } catch (e) {
      // 处理异常
      print("加载评论失败: $e");
    }
  }

  void like_post() async {
    try {
      Dio dio = Dio();
      Response response = await dio
          .post('http://120.26.127.37:8080/sdu_forum/api/post/like', data: {
        'post_id': widget.postId,
        'user_id': widget.current_user_id,
        'author_user_id': widget.user_id,
      });
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;

        if (data['state'] == true) {
          // 使用服务器返回的最新点赞数
          setState(() {
            postDetail?.favoriteCount =
                data['favorite_count'] ?? postDetail!.favoriteCount;
            _isLiked = true;
          });
        } else {
          // API调用失败，回滚状态
          setState(() {
            _isLiked = false;
            postDetail?.favoriteCount--;
          });
          print("点赞失败: ${data['message']}");
        }
      }
    } catch (e) {
      // 处理异常
      // API调用失败，回滚状态
      setState(() {
        _isLiked = false;
        postDetail?.favoriteCount--;
      });
      print("点赞帖子失败: $e");
    }
  }

  void unlike_post() async {
    try {
      Dio dio = Dio();
      Response response = await dio
          .post('http://120.26.127.37:8080/sdu_forum/api/post/unlike', data: {
        'post_id': widget.postId,
        'user_id': widget.current_user_id,
        'author_user_id': widget.user_id,
      });
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == true) {
          // 使用服务器返回的最新点赞数
          setState(() {
            postDetail?.favoriteCount =
                data['favorite_count'] ?? postDetail!.favoriteCount;
            _isLiked = false;
          });
        } else {
          // API调用失败，回滚状态
          setState(() {
            _isLiked = true;
            postDetail?.favoriteCount++;
          });
          print("取消点赞失败: ${data['message']}");
        }
      }
    } catch (e) {
      // 处理异常
      // API调用失败，回滚状态
      setState(() {
        _isLiked = true;
        postDetail?.favoriteCount++;
      });
      print("取消点赞帖子失败: $e");
    }
  }

  void like_comment(Comment comment) async {
    try {
      Dio dio = Dio();
      Response response = await dio.post(
        'http://120.26.127.37:8080/sdu_forum/api/comment/likeComment',
        data: {
          'id': comment.id,
          'post_id': widget.postId,
        },
      );
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == true) {
          setState(() {
            comment.isLiked = true;
            comment.likeNum = data['count'] ?? comment.likeNum + 1;
          });
        } else {
          print("点赞评论失败: ");
        }
      }
    } catch (e) {
      // 处理异常
      print("点赞评论失败: $e");
    }
  }

  void unlike_comment(Comment comment) async {
    try {
      Dio dio = Dio();
      Response response = await dio.post(
        'http://120.26.127.37:8080/sdu_forum/api/comment/unlikeComment',
        data: {
          'id': comment.id,
          'post_id': widget.postId,
        },
      );
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == true) {
          setState(() {
            comment.isLiked = false;
            comment.likeNum = data['count'] ?? comment.likeNum - 1;
          });
        }
      }
    } catch (e) {
      // 处理异常
      print("取消点赞评论失败: $e");
    }
  }

  void favorite_post() async {
    try {
      Dio dio = Dio();
      print(widget.postId);
      print(widget.user_id);
      Response response = await dio.post(
          'http://120.26.127.37:8080/sdu_forum/api/post/add_post_favorite',
          data: {
            'post_id': widget.postId,
            'user_id': widget.current_user_id,
          });
      print(response.data);
      if (response.statusCode == 200) {
        if (response.data['state'] == true) {
          print("收藏帖子成功");
        } else {
          setState(() {
            _isFavorited = false;
          });
          print("1收藏帖子失败: ${response.data['info']}");
        }
      }
    } catch (e) {
      // 处理异常
      setState(() {
        _isFavorited = false; // 回滚状态
      });
      print("2收藏帖子失败: $e");
    }
  }

  void unfavorite_post() async {
    try {
      Dio dio = Dio();

      Response response = await dio.post(
          'http://120.26.127.37:8080/sdu_forum/api/post/sub_post_favorite',
          data: {
            'post_id': widget.postId,
            'user_id': widget.current_user_id,
          });
      print(widget.postId);
      print(widget.user_id);
      if (response.statusCode == 200) {
        if (response.data['state'] == true) {
          print("取消收藏帖子成功");
        } else {
          setState(() {
            _isFavorited = true;
          });
          print("1取消收藏帖子失败: ${response.data['info']}");
        }
      }
    } catch (e) {
      // 处理异常
      setState(() {
        _isFavorited = true; // 回滚状态
      });
      print("2取消收藏帖子失败: $e");
    }
  }

  void saveComment(String text,
      {int? father = 0, String to_nickName = ''}) async {
    try {
      //print("正在保存评论: $text, 父评论ID: $father, 回复用户: $to_nickName");
      Dio dio = Dio();
      Response response = await dio.post(
          'http://120.26.127.37:8080/sdu_forum/api/comment/saveComment',
          data: {
            'post_id': widget.postId,
            'user_id': widget.current_user_id,
            'text': text,
            'father': father,
            'to_nickName': to_nickName,
          });
      if (response.statusCode == 200) {
        var data =
            response.data is String ? jsonDecode(response.data) : response.data;
        if (data['state'] == true) {
          setState(() {
            _replyingToComment = null;
            _commentController.clear();
          });
          loadComments();
          print("评论保存成功");
        } else {
          print("评论保存失败: ${data['info']}");
        }
      }
    } catch (e) {
      print("保存评论失败: $e");
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.grey.shade50,
      appBar: AppBar(
        elevation: 0,
        backgroundColor: Colors.white,
        leading: IconButton(
          icon: Icon(Icons.arrow_back, color: Colors.grey.shade700),
          onPressed: () => Navigator.pop(context),
        ),
        title: Text(
          '帖子详情',
          style: TextStyle(
            color: Colors.grey.shade800,
            fontWeight: FontWeight.bold,
          ),
        ),
        actions: [
          IconButton(
            icon: Icon(Icons.more_vert, color: Colors.grey.shade700),
            onPressed: () {
              // 显示更多操作菜单
            },
          ),
        ],
      ),
      body: Column(
        children: [
          Expanded(
            child: SingleChildScrollView(
              child: Column(
                children: [
                  // 帖子内容卡片
                  _buildPostCard(context),
                  // 评论区
                  _buildCommentSection(),
                ],
              ),
            ),
          ),
          // 底部评论栏
          _buildBottomBar(),
        ],
      ),
    );
  }

  Widget _buildPostCard(BuildContext context) {
    if (isLoading) {
      return const Center(child: CircularProgressIndicator());
    }

    if (hasError) {
      return const Center(child: Text('加载失败，请重试'));
    }

    if (postDetail == null) {
      return const Center(child: Text('暂无数据'));
    }
    return Card(
      color: Colors.white,
      margin: const EdgeInsets.all(12),
      elevation: 1,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // 用户信息行
            Row(
              children: [
                InkWell(
                  onTap: () {
                    // 处理点击事件
                    Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => ProfilePage(
                          user_id: widget.user_id,
                          isCurrentUser: false,
                        ),
                      ),
                    );
                  },
                  customBorder: const CircleBorder(),
                  child: CircleAvatar(
                    radius: 24,
                    backgroundColor: Colors.blue.shade50,
                    backgroundImage: postDetail!.avatarImageUrl.isNotEmpty
                        ? NetworkImage(postDetail!.avatarImageUrl)
                        : null,
                    child: postDetail!.avatarImageUrl.isEmpty
                        ? Icon(
                            Icons.person_outline,
                            size: 24,
                            color: Colors.blue.shade700,
                          )
                        : null,
                  ),
                ),
                const SizedBox(width: 12),
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      postDetail!.nickName,
                      style: const TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 16,
                      ),
                    ),
                    Text(
                      "校园达人 · ${_formatTime(postDetail!.createdAt)}",
                      style: TextStyle(
                        color: Colors.grey.shade500,
                        fontSize: 12,
                      ),
                    ),
                  ],
                ),
                const Spacer(),
                IconButton(
                  icon: Icon(Icons.share_rounded, color: Colors.grey.shade700),
                  onPressed: () {
                    // 分享功能
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('分享功能尚未实现,敬请期待'),duration: Duration(seconds: 1)),
                    );
                  },
                ),
              ],
            ),

            const SizedBox(height: 16),

            // 帖子正文
            Text(
              postDetail!.content,
              style: const TextStyle(
                fontSize: 15,
                height: 1.6,
              ),
            ),

            const SizedBox(height: 12),

            // 图片展示区
            if (postDetail!.images.isNotEmpty)
              SizedBox(
                height: 200,
                child: ListView.builder(
                  scrollDirection: Axis.horizontal,
                  itemCount: postDetail!.images.length,
                  itemBuilder: (context, index) {
                    return GestureDetector(
                      onTap: () {
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (context) => FullScreenImageViewer(
                              images: postDetail!.images,
                              initialIndex: index,
                            ),
                          ),
                        );
                      },
                      child: Padding(
                        padding: const EdgeInsets.only(right: 8),
                        child: ClipRRect(
                          borderRadius: BorderRadius.circular(8),
                          child: Hero(
                            tag: 'image_$index',
                            child: Image.network(
                              postDetail!.images[index],
                              width: 200,
                              fit: BoxFit.cover,
                              errorBuilder: (context, error, stackTrace) {
                                return Container(
                                  width: 200,
                                  color: Colors.grey.shade200,
                                  child: const Icon(Icons.broken_image),
                                );
                              },
                            ),
                          ),
                        ),
                      ),
                    );
                  },
                ),
              ),

            const SizedBox(height: 16),

            // 标签和互动区域
            if (postDetail!.tags.isNotEmpty)
              Wrap(
                spacing: 8,
                children: postDetail!.tags
                    .map((tag) => _buildTagButton("#$tag"))
                    .toList(),
              ),
            if (postDetail!.tags.isNotEmpty) const SizedBox(height: 16),

            // 互动统计
            Row(
              children: [
                //点赞数
                _buildInteractionButton(
                  icon: _isLiked ? Icons.favorite : Icons.favorite_border,
                  count: postDetail!.favoriteCount,
                  active: _isLiked,
                  onPressed: () {
                    setState(() {
                      if (_isLiked) {
                        // 取消点赞
                        postDetail!.favoriteCount--;
                        _isLiked = false;
                        unlike_post();
                      } else {
                        // 点赞
                        postDetail!.favoriteCount++;
                        _isLiked = true;
                        like_post();
                      }
                    });
                  },
                ),
                // 评论数
                const SizedBox(width: 16),
                _buildInteractionButton(
                  icon: Icons.chat_bubble_outline,
                  count: postDetail!.commentCount,
                  active: false,
                ),
                // 收藏数
                const SizedBox(width: 16),
                IconButton(
                  onPressed: () {
                    setState(() {
                      if (_isFavorited) {
                        _isFavorited = false;
                        unfavorite_post();
                      } else {
                        _isFavorited = true;
                        favorite_post();
                      }
                    });
                  },
                  icon: Icon(_isFavorited ? Icons.star : Icons.star_border,
                      color: _isFavorited
                          ? Colors.yellow.shade700
                          : Colors.grey.shade500,
                      size: 25),
                ),
                // _buildInteractionButton(
                //   icon: _isFavorited ? Icons.star_border : Icons.star_border,
                //   count: 0,
                //   active: _isFavorited,
                //   onPressed: () {
                //     setState(() {
                //       if (_isFavorited) {
                //         _isFavorited = false;
                //         unfavorite_post();
                //       } else {
                //         _isFavorited = true;
                //         favorite_post();
                //       }
                //     });
                //   },
                // ),
                const Spacer(),
                Icon(
                  Icons.visibility_outlined,
                  size: 18,
                  color: Colors.grey.shade500,
                ),
                // 浏览量
                const SizedBox(width: 4),
                Text(
                  "${postDetail!.favoriteCount * 2 + postDetail!.commentCount * 3}",
                  style: TextStyle(
                    color: Colors.grey.shade500,
                    fontSize: 13,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTagButton(String tag) {
    return InkWell(
      onTap: () {},
      borderRadius: BorderRadius.circular(12),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        decoration: BoxDecoration(
          color: Colors.grey.shade100,
          borderRadius: BorderRadius.circular(12),
        ),
        child: Text(
          tag,
          style: TextStyle(
            color: Colors.blue.shade700,
            fontSize: 13,
          ),
        ),
      ),
    );
  }

  Widget _buildInteractionButton({
    required IconData icon,
    required int count,
    required bool active,
    VoidCallback? onPressed,
  }) {
    return InkWell(
      onTap: onPressed, // 添加点击事件
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            icon,
            size: 20,
            color: active ? Colors.red : Colors.grey.shade500,
          ),
          const SizedBox(width: 4),
          Text(
            "$count",
            style: TextStyle(
              color: active ? Colors.red : Colors.grey.shade500,
              fontSize: 13,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCommentSection() {
    if (isLoading) {
      return const Center(child: CircularProgressIndicator());
    }
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Padding(
            padding: const EdgeInsets.symmetric(vertical: 12),
            child: Text(
              "评论 (${postDetail?.commentCount})",
              style: const TextStyle(
                fontWeight: FontWeight.bold,
                fontSize: 16,
              ),
            ),
          ),

          // 评论列表
          ListView.separated(
            physics: const NeverScrollableScrollPhysics(),
            shrinkWrap: true,
            itemCount: comments.length,
            separatorBuilder: (context, index) => const SizedBox(height: 12),
            itemBuilder: (context, index) {
              return _buildCommentItem(comments[index]);
            },
          ),

          // TextButton(
          //   onPressed: () {},
          //   child: const Text("查看全部评论"),
          // ),
        ],
      ),
    );
  }

  Widget _buildCommentItem(Comment comment) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 评论用户信息
          Row(
            children: [
              CircleAvatar(
                  radius: 16,
                  backgroundColor: Colors.blue.shade50,
                  child: InkWell(
                    onTap: () {
                      // 处理头像点击事件
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => ProfilePage(
                            user_id: comment.user_id,
                            isCurrentUser: false,
                          ),
                        ),
                      );
                    },
                    borderRadius: BorderRadius.circular(16), // 圆形点击区域
                    child: comment.avatar.isNotEmpty
                        ? ClipOval(
                            child: Image.network(
                              comment.avatar,
                              width: 32,
                              height: 32,
                              fit: BoxFit.cover,
                              errorBuilder: (context, error, stackTrace) {
                                return Icon(
                                  Icons.person_outline,
                                  size: 32,
                                  color: Colors.blue.shade700,
                                );
                              },
                            ),
                          )
                        : Icon(
                            Icons.person_outline,
                            size: 32,
                            color: Colors.blue.shade700,
                          ),
                  )),
              const SizedBox(width: 8),
              Text(
                comment.nickName,
                style: const TextStyle(
                  fontWeight: FontWeight.bold,
                  fontSize: 14,
                ),
              ),
              const SizedBox(width: 8),
              Text(
                _formatTime(comment.releaseTime),
                style: TextStyle(
                  color: Colors.grey.shade500,
                  fontSize: 12,
                ),
              ),
            ],
          ),
          // 评论内容
          const SizedBox(height: 8),
          Text(
            comment.text,
            style: const TextStyle(
              fontSize: 14,
              height: 1.4,
            ),
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              Text(
                _formatTime(comment.releaseTime),
                style: TextStyle(
                  color: Colors.grey.shade500,
                  fontSize: 12,
                ),
              ),
              const Spacer(),
              _buildInteractionButton(
                icon: comment.isLiked ? Icons.favorite : Icons.favorite_border,
                count: comment.likeNum,
                active: comment.isLiked,
                onPressed: () {
                  if (comment.isLiked) {
                    unlike_comment(comment);
                  } else {
                    like_comment(comment);
                  }
                },
              ),
              const SizedBox(width: 16),
              TextButton(
                onPressed: () {
                  setState(() {
                    _replyingToComment = comment; // 设置正在回复的评论
                    _commentController.text = "@${comment.nickName} ";
                  });
                  // 滚动到底部
                  Future.delayed(const Duration(milliseconds: 100), () {
                    Scrollable.ensureVisible(
                      context,
                      alignment: 1.0,
                      duration: const Duration(milliseconds: 300),
                    );
                  });
                },
                style: TextButton.styleFrom(
                  padding: EdgeInsets.zero,
                  minimumSize: const Size(0, 0),
                  tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                ),
                child: const Text(
                  "回复",
                  style: TextStyle(fontSize: 13),
                ),
              ),
            ],
          ),
          // 显示回复评论
          if (comment.replies.isNotEmpty)
            Padding(
              padding: const EdgeInsets.only(left: 16),
              child: Column(
                children: comment.replies
                    .map((reply) => _buildReplyItem(reply))
                    .toList(),
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildBottomBar() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: Colors.white,
        boxShadow: [
          BoxShadow(
            color: Colors.grey.withOpacity(0.1),
            spreadRadius: 1,
            blurRadius: 3,
            offset: const Offset(0, -1),
          ),
        ],
      ),
      child: Column(
        children: [
          // 显示正在回复谁
          if (_replyingToComment != null)
            Container(
              padding: const EdgeInsets.only(bottom: 8),
              //alignment: Alignment.centerLeft,
              child: Row(
                children: [
                  Text(
                    "回复 ${_replyingToComment!.nickName}",
                    style: TextStyle(
                      color: Colors.blue.shade700,
                      fontSize: 13,
                    ),
                  ),
                  const Spacer(),
                  GestureDetector(
                    onTap: () {
                      setState(() {
                        _replyingToComment = null;
                        _commentController.clear();
                      });
                    },
                    child: Icon(
                      Icons.close,
                      size: 16,
                      color: Colors.grey.shade500,
                    ),
                  ),
                ],
              ),
            ),
          Row(
            children: [
              Expanded(
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12),
                  decoration: BoxDecoration(
                    color: Colors.grey.shade100,
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: TextField(
                    controller: _commentController,
                    decoration: InputDecoration(
                      hintText: _replyingToComment != null
                          ? "回复 ${_replyingToComment!.nickName}..."
                          : "写评论...",
                      hintStyle: TextStyle(color: Colors.grey.shade500),
                      border: InputBorder.none,
                      isDense: true,
                    ),
                  ),
                ),
              ),
              const SizedBox(width: 8),
              ElevatedButton(
                onPressed: () {
                  // 提交评论逻辑
                  setState(() {
                    _commentText = _commentController.text.trim();
                  });
                  if (_commentText.isEmpty) {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('评论内容不能为空')),
                    );
                    return;
                  }

                  if (_replyingToComment != null) {
                    // 如果是回复评论
                    saveComment(
                      _commentText,
                      father: _replyingToComment!.parentId == 0
                          ? _replyingToComment!.id
                          : _replyingToComment!.parentId,
                      to_nickName: _replyingToComment!.nickName,
                    );
                    //print(_replyingToComment!.parentId);
                    //print(_replyingToComment!.id);
                  } else {
                    // 如果是普通评论
                    saveComment(_commentText);
                  }
                },
                style: ElevatedButton.styleFrom(
                  foregroundColor: Colors.white,
                  backgroundColor: Colors.blue.shade700,
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(20),
                  ),
                ),
                child: const Text("发送"),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildReplyItem(Comment reply) {
    return Container(
      margin: const EdgeInsets.only(top: 8),
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: Colors.grey.shade100,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              CircleAvatar(
                  radius: 16,
                  backgroundColor: Colors.blue.shade50,
                  child: InkWell(
                    onTap: () {
                      // 处理头像点击事件
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => ProfilePage(
                            user_id: reply.user_id,
                            isCurrentUser: false,
                          ),
                        ),
                      );
                    },
                    borderRadius: BorderRadius.circular(16), // 圆形点击区域
                    child: reply.avatar.isNotEmpty
                        ? ClipOval(
                            child: Image.network(
                              reply.avatar,
                              width: 32,
                              height: 32,
                              fit: BoxFit.cover,
                              errorBuilder: (context, error, stackTrace) {
                                return Icon(
                                  Icons.person_outline,
                                  size: 32,
                                  color: Colors.blue.shade700,
                                );
                              },
                            ),
                          )
                        : Icon(
                            Icons.person_outline,
                            size: 32,
                            color: Colors.blue.shade700,
                          ),
                  )),
              const SizedBox(width: 8),
              Text(
                reply.nickName,
                style: const TextStyle(
                  fontWeight: FontWeight.bold,
                  fontSize: 14,
                ),
              ),
              if (reply.to_nickName != "null" &&
                  reply.to_nickName != null &&
                  reply.to_nickName!.isNotEmpty)
                Row(
                  children: [
                    const Text(
                      "->",
                      style: TextStyle(fontSize: 13),
                    ),
                    Text(
                      reply.to_nickName!,
                      style: const TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 13,
                      ),
                    ),
                  ],
                ),
            ],
          ),
          const SizedBox(height: 4),
          Text(
            reply.text,
            style: const TextStyle(
              fontSize: 13,
              height: 1.4,
            ),
          ),
          Row(
            children: [
              Text(
                _formatTime(reply.releaseTime),
                style: TextStyle(
                  color: Colors.grey.shade500,
                  fontSize: 12,
                ),
              ),
              const Spacer(),
              _buildInteractionButton(
                icon: reply.isLiked ? Icons.favorite : Icons.favorite_border,
                count: reply.likeNum,
                active: reply.isLiked,
                onPressed: () {
                  if (reply.isLiked) {
                    // 取消点
                    unlike_comment(reply);
                  } else {
                    // 点赞
                    like_comment(reply);
                  }
                },
              ),
              const SizedBox(width: 16),
              TextButton(
                onPressed: () {
                  setState(() {
                    _replyingToComment = reply; // 设置正在回复的评论
                    _commentController.text = "@${reply.nickName} ";
                  });
                  // 滚动到底部
                  Future.delayed(const Duration(milliseconds: 100), () {
                    Scrollable.ensureVisible(
                      context,
                      alignment: 1.0,
                      duration: const Duration(milliseconds: 300),
                    );
                  });
                },
                style: TextButton.styleFrom(
                  padding: EdgeInsets.zero,
                  minimumSize: const Size(0, 0),
                  tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                ),
                child: const Text(
                  "回复",
                  style: TextStyle(fontSize: 13),
                ),
              ),
            ],
          ),
        ],
      ),
    );
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
