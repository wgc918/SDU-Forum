import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:intl/intl.dart';

class MessagePage extends StatefulWidget {
  const MessagePage({Key? key}) : super(key: key);

  @override
  State<MessagePage> createState() => _MessagePageState();
}

class _MessagePageState extends State<MessagePage>
    with SingleTickerProviderStateMixin {
  late TabController _tabController;
  final List<String> _tabs = ['通知', '点赞', '收藏', '评论'];

  final List<Message> _notifications = [
    Message(
      type: 'system',
      title: '系统通知',
      content: '您的账号已通过学生认证，现在可以发布社团活动了。',
      time: DateTime.now().subtract(const Duration(minutes: 30)),
      read: false,
      avatar: 'assets/icons/system_notification.svg',
    ),
    Message(
      type: 'post',
      title: '帖子审核',
      content: '您发布的"编程学习小组招募"已通过审核，现在可以查看了。',
      time: DateTime.now().subtract(const Duration(hours: 2)),
      read: true,
      avatar: 'assets/icons/post_approved.svg',
    ),
    Message(
      type: 'activity',
      title: '活动提醒',
      content: '您报名的"校园编程大赛"将于明天下午3点开始，请准时参加。',
      time: DateTime.now().subtract(const Duration(days: 1)),
      read: true,
      avatar: 'assets/icons/activity_reminder.svg',
    ),
  ];

  final List<Message> _likes = [
    Message(
      type: 'like',
      title: '点赞通知',
      content: '用户"Flutter爱好者"点赞了您的帖子"Dart编程技巧分享"。',
      time: DateTime.now().subtract(const Duration(hours: 3)),
      read: false,
      avatar: 'assets/images/user1.jpg',
      postPreview: '这篇帖子分享了一些Dart语言的高级用法和技巧...',
    ),
    Message(
      type: 'like',
      title: '点赞通知',
      content: '用户"前端开发"点赞了您的评论"这个方法确实很实用"。',
      time: DateTime.now().subtract(const Duration(days: 1)),
      read: true,
      avatar: 'assets/images/user2.jpg',
      postPreview: '讨论：如何在Flutter中实现复杂的动画效果...',
    ),
  ];

  final List<Message> _favorites = [
    Message(
      type: 'favorite',
      title: '收藏通知',
      content: '用户"移动开发者"收藏了您的帖子"Flutter状态管理比较"。',
      time: DateTime.now().subtract(const Duration(days: 2)),
      read: true,
      avatar: 'assets/images/user3.jpg',
      postPreview: '比较Provider、Bloc、Riverpod等状态管理方案...',
    ),
  ];

  final List<Message> _comments = [
    Message(
      type: 'comment',
      title: '评论通知',
      content: '用户"校园助手"回复了您的帖子"求推荐笔记本电脑"。',
      time: DateTime.now().subtract(const Duration(minutes: 45)),
      read: false,
      avatar: 'assets/images/user4.jpg',
      postPreview: '预算5000左右，主要用于编程学习...',
      commentContent: '推荐联想小新Pro16，性价比高，适合学生使用。',
    ),
    Message(
      type: 'reply',
      title: '回复通知',
      content: '用户"技术达人"回复了您的评论"这个配置足够用了"。',
      time: DateTime.now().subtract(const Duration(days: 3)),
      read: true,
      avatar: 'assets/images/user5.jpg',
      postPreview: '讨论：学生应该选择Mac还是Windows...',
      commentContent: '确实够用，但如果有预算可以考虑更好的显卡。',
    ),
  ];

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: _tabs.length, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Scaffold(
      appBar: AppBar(
        title: const Text('消息中心(功能尚未完善，敬请期待)'),
        centerTitle: true,
        elevation: 0,
        bottom: TabBar(
          controller: _tabController,
          tabs: _tabs.map((tab) => Tab(text: tab)).toList(),
          labelColor: colorScheme.primary,
          unselectedLabelColor: colorScheme.onSurface.withOpacity(0.6),
          indicatorColor: colorScheme.primary,
          indicatorSize: TabBarIndicatorSize.label,
          labelStyle: const TextStyle(fontWeight: FontWeight.w600),
          isScrollable: true,
        ),
      ),
      body: TabBarView(
        controller: _tabController,
        children: [
          _buildNotificationList(_notifications),
          _buildMessageList(_likes),
          _buildMessageList(_favorites),
          _buildCommentList(_comments),
        ],
      ),
    );
  }

  Widget _buildNotificationList(List<Message> messages) {
    return ListView.separated(
      padding: const EdgeInsets.symmetric(vertical: 8),
      itemCount: messages.length,
      separatorBuilder: (context, index) =>
          const Divider(height: 1, indent: 72),
      itemBuilder: (context, index) => _buildNotificationItem(messages[index]),
    );
  }

  Widget _buildMessageList(List<Message> messages) {
    return ListView.separated(
      padding: const EdgeInsets.symmetric(vertical: 8),
      itemCount: messages.length,
      separatorBuilder: (context, index) =>
          const Divider(height: 1, indent: 72),
      itemBuilder: (context, index) => _buildMessageItem(messages[index]),
    );
  }

  Widget _buildCommentList(List<Message> messages) {
    return ListView.separated(
      padding: const EdgeInsets.symmetric(vertical: 8),
      itemCount: messages.length,
      separatorBuilder: (context, index) =>
          const Divider(height: 1, indent: 72),
      itemBuilder: (context, index) => _buildCommentItem(messages[index]),
    );
  }

  Widget _buildNotificationItem(Message message) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final timeFormat = DateFormat('MM/dd HH:mm');

    return ListTile(
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      leading: Container(
        width: 48,
        height: 48,
        decoration: BoxDecoration(
          color: colorScheme.surfaceVariant,
          borderRadius: BorderRadius.circular(12),
        ),
        child: Center(
          child: SvgPicture.asset(
            message.avatar,
            width: 24,
            height: 24,
            color: colorScheme.primary,
          ),
        ),
      ),
      title: Text(
        message.title,
        style: TextStyle(
          fontWeight: message.read ? FontWeight.normal : FontWeight.bold,
          color: message.read ? colorScheme.onSurface : colorScheme.primary,
        ),
      ),
      subtitle: Text(
        message.content,
        maxLines: 2,
        overflow: TextOverflow.ellipsis,
        style: TextStyle(
          color: message.read
              ? colorScheme.onSurface.withOpacity(0.7)
              : colorScheme.onSurface,
        ),
      ),
      trailing: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        crossAxisAlignment: CrossAxisAlignment.end,
        children: [
          Text(
            timeFormat.format(message.time),
            style: TextStyle(
              fontSize: 12,
              color: colorScheme.onSurface.withOpacity(0.5),
            ),
          ),
          if (!message.read) ...[
            const SizedBox(height: 4),
            Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(
                color: colorScheme.primary,
                shape: BoxShape.circle,
              ),
            ),
          ],
        ],
      ),
      onTap: () {
        setState(() {
          message.read = true;
        });
        // 处理点击事件
      },
    );
  }

  Widget _buildMessageItem(Message message) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final timeFormat = DateFormat('MM/dd HH:mm');

    return ListTile(
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      leading: CircleAvatar(
        radius: 24,
        backgroundImage: AssetImage(message.avatar),
      ),
      title: Text(
        message.title,
        style: TextStyle(
          fontWeight: message.read ? FontWeight.normal : FontWeight.bold,
          color: message.read ? colorScheme.onSurface : colorScheme.primary,
        ),
      ),
      subtitle: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            message.content,
            style: TextStyle(
              color: message.read
                  ? colorScheme.onSurface.withOpacity(0.7)
                  : colorScheme.onSurface,
            ),
          ),
          const SizedBox(height: 4),
          Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
              color: colorScheme.surfaceVariant.withOpacity(0.3),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Text(
              message.postPreview!,
              maxLines: 2,
              overflow: TextOverflow.ellipsis,
              style: TextStyle(
                fontSize: 12,
                color: colorScheme.onSurface.withOpacity(0.7),
              ),
            ),
          ),
        ],
      ),
      trailing: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        crossAxisAlignment: CrossAxisAlignment.end,
        children: [
          Text(
            timeFormat.format(message.time),
            style: TextStyle(
              fontSize: 12,
              color: colorScheme.onSurface.withOpacity(0.5),
            ),
          ),
          if (!message.read) ...[
            const SizedBox(height: 4),
            Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(
                color: colorScheme.primary,
                shape: BoxShape.circle,
              ),
            ),
          ],
        ],
      ),
      onTap: () {
        setState(() {
          message.read = true;
        });
        // 处理点击事件
      },
    );
  }

  Widget _buildCommentItem(Message message) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final timeFormat = DateFormat('MM/dd HH:mm');

    return ListTile(
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      leading: CircleAvatar(
        radius: 24,
        backgroundImage: AssetImage(message.avatar),
      ),
      title: Text(
        message.title,
        style: TextStyle(
          fontWeight: message.read ? FontWeight.normal : FontWeight.bold,
          color: message.read ? colorScheme.onSurface : colorScheme.primary,
        ),
      ),
      subtitle: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            message.content,
            style: TextStyle(
              color: message.read
                  ? colorScheme.onSurface.withOpacity(0.7)
                  : colorScheme.onSurface,
            ),
          ),
          const SizedBox(height: 4),
          Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
              color: colorScheme.surfaceVariant.withOpacity(0.3),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  message.postPreview!,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: TextStyle(
                    fontSize: 12,
                    color: colorScheme.onSurface.withOpacity(0.7),
                  ),
                ),
                const SizedBox(height: 4),
                Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
                  decoration: BoxDecoration(
                    color: colorScheme.surfaceVariant,
                    borderRadius: BorderRadius.circular(6),
                  ),
                  child: Text(
                    message.commentContent!,
                    maxLines: 2,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      fontSize: 12,
                      color: colorScheme.onSurface,
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
      trailing: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        crossAxisAlignment: CrossAxisAlignment.end,
        children: [
          Text(
            timeFormat.format(message.time),
            style: TextStyle(
              fontSize: 12,
              color: colorScheme.onSurface.withOpacity(0.5),
            ),
          ),
          if (!message.read) ...[
            const SizedBox(height: 4),
            Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(
                color: colorScheme.primary,
                shape: BoxShape.circle,
              ),
            ),
          ],
        ],
      ),
      onTap: () {
        setState(() {
          message.read = true;
        });
        // 处理点击事件
      },
    );
  }
}

class Message {
  final String type;
  final String title;
  final String content;
  final DateTime time;
  bool read;
  final String avatar;
  String? postPreview;
  String? commentContent;

  Message({
    required this.type,
    required this.title,
    required this.content,
    required this.time,
    required this.read,
    required this.avatar,
    this.postPreview,
    this.commentContent,
  });
}
