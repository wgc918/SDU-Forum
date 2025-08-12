import 'package:dio/dio.dart';
import 'package:flutter/material.dart';

class HelpFeedbackPage extends StatefulWidget {
  final String user_id;
  const HelpFeedbackPage({super.key, required this.user_id});

  @override
  State<HelpFeedbackPage> createState() => _HelpFeedbackPageState();
}

class _HelpFeedbackPageState extends State<HelpFeedbackPage> {
  final TextEditingController _feedbackController = TextEditingController();
  final Dio _dio = Dio();

  @override
  void dispose() {
    _feedbackController.dispose();
    super.dispose();
  }

  Future<void> _submitFeedback() async {
     if (_feedbackController.text.trim().isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('反馈内容不能为空')),
      );
      return;
    }
    //print('你好：${_feedbackController.text}');
    //if (_feedbackController.text.trim()=='') return;
    print(widget.user_id);
    try {
      var response = await _dio.post(
        'http://120.26.127.37:8080/sdu_forum/api/feedback/submit',
        data: {
          'feedback': _feedbackController.text.trim(),
          'user_id': widget.user_id,
        },
      );

      if (response.statusCode == 200) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('反馈提交成功'),backgroundColor: Colors.green,),
        );
        _feedbackController.clear();
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('提交失败'),backgroundColor: Colors.red,),
        );
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('提交失败: ${e.toString()}')),
      );
      //print('提交失败: ${e.toString()}');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('帮助与反馈')),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              '常见问题',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            _buildFAQItem('如何修改个人资料？', '在个人资料页面点击编辑按钮即可修改'),
            _buildFAQItem('忘记密码怎么办？', '在登录页面点击"忘记密码"，按照提示重置密码'),
            _buildFAQItem('如何联系客服？', '发送邮件至3524515056@qq.com'),
            const SizedBox(height: 30),
            const Text(
              '意见反馈',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            TextField(
              controller: _feedbackController,
              maxLines: 5,
              decoration: const InputDecoration(
                hintText: '请输入您的意见或建议...',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 10),
            ElevatedButton(
              onPressed: () => _submitFeedback(),
              child: const Text('提交反馈'),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildFAQItem(String question, String answer) {
    return ExpansionTile(
      title: Text(question),
      children: [ListTile(title: Text(answer))],
    );
  }
}
