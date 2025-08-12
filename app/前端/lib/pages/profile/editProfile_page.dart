import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:image_picker/image_picker.dart';
import 'package:sduforum/pages/profile/profile_page.dart';

class EditProfilePage extends StatefulWidget {
  final String userId;
  final UserProfile initialProfile;

  const EditProfilePage({
    super.key,
    required this.userId,
    required this.initialProfile,
  });

  @override
  State<EditProfilePage> createState() => _EditProfilePageState();
}

class _EditProfilePageState extends State<EditProfilePage> {
  late TextEditingController _nickNameController;
  late TextEditingController _majorController;

  late String init_avatar_image;
  late String init_background_image;

  File? _avatar_image;
  File? _background_image;

  bool _isLoading = false;
  final Dio _dio = Dio();

  @override
  void initState() {
    super.initState();
    _nickNameController =
        TextEditingController(text: widget.initialProfile.nickName);
    _majorController = TextEditingController(text: widget.initialProfile.major);
    init_avatar_image = widget.initialProfile.avatarImageUrl;
    init_background_image = widget.initialProfile.backgroundImageUrl;
  }

  Future<void> _pickSingleImage(bool isAvatar) async {
    // 使用 image_picker 选择单张图片
    final pickedFile = await ImagePicker().pickImage(
      source: ImageSource.gallery,
      maxWidth: 800,
      maxHeight: 800,
    );

    if (pickedFile != null) {
      setState(() {
        if (isAvatar) {
          _avatar_image = File(pickedFile.path);
        } else {
          _background_image = File(pickedFile.path);
        }
      });
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('没有选择图片')),
      );
    }
  }

  @override
  void dispose() {
    _nickNameController.dispose();
    _majorController.dispose();
    super.dispose();
  }

  Future<void> _updateProfile() async {
    if (_isLoading) return;
    setState(() => _isLoading = true);
    String? _avatarImage;
    String? _backgroundImage;
    try {
      if (_avatar_image != null) {
        Uint8List avatarBytes = File(_avatar_image!.path).readAsBytesSync();
        _avatarImage = base64Encode(avatarBytes);
      } else {
        _avatarImage = '';
      }
      if (_background_image != null) {
        Uint8List backgroundBytes =
            File(_background_image!.path).readAsBytesSync();
        _backgroundImage = base64Encode(backgroundBytes);
      } else {
        _backgroundImage = '';
      }

      final response = await _dio.post(
        'http://120.26.127.37:8080/sdu_forum/api/user/edit_user_profile',
        data: {
          'user_id': widget.userId,
          'nickName': _nickNameController.text,
          'major': _majorController.text,
          'avatar_image': _avatarImage,
          'background_image': _backgroundImage,
        },
      );

      if (response.statusCode == 200) {
        Navigator.pop(context, true); // 返回并指示需要刷新
      }
    } catch (e) {
      //print('更新个人资料失败: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('更新失败')),
      );
    } finally {
      setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('编辑个人资料'),
        actions: [
          TextButton(
            onPressed: _isLoading ? null : _updateProfile,
            child: _isLoading
                ? const CircularProgressIndicator()
                : const Text('保存'),
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            GestureDetector(
              onTap: () => _pickSingleImage(true),
              child: Stack(
                alignment: Alignment.bottomRight,
                children: [
                  CircleAvatar(radius: 50, backgroundImage: _getAvatarImage()),
                  Container(
                    padding: const EdgeInsets.all(8),
                    decoration: BoxDecoration(
                      color: Colors.grey[800],
                      shape: BoxShape.circle,
                    ),
                    child:
                        const Icon(Icons.edit, color: Colors.white, size: 20),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 20),
            TextField(
              controller: _nickNameController,
              decoration: const InputDecoration(
                labelText: '昵称',
                border: OutlineInputBorder(),
              ),
              maxLength: 20,
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _majorController,
              decoration: const InputDecoration(
                labelText: '专业',
                border: OutlineInputBorder(),
              ),
              maxLength: 30,
            ),
            const SizedBox(height: 20),
            GestureDetector(
              onTap: () => _pickSingleImage(false),
              child: Container(
                height: 150,
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(12),
                  image: _getBackgroundImage(),
                  color: Colors.grey[200],
                ),
                child:
                    _background_image == null && (init_background_image.isEmpty)
                        ? const Center(
                            child: Column(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                Icon(Icons.add_photo_alternate, size: 40),
                                Text('设置背景图片'),
                              ],
                            ),
                          )
                        : null,
              ),
            ),
          ],
        ),
      ),
    );
  }

  ImageProvider? _getAvatarImage() {
    if (_avatar_image != null) {
      return FileImage(_avatar_image!);
    } else if (init_avatar_image.isNotEmpty) {
      return NetworkImage(
          "$init_avatar_image?t=${DateTime.now().millisecondsSinceEpoch}");
    } else {
      return null;
    }
  }

  DecorationImage? _getBackgroundImage() {
    if (_background_image != null) {
      return DecorationImage(
        image: FileImage(_background_image!),
        fit: BoxFit.cover,
      );
    } else if (init_background_image.isNotEmpty) {
      return DecorationImage(
        image: NetworkImage(
            "$init_background_image?t=${DateTime.now().millisecondsSinceEpoch}"),
        fit: BoxFit.cover,
      );
    } else {
      return null;
    }
  }
}
