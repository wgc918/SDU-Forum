import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:sduforum/core/main_frame.dart';
import 'package:sduforum/utils/tokenManager.dart';
import 'package:sduforum/pages/home_page.dart';
import 'package:sduforum/utils/tokenInterceptor.dart';

final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  final dio = Dio();
  dio.interceptors.add(TokenInterceptor(dio));

  final isLoggedIn = await _checkLoginStatus();

  runApp(MyApp(
    dio: dio,
    initialRoute: isLoggedIn ? '/' : '/login',
  ));
}

Future<bool> _checkLoginStatus() async {
  final tokens = await TokenManager.getTokens();
  if (tokens == null) {
    //print('No tokens found, returning false');
    return false;
  }
  final accessTokenExp = tokens['accessTokenExp'];

  // 检查 accessTokenExp 是否存在
  if (accessTokenExp == null) {
    //print('Access token expiry is null, returning false');
    return false;
  }

  // 解析过期时间
  final expiryDate =
      DateTime.fromMillisecondsSinceEpoch(int.parse(accessTokenExp) * 1000);

  // 检查是否过期
  if (expiryDate.isBefore(DateTime.now())) {
    //print('Access token expired, attempting to refresh...');
    try {
      final dio = Dio();
      final response = await dio.post(
        'http://101.76.244.44:8080/sdu_forum/api/user/refresh_token',
        data: {
          'refresh_token': tokens['refreshToken']??'',
          'user_id': tokens['userId'],
        },
      );
      print(response.data);

      if (response.statusCode == 200 &&
          response.data['refresh_state'] == true) {
        await TokenManager.saveTokens(
          accessToken: response.data['access_token'],
          accessTokenExp: response.data['access_token_exp'].toString(),
          refreshToken: tokens['refreshToken'] ?? '',
          refreshTokenExp: tokens['refreshTokenExp'] ?? '',
          userId: tokens['userId'] ?? '',
        );
        //print('Token refreshed successfully');
        return true;
      } else {
        //print('Refresh token failed: invalid response');
        await TokenManager.clearTokens();
        return false;
      }
    } catch (e) {
      //print('Token refresh failed: $e');
      await TokenManager.clearTokens();
      return false;
    }
  } else {
    //print('Access token is valid, not expired');
    return true; 
  }
}

Future<Widget> _buildMainFramePage(Dio dio) async {
  final tokens = await TokenManager.getTokens();
  return MainFramePage(
    dio: dio,
    account: tokens?['userId'] ?? '',
    initialTabIndex: 0,
  );
}

class MyApp extends StatelessWidget {
  final Dio dio;
  final String initialRoute;

  const MyApp({
    super.key,
    required this.dio,
    required this.initialRoute,
  });

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      navigatorKey: navigatorKey,
      title: 'SDU Forum',
      initialRoute: initialRoute,
      routes: {
        '/': (context) => FutureBuilder(
              future: _buildMainFramePage(dio),
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.done) {
                  return snapshot.data ?? const SizedBox();
                }
                return const CircularProgressIndicator();
              },
            ),
        '/login': (context) => HomePage(dio: dio),
      },
      theme: ThemeData(
        primarySwatch: Colors.blue,
        visualDensity: VisualDensity.adaptivePlatformDensity,
      ),
    );
  }
}
