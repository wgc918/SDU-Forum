import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/main.dart';
import 'package:sduforum/pages/home_page.dart';
import 'package:sduforum/utils/tokenManager.dart';

class TokenInterceptor extends Interceptor {
  final Dio dio;

  TokenInterceptor(this.dio);

  @override
  Future<void> onRequest(
    RequestOptions options,
    RequestInterceptorHandler handler,
  ) async {
    // 从本地获取access token
    final tokens = await TokenManager.getTokens();
    if (tokens != null) {
      options.headers['Authorization'] = 'Bearer ${tokens['accessToken']}';
    }
    handler.next(options);
  }

  @override
  Future<void> onError(
    DioException err,
    ErrorInterceptorHandler handler,
  ) async {
    // 如果access token过期，尝试用refresh token刷新
    if (err.response?.statusCode == 401) {
      final tokens = await TokenManager.getTokens();
      if (tokens != null) {
        try {
          // 调用刷新token的API
          final response = await dio.post(
            'http://101.76.244.44:8080/sdu_forum/api/user/refresh_token',
            data: {
              'refresh_token': tokens['refreshToken'],
              'user_id': tokens['userId'],
            },
          );

          if (response.statusCode == 200) {
            // 保存新的token
            await TokenManager.saveTokens(
              accessToken: response.data['access_token'],
              accessTokenExp: response.data['access_token_exp'].toString(),
              refreshToken: tokens['refreshToken'] ?? '',
              refreshTokenExp: tokens['refreshTokenExp'] ?? '',
              userId: tokens['userId'].toString(),
            );

            // 重新发起原始请求
            final opts = err.requestOptions;
            opts.headers['Authorization'] =
                'Bearer ${response.data['access_token']}';
            final retryResponse = await dio.fetch(opts);
            return handler.resolve(retryResponse);
          }
        } catch (e) {
          // 刷新token失败，跳转到登录页
          await TokenManager.clearTokens();
          navigatorKey.currentState?.pushAndRemoveUntil(
            MaterialPageRoute(builder: (_) => HomePage(dio: dio)),
            (route) => false,
          );
        }
      }
    }
    handler.next(err);
  }
}
