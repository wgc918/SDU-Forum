import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/profile/about_page.dart';
import 'package:sduforum/pages/profile/editProfile_page.dart';
import 'package:sduforum/pages/profile/helpFeedback_page.dart';
import 'package:sduforum/pages/profile/profile_page.dart';
import 'package:sduforum/pages/profile/securitySettings_page.dart';
import 'package:sduforum/utils/tokenManager.dart';
import 'package:sduforum/pages/home_page.dart';

class SettingsPage extends StatefulWidget {
  final String user_id;
  final UserProfile profile;
  SettingsPage({super.key, required this.user_id, required this.profile});
  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  Dio dio = Dio();
  bool _darkMode = false;
  bool _notificationsEnabled = true;
  bool _biometricLogin = false;
  String _language = '简体中文';

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final textTheme = theme.textTheme;

    return Scaffold(
      appBar: AppBar(
        title: Text('设置', style: textTheme.titleLarge),
        centerTitle: true,
        elevation: 0,
      ),
      body: ListView(
        padding: const EdgeInsets.symmetric(vertical: 16),
        children: [
          // 账号设置
          _buildSectionHeader('账号设置', context),
          _buildListTile(
            context,
            leading: Icon(Icons.account_circle, color: colorScheme.primary),
            title: '个人资料',
            trailing: Icon(Icons.chevron_right,
                color: colorScheme.onSurface.withOpacity(0.5)),
            onTap: () => _navigateToProfile(),
          ),
          _buildListTile(
            context,
            leading: Icon(Icons.security, color: Colors.blue),
            title: '账号安全',
            trailing: Icon(Icons.chevron_right,
                color: colorScheme.onSurface.withOpacity(0.5)),
            onTap: () => _navigateToSecurity(),
          ),
          _buildSwitchListTile(
            context,
            leading: Icon(Icons.fingerprint, color: Colors.purple),
            title: '生物识别登录',
            value: _biometricLogin,
            onChanged: (value) => setState(() => _biometricLogin = value),
          ),

          // 应用设置
          _buildSectionHeader('应用设置', context),
          _buildListTile(
            context,
            leading: Icon(Icons.language, color: Colors.green),
            title: '语言',
            trailing: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(_language,
                    style: textTheme.bodyMedium?.copyWith(
                        color: colorScheme.onSurface.withOpacity(0.6))),
                Icon(Icons.chevron_right,
                    color: colorScheme.onSurface.withOpacity(0.5)),
              ],
            ),
            onTap: () => _showLanguageDialog(),
          ),
          _buildSwitchListTile(
            context,
            leading: Icon(Icons.dark_mode, color: Colors.indigo),
            title: '深色模式',
            value: _darkMode,
            onChanged: (value) => setState(() => _darkMode = value),
          ),
          _buildSwitchListTile(
            context,
            leading: Icon(Icons.notifications, color: Colors.orange),
            title: '消息通知',
            value: _notificationsEnabled,
            onChanged: (value) => setState(() => _notificationsEnabled = value),
          ),

          // 其他
          _buildSectionHeader('其他', context),
          _buildListTile(
            context,
            leading: Icon(Icons.help_outline, color: Colors.blueGrey),
            title: '帮助与反馈',
            trailing: Icon(Icons.chevron_right,
                color: colorScheme.onSurface.withOpacity(0.5)),
            onTap: () => _navigateToHelp(),
          ),
          _buildListTile(
            context,
            leading: Icon(Icons.info_outline, color: Colors.blueGrey),
            title: '关于我们',
            trailing: Icon(Icons.chevron_right,
                color: colorScheme.onSurface.withOpacity(0.5)),
            onTap: () => _navigateToAbout(),
          ),

          // 退出登录按钮
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 32),
            child: ElevatedButton(
              onPressed: //() => _showLogoutDialog(),
                  () {
                _push_token_to_blackList();
                TokenManager.clearTokens();
                Navigator.pushAndRemoveUntil(
                    context,
                    MaterialPageRoute(builder: (context) => HomePage(dio: dio)),
                    (route) => false);
              },
              style: ElevatedButton.styleFrom(
                backgroundColor: colorScheme.errorContainer,
                foregroundColor: colorScheme.onErrorContainer,
                padding: const EdgeInsets.symmetric(vertical: 16),
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
              child: const Text('退出登录'),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSectionHeader(String title, BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(24, 24, 24, 8),
      child: Text(
        title,
        style: Theme.of(context).textTheme.titleSmall?.copyWith(
              color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
            ),
      ),
    );
  }

  Widget _buildListTile(
    BuildContext context, {
    required Widget leading,
    required String title,
    required Widget trailing,
    required VoidCallback onTap,
  }) {
    return Card(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      elevation: 0,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(
          color: Theme.of(context).colorScheme.outline.withOpacity(0.1),
          width: 1,
        ),
      ),
      child: ListTile(
        leading: leading,
        title: Text(title, style: Theme.of(context).textTheme.bodyLarge),
        trailing: trailing,
        onTap: onTap,
        contentPadding: const EdgeInsets.symmetric(horizontal: 16),
        minLeadingWidth: 24,
      ),
    );
  }

  Widget _buildSwitchListTile(
    BuildContext context, {
    required Widget leading,
    required String title,
    required bool value,
    required ValueChanged<bool> onChanged,
  }) {
    return Card(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      elevation: 0,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(
          color: Theme.of(context).colorScheme.outline.withOpacity(0.1),
          width: 1,
        ),
      ),
      child: SwitchListTile(
        secondary: leading,
        title: Text(title, style: Theme.of(context).textTheme.bodyLarge),
        value: value,
        onChanged: onChanged,
        contentPadding: const EdgeInsets.symmetric(horizontal: 16),
      ),
    );
  }

  // 以下是模拟的导航和对话框方法
  Future<void> _navigateToProfile() async {
    // 导航到个人资料页面
    final shouldRefresh = await Navigator.push<bool>(
      context,
      MaterialPageRoute(
        builder: (context) => EditProfilePage(
          userId: widget.user_id,
          initialProfile: widget.profile,
        ),
      ),
    );
  }

  void _navigateToSecurity() {
    // 导航到账号安全页面
     Navigator.push(
      context,
      MaterialPageRoute(
        builder: (context) => SecuritySettingsPage(userId: widget.user_id),
      ),
    );
  }

  void _navigateToHelp() {
    // 导航到帮助与反馈页面
     Navigator.push(
      context,
      MaterialPageRoute(builder: (context) =>  HelpFeedbackPage(user_id: widget.user_id)),
    );
  }

  void _navigateToAbout() {
    // 导航到关于我们页面
     Navigator.push(
      context,
      MaterialPageRoute(builder: (context) => const AboutPage()),
    );
  }

  void _showLanguageDialog() {
    showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('选择语言'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              RadioListTile(
                title: const Text('简体中文'),
                value: '简体中文',
                groupValue: _language,
                onChanged: (value) {
                  setState(() => _language = value.toString());
                  Navigator.pop(context);
                },
              ),
              RadioListTile(
                title: const Text('English'),
                value: 'English',
                groupValue: _language,
                onChanged: (value) {
                  setState(() => _language = value.toString());
                  Navigator.pop(context);
                },
              ),
            ],
          ),
        );
      },
    );
  }

  void _showLogoutDialog() {
    showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('确认退出登录？'),
          content: const Text('退出后需要重新登录才能使用完整功能'),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('取消'),
            ),
            TextButton(
              onPressed: () {
                // 执行退出登录逻辑
                Navigator.pop(context);
              },
              child: const Text('确认退出'),
            ),
          ],
        );
      },
    );
  }

  void _push_token_to_blackList() async {
    final tokens = await TokenManager.getTokens();
    String? refreshToken = tokens?['refreshToken'];

    if (refreshToken != null) {
      try {
        await dio.post(
            'http://101.76.244.44:8080/sdu_forum/api/user/push_token_to_blackList',
            data: {'user_id': widget.user_id, 'token': refreshToken});
      } catch (e) {
        //print('Error pushing token to blacklist: $e');
      }
    }
  }
}
