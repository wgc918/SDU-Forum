import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/pages/auth/login_page.dart';

class RegisterPage extends StatefulWidget {
  const RegisterPage({super.key});

  @override
  State<RegisterPage> createState() => _RegisterPageState();
}

class _RegisterPageState extends State<RegisterPage> {
  // 控制器用于获取输入框的值
  final TextEditingController _userIdController = TextEditingController();
  final TextEditingController _passwordController = TextEditingController();
  final TextEditingController _confirmPasswordController =
      TextEditingController();
  final TextEditingController _verificationCodeController =
      TextEditingController();

  // 使用 GlobalKey 来管理表单状态
  final _formKey = GlobalKey<FormState>();

  // 用于管理焦点
  final FocusNode _focusNode = FocusNode();

  // 用于管理加载状态和用户协议同意状态
  bool _isLoading = false;
  bool _agreedToTerms = false;

  @override
  void dispose() {
    _userIdController.dispose();
    _passwordController.dispose();
    _confirmPasswordController.dispose();
    _verificationCodeController.dispose();
    _focusNode.dispose();
    super.dispose();
  }

  Future<void> _submitForm() async {
    // 先取消键盘
    FocusScope.of(context).unfocus();

    if (!_formKey.currentState!.validate()) return;
    if (!_agreedToTerms) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('请阅读并同意用户协议和隐私政策'),
          backgroundColor: Colors.red,
        ),
      );
      return;
    }

    setState(() => _isLoading = true);

    try {
      Dio dio = Dio();

      Response response = await dio
          .post('http://120.26.127.37:8080/sdu_forum/api/user/register', data: {
        'user_id': _userIdController.text.trim(),
        'password': _passwordController.text.trim(),
        'verificationCode': _verificationCodeController.text.trim(),
      });
      //print('注册请求: ${response.data}');
      if (response.statusCode == 200) {
        setState(() => _isLoading = false);
        if (response.data['Register_state'] == 0) {
          print('注册成功0');
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('注册成功'),
              backgroundColor: Colors.green,
            ),
          );
          // 注册成功后跳转到登录页面
          Navigator.pushAndRemoveUntil(
              context,
              MaterialPageRoute(builder: (context) => const LoginPage()),
              (route) => false);
        } else if (response.data['Register_state'] == 1) {
          //print('注册失败1');
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('认证码错误'),
              backgroundColor: Colors.red,
            ),
          );
        } else if (response.data['Register_state'] == 2) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('学号已被注册'),
              backgroundColor: Colors.red,
            ),
          );
        }
      }
      else {
        setState(() => _isLoading = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('注册失败，请稍后再试'),
            backgroundColor: Colors.red,
          ),
        );
      }
    } catch (e) {
      setState(() => _isLoading = false);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('注册失败'),
          backgroundColor: Colors.red,
        ),
      );
      //print('注册失败: $e');
    }
  }

  void _showTermsDialog() {
    // 显示对话框前取消键盘
    FocusScope.of(context).unfocus();

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('用户协议和隐私政策'),
        content: SingleChildScrollView(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                '用户协议',
                style: TextStyle(fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 8),
              const Text(
                '1. 您必须遵守所有适用的法律法规\n'
                '2. 不得发布违法、侵权或不当内容\n'
                '3. 您对账号下的所有行为负责\n'
                '4. 我们保留终止违规账号的权利',
              ),
              const SizedBox(height: 16),
              const Text(
                '隐私政策',
                style: TextStyle(fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 8),
              const Text(
                '1. 我们仅收集必要的个人信息\n'
                '2. 您的数据将受到严格保护\n'
                '3. 我们不会未经许可共享您的信息\n'
                '4. 您有权随时查看和删除您的数据',
              ),
              const SizedBox(height: 16),
              Text(
                '请仔细阅读并确认您已理解上述条款',
                style: TextStyle(color: Colors.grey.shade600),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () {
              setState(() => _agreedToTerms = true);
              Navigator.pop(context);
            },
            child: const Text('同意'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () {
        // 点击空白处取消键盘和输入框焦点
        FocusScope.of(context).unfocus();
      },
      behavior: HitTestBehavior.translucent,
      child: Scaffold(
        appBar: AppBar(
          title: const Text(
            '注册账号',
            style: TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 20,
            ),
          ),
          centerTitle: true,
          elevation: 0,
          backgroundColor: Colors.transparent,
          foregroundColor: Colors.blue.shade800,
          leading: IconButton(
            icon: const Icon(Icons.arrow_back),
            onPressed: () {
              FocusScope.of(context).unfocus(); // 返回前取消键盘
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
                      '创建您的账号',
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                        color: Colors.blue.shade900,
                      ),
                      textAlign: TextAlign.center,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      '加入SDU校园社区',
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
                        labelText: '学号',
                        prefixIcon: const Icon(Icons.person_outline),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                        filled: true,
                        fillColor: Colors.white,
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return '请输入您的学号';
                        }
                        // 检查是否为纯数字且长度为12
                        if (!RegExp(r'^\d{12}$').hasMatch(value)) {
                          return '学号格式出错';
                        }
                        return null;
                      },
                      keyboardType: TextInputType.number,
                       
                    ),
                    const SizedBox(height: 20),
                    TextFormField(
                      controller: _passwordController,
                      obscureText: true,
                      decoration: InputDecoration(
                        labelText: '密码',
                        prefixIcon: const Icon(Icons.lock_outline),
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
                          return '密码长度为6位字符';
                        }
                        return null;
                      },
                    ),
                    const SizedBox(height: 20),
                    TextFormField(
                      controller: _confirmPasswordController,
                      obscureText: true,
                      decoration: InputDecoration(
                        labelText: '确认密码',
                        prefixIcon: const Icon(Icons.lock_outline),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                        filled: true,
                        fillColor: Colors.white,
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return '请再次输入密码';
                        }
                        if (value != _passwordController.text) {
                          return '两次输入的密码不一致';
                        }
                        return null;
                      },
                    ),
                    const SizedBox(height: 20),
                    TextFormField(
                      controller: _verificationCodeController,
                      decoration: InputDecoration(
                        labelText: '认证码',
                        prefixIcon: const Icon(Icons.verified_user_outlined),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                        filled: true,
                        fillColor: Colors.white,
                      ),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return '请输入认证码';
                        }
                        return null;
                      },
                      keyboardType: TextInputType.number,
                    ),
                    const SizedBox(height: 20),
                    Row(
                      crossAxisAlignment: CrossAxisAlignment.center,
                      children: [
                        Checkbox(
                          value: _agreedToTerms,
                          onChanged: (value) {
                            FocusScope.of(context).unfocus();
                            setState(() {
                              _agreedToTerms = value ?? false;
                            });
                          },
                          activeColor: Colors.blue.shade700,
                        ),
                        Expanded(
                          child: Wrap(
                            crossAxisAlignment:
                                WrapCrossAlignment.center, 
                            spacing: 4, 
                            children: [
                              const Text('我已阅读并同意'),
                              GestureDetector(
                                onTap: () {
                                  FocusScope.of(context).unfocus();
                                  _showTermsDialog();
                                },
                                child: Text(
                                  '《用户协议》',
                                  style: TextStyle(
                                    color: Colors.blue.shade700,
                                    decoration: TextDecoration.underline,
                                  ),
                                ),
                              ),
                              GestureDetector(
                                onTap: () {
                                  FocusScope.of(context).unfocus();
                                  _showTermsDialog();
                                },
                                child: Text(
                                  '《隐私政策》',
                                  style: TextStyle(
                                    color: Colors.blue.shade700,
                                    decoration: TextDecoration.underline,
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 20),
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
                                '注册',
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                      ),
                    ),
                    const SizedBox(height: 20),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Text('已有账号？'),
                        TextButton(
                          onPressed: () {
                            FocusScope.of(context).unfocus(); 
                            Navigator.pop(context);
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => const LoginPage(),
                              ),
                            );
                          },
                          child: Text(
                            '立即登录',
                            style: TextStyle(
                              color: Colors.blue.shade700,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 50)
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
