import 'dart:convert';
import 'dart:typed_data';

import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:image_picker/image_picker.dart';
import 'dart:io';

class PostCreatePage extends StatefulWidget {
  final String userId;
  PostCreatePage({Key? key, required this.userId}) : super(key: key);

  @override
  State<PostCreatePage> createState() => _PostCreatePageState();
}

class _PostCreatePageState extends State<PostCreatePage> {
  final TextEditingController _contentController = TextEditingController();
  final TextEditingController _tagController = TextEditingController();
  final FocusNode _contentFocusNode = FocusNode();

  // 选中的图片列表
  List<File> _selectedImages = [];
  List<String> _selectedTags = [];
  // 发布状态
  bool _isPublishing = false;

  // 预设标签
  final List<String> _popularTags = [
    '二手闲置',
    '打听求助',
    '学习搭子',
    '社团活动',
    '校园招聘',
  ];

  @override
  void dispose() {
    _contentController.dispose();
    _tagController.dispose();
    _contentFocusNode.dispose();
    super.dispose();
  }

  Future<void> _pickImages() async {
    final picker = ImagePicker();
    final pickedFiles = await picker.pickMultiImage(
      maxWidth: 2000,
      maxHeight: 2000,
      imageQuality: 85,
    );
    setState(() {
      for (var image in pickedFiles) {
        if (_selectedImages.length < 9) {
          _selectedImages.add(File(image.path));
        } else {
          _showErrorSnackBar('最多只能选择9张图片');
          break;
        }
      }
    });
  }

  void _removeImage(int index) {
    setState(() {
      _selectedImages.removeAt(index);
    });
  }

  void _addTag() {
    final tag = _tagController.text.trim();
    if (tag.isNotEmpty && !_selectedTags.contains(tag)) {
      setState(() {
        _selectedTags.add(tag);
        _tagController.clear();
      });
    }
  }

  void _removeTag(String tag) {
    setState(() {
      _selectedTags.remove(tag);
    });
  }

  Future<void> _submitPost() async {
    if (_contentController.text.isEmpty) {
      _showErrorSnackBar('请填写内容');
      return;
    }

    setState(() => _isPublishing = true);

    try {
      List<String> imagePaths = [];
      if (_selectedImages.isNotEmpty) {
        for (var img in _selectedImages) {
          Uint8List imageBytes = File(img.path).readAsBytesSync();
          String base64Image = base64Encode(imageBytes);
          imagePaths.add(base64Image);
        }
      }
      Dio dio = Dio();
      Response response = await dio
          .post('http://120.26.127.37:8080/sdu_forum/api/post/create', data: {
        'user_id': widget.userId,
        'content': _contentController.text.trim(),
        'tag': _selectedTags,
        'images': imagePaths,
      });
      setState(() => _isPublishing = false);
      if (response.statusCode == 200) {
        // 发布成功
        _showSuccessSnackBar('发布成功');
        //print("发布成功: ${response.data}");
      } else {
        // 发布失败
        _showErrorSnackBar('发布失败: ${response.data}');
      }
    } catch (e) {
      setState(() => _isPublishing = false);
      //print("错误：$e");
    }

    // 清空输入框和选中的图片
    _contentController.clear();
    _tagController.clear();
    _selectedImages.clear();
    _selectedTags.clear();
    _contentFocusNode.unfocus();
  }

  void _showErrorSnackBar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: Colors.red[400],
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(10),
        ),
        margin: const EdgeInsets.all(10),
      ),
    );
  }

  void _showSuccessSnackBar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: Colors.green[400],
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(10),
        ),
        margin: const EdgeInsets.all(10),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Scaffold(
      appBar: AppBar(
        title: const Text('发布新帖子'),
        centerTitle: true,
        elevation: 0,
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 12),
            child: _isPublishing
                ? const SizedBox(
                    width: 24,
                    height: 24,
                    child: CircularProgressIndicator(strokeWidth: 3),
                  )
                : IconButton(
                    icon: const Icon(Icons.send_rounded),
                    onPressed: _submitPost,
                    tooltip: '发布',
                  ),
          ),
        ],
      ),
      body: GestureDetector(
        onTap: () => FocusScope.of(context).unfocus(),
        behavior: HitTestBehavior.translucent,
        child: SingleChildScrollView(
          padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // 分割线
              Divider(
                height: 1,
                thickness: 1,
                color: colorScheme.surfaceVariant.withOpacity(0.5),
              ),

              const SizedBox(height: 16),

              // 内容编辑
              TextField(
                controller: _contentController,
                focusNode: _contentFocusNode,
                maxLines: null,
                minLines: 8,
                style: TextStyle(
                  fontSize: 16,
                  height: 1.5,
                  color: colorScheme.onSurface,
                ),
                decoration: InputDecoration(
                  hintText: '分享你的想法...',
                  hintStyle: TextStyle(
                    color: colorScheme.onSurface.withOpacity(0.5),
                  ),
                  border: InputBorder.none,
                ),
              ),

              const SizedBox(height: 16),

              // 图片上传区域
              if (_selectedImages.isNotEmpty) ...[
                SizedBox(
                  height: 120,
                  child: ListView.builder(
                    scrollDirection: Axis.horizontal,
                    itemCount: _selectedImages.length,
                    itemBuilder: (context, index) {
                      return Padding(
                        padding: const EdgeInsets.only(right: 12),
                        child: ClipRRect(
                          borderRadius: BorderRadius.circular(12),
                          child: Stack(
                            children: [
                              Image.file(
                                _selectedImages[index],
                                width: 120,
                                height: 120,
                                fit: BoxFit.cover,
                              ),
                              Positioned(
                                top: 6,
                                right: 6,
                                child: GestureDetector(
                                  onTap: () => _removeImage(index),
                                  child: Container(
                                    padding: const EdgeInsets.all(4),
                                    decoration: BoxDecoration(
                                      shape: BoxShape.circle,
                                      color: colorScheme.errorContainer,
                                    ),
                                    child: Icon(
                                      Icons.close,
                                      size: 16,
                                      color: colorScheme.onErrorContainer,
                                    ),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      );
                    },
                  ),
                ),
                const SizedBox(height: 16),
              ],

              // 添加图片按钮
              OutlinedButton(
                onPressed: _pickImages,
                style: OutlinedButton.styleFrom(
                  padding: const EdgeInsets.symmetric(vertical: 14),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(12),
                  ),
                  side: BorderSide(
                    color: colorScheme.outline.withOpacity(0.3),
                  ),
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Icon(
                      Icons.image_outlined,
                      color: colorScheme.primary,
                    ),
                    const SizedBox(width: 8),
                    Text(
                      '添加图片',
                      style: TextStyle(
                        color: colorScheme.primary,
                        fontSize: 16,
                      ),
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 24),

              // 标签选择
              Text(
                '添加标签',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w600,
                  color: colorScheme.onSurface,
                ),
              ),

              const SizedBox(height: 12),

              // 已选标签
              if (_selectedTags.isNotEmpty) ...[
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: _selectedTags
                      .map((tag) => Chip(
                            label: Text(tag),
                            labelStyle: TextStyle(
                              color: colorScheme.onSecondaryContainer,
                            ),
                            backgroundColor: colorScheme.secondaryContainer,
                            deleteIcon: Icon(
                              Icons.close,
                              size: 18,
                              color: colorScheme.onSecondaryContainer,
                            ),
                            onDeleted: () => _removeTag(tag),
                          ))
                      .toList(),
                ),
                const SizedBox(height: 16),
              ],

              // 热门标签
              Text(
                '热门标签',
                style: TextStyle(
                  fontSize: 14,
                  fontWeight: FontWeight.w500,
                  color: colorScheme.onSurface.withOpacity(0.8),
                ),
              ),

              const SizedBox(height: 8),

              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: _popularTags
                    .where((tag) => !_selectedTags.contains(tag))
                    .map((tag) => FilterChip(
                          label: Text(tag),
                          labelStyle: TextStyle(
                            color: colorScheme.onSurfaceVariant,
                          ),
                          backgroundColor: colorScheme.surfaceVariant,
                          selected: false,
                          onSelected: (_) =>
                              setState(() => _selectedTags.add(tag)),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(8),
                          ),
                        ))
                    .toList(),
              ),

              const SizedBox(height: 16),

              // 自定义标签输入
              Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _tagController,
                      decoration: InputDecoration(
                        hintText: '自定义标签',
                        hintStyle: TextStyle(
                          color: colorScheme.onSurface.withOpacity(0.5),
                        ),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                          borderSide: BorderSide(
                            color: colorScheme.outline.withOpacity(0.3),
                          ),
                        ),
                        enabledBorder: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(12),
                          borderSide: BorderSide(
                            color: colorScheme.outline.withOpacity(0.3),
                          ),
                        ),
                        contentPadding: const EdgeInsets.symmetric(
                          horizontal: 16,
                          vertical: 12,
                        ),
                        suffixIcon: IconButton(
                          icon: Icon(
                            Icons.add_circle_outline,
                            color: colorScheme.primary,
                          ),
                          onPressed: _addTag,
                        ),
                      ),
                      onSubmitted: (_) => _addTag(),
                    ),
                  ),
                ],
              ),

              const SizedBox(height: 24),
            ],
          ),
        ),
      ),
    );
  }
}
