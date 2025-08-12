import 'package:flutter/material.dart';

class GuestProfilePage extends StatelessWidget {
  const GuestProfilePage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final textTheme = theme.textTheme;

    return Scaffold(
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
            expandedHeight: 200,
            pinned: true,
            flexibleSpace: FlexibleSpaceBar(
              background: Container(
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    begin: Alignment.topCenter,
                    end: Alignment.bottomCenter,
                    colors: [
                      colorScheme.primary.withOpacity(0.8),
                      colorScheme.primary.withOpacity(0.4),
                    ],
                  ),
                ),
                child: Center(
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const SizedBox(height: 60),
                      Container(
                        width: 100,
                        height: 100,
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: colorScheme.surface,
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.1),
                              blurRadius: 8,
                              offset: const Offset(0, 4),
                            ),
                          ],
                        ),
                        child: Icon(
                          Icons.account_circle,
                          size: 60,
                          color: colorScheme.onSurface.withOpacity(0.5),
                        ),
                      ),
                      const SizedBox(height: 16),
                      Text(
                        '游客',
                        style: textTheme.headlineSmall?.copyWith(
                          fontWeight: FontWeight.bold,
                          color: colorScheme.onSurface,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
              collapseMode: CollapseMode.pin,
            ),
          ),
          SliverPadding(
            padding: const EdgeInsets.symmetric(horizontal: 20),
            sliver: SliverList(
              delegate: SliverChildListDelegate([
                Column(
                  children: [
                    const SizedBox(height: 24),
                    Text(
                      '登录后解锁完整功能',
                      style: textTheme.titleMedium?.copyWith(
                        color: colorScheme.onSurface.withOpacity(0.8),
                      ),
                    ),
                    const SizedBox(height: 24),
                    SizedBox(
                      width: double.infinity,
                      child: ElevatedButton(
                        onPressed: () {
                          // 跳转到登录页面
                        },
                        style: ElevatedButton.styleFrom(
                          padding: const EdgeInsets.symmetric(vertical: 16),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12),
                          ),
                        ),
                        child: Text(
                          '立即登录/注册',
                          style: textTheme.titleMedium?.copyWith(
                            color: colorScheme.onPrimary,
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(height: 32),
                  ],
                ),
              ]),
            ),
          ),
          SliverPadding(
            padding: const EdgeInsets.symmetric(horizontal: 20),
            sliver: _buildFeatureGrid(context),
          ),
          const SliverPadding(
            padding: EdgeInsets.only(bottom: 20), // 添加底部安全间距
          ),
        ],
      ),
    );
  }

  Widget _buildFeatureGrid(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    final features = [
      _FeatureItem(icon: Icons.edit_note, label: '发布帖子', color: Colors.blue),
      _FeatureItem(icon: Icons.favorite, label: '点赞收藏', color: Colors.red),
      _FeatureItem(icon: Icons.comment, label: '发表评论', color: Colors.green),
      _FeatureItem(icon: Icons.group, label: '加入社团', color: Colors.orange),
      _FeatureItem(
          icon: Icons.notifications, label: '消息通知', color: Colors.purple),
      _FeatureItem(icon: Icons.verified, label: '学生认证', color: Colors.teal),
    ];

    return SliverGrid(
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 3,
        mainAxisSpacing: 16,
        crossAxisSpacing: 16,
        childAspectRatio: 1.0, // 调整为1.0确保有足够空间
      ),
      delegate: SliverChildBuilderDelegate(
        (context, index) {
          final feature = features[index];
          return Container(
            margin: const EdgeInsets.only(bottom: 8), // 添加底部边距
            decoration: BoxDecoration(
              color: colorScheme.surfaceVariant.withOpacity(0.3),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  width: 48,
                  height: 48,
                  decoration: BoxDecoration(
                    color: feature.color.withOpacity(0.1),
                    shape: BoxShape.circle,
                  ),
                  child: Icon(feature.icon, color: feature.color),
                ),
                const SizedBox(height: 8),
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 4), // 添加水平内边距
                  child: Text(
                    feature.label,
                    style: theme.textTheme.bodyMedium?.copyWith(
                      color: colorScheme.onSurface.withOpacity(0.8),
                    ),
                    textAlign: TextAlign.center,
                    maxLines: 2, // 允许文本换行
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ],
            ),
          );
        },
        childCount: features.length,
      ),
    );
  }
}

class _FeatureItem {
  final IconData icon;
  final String label;
  final Color color;

  _FeatureItem({
    required this.icon,
    required this.label,
    required this.color,
  });
}
