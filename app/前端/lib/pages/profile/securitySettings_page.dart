import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/home_page.dart';

class SecuritySettingsPage extends StatelessWidget {
  final String userId;
  const SecuritySettingsPage({super.key, required this.userId});

  Future<bool> modifyPassword(
      String currentPassword, String newPassword) async {
    if (newPassword.length != 6) {
      return false;
    }

    Dio dio = Dio();
    try {
      var response = await dio.post(
        'http://120.26.127.37:8080/sdu_forum/api/user/modify_password',
        data: {
          'user_id': userId,
          'oldPassword': currentPassword,
          'newPassword': newPassword,
        },
      );
      return response.statusCode == 200 && response.data['state'];
    } catch (e) {
      print('修改密码失败: $e');
      return false;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('账号安全')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _buildSecurityOption(
            context,
            icon: Icons.lock,
            title: '修改密码',
            onTap: () => _showChangePasswordDialog(context),
          ),
          _buildSecurityOption(
            context,
            icon: Icons.email,
            title: '绑定邮箱',
            onTap: () => _showBindEmailDialog(context),
          ),
          _buildSecurityOption(
            context,
            icon: Icons.phone,
            title: '绑定手机',
            onTap: () => _showBindPhoneDialog(context),
          ),
        ],
      ),
    );
  }

  Widget _buildSecurityOption(
    BuildContext context, {
    required IconData icon,
    required String title,
    required VoidCallback onTap,
  }) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: ListTile(
        leading: Icon(icon, color: Theme.of(context).colorScheme.primary),
        title: Text(title),
        trailing: const Icon(Icons.chevron_right),
        onTap: onTap,
      ),
    );
  }

  void _showChangePasswordDialog(BuildContext context) {
    final currentPasswordController = TextEditingController();
    final newPasswordController = TextEditingController();
    final confirmPasswordController = TextEditingController();

    showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('修改密码'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: currentPasswordController,
                obscureText: true,
                decoration: const InputDecoration(labelText: '当前密码'),
              ),
              TextField(
                controller: newPasswordController,
                obscureText: true,
                decoration: const InputDecoration(
                  labelText: '新密码',
                  helperText: '密码长度6位',
                ),
              ),
              TextField(
                controller: confirmPasswordController,
                obscureText: true,
                decoration: const InputDecoration(labelText: '确认新密码'),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('取消'),
            ),
            TextButton(
              onPressed: () async {
                final currentPassword = currentPasswordController.text;
                final newPassword = newPasswordController.text;
                final confirmPassword = confirmPasswordController.text;

                if (newPassword.length != 6) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('密码长度需要6位')),
                  );
                  return;
                }

                if (newPassword != confirmPassword) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('两次输入的密码不一致')),
                  );
                  return;
                }

                final success =
                    await modifyPassword(currentPassword, newPassword);
                if (success) {
                  Navigator.pop(context);
                  Navigator.pushAndRemoveUntil(
                      context,
                      MaterialPageRoute(
                          builder: (context) => HomePage(dio: Dio())),
                      (route) => false);
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('密码修改成功')),
                  );
                } else {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('密码修改失败，请检查当前密码是否正确')),
                  );
                }
              },
              child: const Text('确认'),
            ),
          ],
        );
      },
    );
  }

  void _showBindEmailDialog(BuildContext context) {
    // 类似实现邮箱绑定对话框
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('绑定邮箱'),
        content: const TextField(
          decoration: InputDecoration(labelText: '请输入邮箱地址'),
          keyboardType: TextInputType.emailAddress,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () {
              // 实现邮箱绑定逻辑
              Navigator.pop(context);
            },
            child: const Text('确认'),
          ),
        ],
      ),
    );
  }

  void _showBindPhoneDialog(BuildContext context) {
    // 类似实现手机绑定对话框
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('绑定手机'),
        content: const TextField(
          decoration: InputDecoration(labelText: '请输入手机号码'),
          keyboardType: TextInputType.phone,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () {
              // 实现手机绑定逻辑
              Navigator.pop(context);
            },
            child: const Text('确认'),
          ),
        ],
      ),
    );
  }
}
