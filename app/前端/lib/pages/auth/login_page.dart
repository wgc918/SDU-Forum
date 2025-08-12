import 'package:flutter/material.dart';
import 'package:sduforum/core/main_frame.dart';
import 'package:sduforum/pages/auth/register_page.dart';
import 'package:dio/dio.dart';
import 'package:sduforum/utils/tokenManager.dart';

class LoginPage extends StatefulWidget {
  const LoginPage({super.key});

  @override
  State<LoginPage> createState() => _LoginPageState();
}

class _LoginPageState extends State<LoginPage> {
  // 控制器用于获取输入框的值
  final TextEditingController _userIdController = TextEditingController();
  final TextEditingController _passwordController = TextEditingController();

  // 用于表单验证
  final _formKey = GlobalKey<FormState>();

  // 用于密码输入框的焦点控制
  final FocusNode _passwordFocusNode = FocusNode();

  // 用于控制加载状态和密码可见性
  bool _isLoading = false;
  bool _obscurePassword = true;

  @override
  void dispose() {
    _userIdController.dispose();
    _passwordController.dispose();
    _passwordFocusNode.dispose();
    super.dispose();
  }

  Future<void> _submitForm() async {
    if (!_formKey.currentState!.validate()) return;

    // 取消键盘
    FocusScope.of(context).unfocus();

    setState(() => _isLoading = true);

    Dio dio = Dio();
    try {
      var response = await dio
          .post('http://120.26.127.37:8080/sdu_forum/api/user/login', data: {
        'user_id': _userIdController.text.trim(),
        'password': _passwordController.text.trim(),
      });
       // 登录成功
      if (response.statusCode == 200 && response.data['login_state'] == true&&response.data['is_blacklist']==false) {
        final accessToken = response.data['access_token'];
        final refreshToken = response.data['refresh_token'];

        // 保存token到本地
        await TokenManager.saveTokens(
          accessToken: accessToken,
          accessTokenExp: response.data['access_token_exp'].toString(),
          refreshToken: refreshToken,
          refreshTokenExp: response.data['refresh_token_exp'].toString(),
          userId: _userIdController.text.trim(),
        );
        // 打印token信息
        var tokens = await TokenManager.getTokens();
        print('Access Token: ${tokens?['accessToken']}');
        print('Refresh Token: ${tokens?['refreshToken']}');

        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('登录成功！'),
            backgroundColor: Colors.green,
            duration: Duration(seconds: 1),
          ),
        );

        // 导航到主页
        Navigator.pushAndRemoveUntil(
            context,
            MaterialPageRoute(
                builder: (context) => MainFramePage(
                    account: _userIdController.text.trim(), dio: dio)),
            (route) => false);
      } else if(response.statusCode == 200&&response.data['is_blacklist']==true){
        setState(() => _isLoading = false);
        final reason = response.data['reason']?.toString() ?? '未知原因';
        final endTime = response.data['end_time']?.toString() ?? '未知时间';

        ScaffoldMessenger.of(context)
          ..hideCurrentSnackBar()
          ..showSnackBar(
            SnackBar(
              content: Text('登录失败，您的账号已被封禁\n原因: $reason\n解封时间: $endTime'),
              backgroundColor: Colors.redAccent[700],
              duration: const Duration(seconds: 5),
              behavior: SnackBarBehavior.floating,
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(10),
              ),
              action: SnackBarAction(
                label: '知道了',
                textColor: Colors.white,
                onPressed: () {
                  ScaffoldMessenger.of(context).hideCurrentSnackBar();
                },
              ),
            ),
          );
      }
      else{
        // 登录失败
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('登录失败，请检查用户名和密码'),
            backgroundColor: Colors.red,
            duration: Duration(seconds: 2),
          ),
        );
        return;
      }
    } catch (e) {
      print('登录请求失败: $e');
      setState(() => _isLoading = false);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('登录失败2:'),
          backgroundColor: Colors.red,
          duration: Duration(seconds: 2),
        ),
      );
      return;
    }
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => FocusScope.of(context).unfocus(),
      behavior: HitTestBehavior.translucent,
      child: Scaffold(
        appBar: AppBar(
          title: const Text(
            '欢迎登录',
            style: TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 22,
            ),
          ),
          centerTitle: true,
          elevation: 0,
          backgroundColor: Colors.transparent,
          foregroundColor: Colors.blue.shade800,
          leading: IconButton(
            icon: const Icon(Icons.arrow_back),
            onPressed: () {
              FocusScope.of(context).unfocus();
              Navigator.pop(context);
            },
          ),
        ),
        body: Container(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [
                Colors.blue.shade50,
                Colors.white,
              ],
            ),
          ),
          child: SafeArea(
            child: SingleChildScrollView(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: Form(
                key: _formKey,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    const SizedBox(height: 40),
                    Center(
                      child: Image.asset(
                        "asset/images/logo.png",
                        height: 100,
                        width: 100,
                      ),
                    ),
                    const SizedBox(height: 30),
                    Text(
                      'SDU校园论坛',
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                        color: Colors.blue.shade900,
                      ),
                      textAlign: TextAlign.center,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      '请使用您的账号登录',
                      style: TextStyle(
                        fontSize: 16,
                        color: Colors.grey.shade600,
                      ),
                      textAlign: TextAlign.center,
                    ),
                    const SizedBox(height: 40),
                    TextFormField(
                      controller: _userIdController,
                      decoration: InputDecoration(
                        labelText: '学号/用户名',
                        prefixIcon: const Icon(Icons.person_outline),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                        filled: true,
                        fillColor: Colors.white,
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return '请输入您的学号或用户名';
                        }
                         // 检查是否为纯数字且长度为12
                        if (!RegExp(r'^\d{12}$').hasMatch(value)) {
                          return '学号格式出错';
                        }
                        return null;
                      },
                      keyboardType: TextInputType.number,
                      textInputAction: TextInputAction.next,
                      onFieldSubmitted: (_) {
                        FocusScope.of(context).requestFocus(_passwordFocusNode);
                      },
                    ),
                    const SizedBox(height: 20),
                    TextFormField(
                      controller: _passwordController,
                      focusNode: _passwordFocusNode,
                      obscureText: _obscurePassword,
                      decoration: InputDecoration(
                        labelText: '密码',
                        prefixIcon: const Icon(Icons.lock_outline),
                        suffixIcon: IconButton(
                          icon: Icon(
                            _obscurePassword
                                ? Icons.visibility_off
                                : Icons.visibility,
                            color: Colors.grey,
                          ),
                          onPressed: () {
                            setState(
                                () => _obscurePassword = !_obscurePassword);
                          },
                        ),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                        filled: true,
                        fillColor: Colors.white,
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return '请输入密码';
                        }
                        if (value.length != 6) {
                          return '密码长度为6位';
                        }
                        return null;
                      },
                    ),
                    const SizedBox(height: 16),
                    Align(
                      alignment: Alignment.centerRight,
                      child: TextButton(
                        onPressed: () {
                          // 跳转到找回密码页面
                          // Navigator.push(context, MaterialPageRoute(builder: (_) => ForgotPasswordPage()));
                        },
                        child: Text(
                          '忘记密码？',
                          style: TextStyle(
                            color: Colors.blue.shade700,
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(height: 30),
                    SizedBox(
                      height: 50,
                      child: ElevatedButton(
                        onPressed: _isLoading ? null : _submitForm,
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.blue.shade700,
                          foregroundColor: Colors.white,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12),
                          ),
                          elevation: 2,
                        ),
                        child: _isLoading
                            ? const SizedBox(
                                width: 20,
                                height: 20,
                                child: CircularProgressIndicator(
                                  strokeWidth: 2,
                                  color: Colors.white,
                                ),
                              )
                            : const Text(
                                '登 录',
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                      ),
                    ),
                    const SizedBox(height: 30),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Text('还没有账号？'),
                        TextButton(
                          onPressed: () {
                            FocusScope.of(context).unfocus();
                            // 跳转到注册页面
                            Navigator.push(
                                context,
                                MaterialPageRoute(
                                    builder: (_) => const RegisterPage()));
                          },
                          child: Text(
                            '立即注册',
                            style: TextStyle(
                              color: Colors.blue.shade700,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
