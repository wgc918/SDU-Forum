import 'package:flutter/material.dart';

class AboutPage extends StatelessWidget {
  const AboutPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('关于我们')),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            Image.asset('asset/images/logo.png', height: 120),
            const SizedBox(height: 20),
            const Text(
              'SDU Forum',
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            const Text('版本 1.0.0'),
            const SizedBox(height: 30),
            const Text(
              '山东大学校园论坛',
              style: TextStyle(fontSize: 18),
            ),
            const SizedBox(height: 20),
            const Text(
              '为山东大学师生提供交流分享的平台',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 16),
            ),
            const SizedBox(height: 40),
            const Text('© 2023 SDU Forum 开发团队'),
            const SizedBox(height: 10),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                IconButton(
                  icon: const Icon(Icons.email),
                  onPressed: () => _launchEmail(),
                ),
                IconButton(
                  icon: const Icon(Icons.public),
                  onPressed: () => _launchWebsite(),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  void _launchEmail() async {
    
  }

  void _launchWebsite() async {
    
  }
}
