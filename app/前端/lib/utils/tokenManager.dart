import 'package:shared_preferences/shared_preferences.dart';

class TokenManager {
  static const String _accessTokenKey = 'access_token';
  static const String _accessToken_expKey='access_token_exp';
  static const String _refreshTokenKey = 'refresh_token';
  static const String _refreshToken_expKey='refresh_token_exp';
  static const String _userIdKey = 'user_id';

  static Future<void> saveTokens({
    required String accessToken,
    required String accessTokenExp,
    required String refreshToken,
    required String refreshTokenExp,
    required String userId,
  }) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_accessTokenKey, accessToken);
    await prefs.setString(_accessToken_expKey, accessTokenExp);
    await prefs.setString(_refreshTokenKey, refreshToken);
    await prefs.setString(_refreshToken_expKey, refreshTokenExp);
    await prefs.setString(_userIdKey, userId);
  }

  static Future<Map<String, String>?> getTokens() async {
    final prefs = await SharedPreferences.getInstance();
    final accessToken = prefs.getString(_accessTokenKey);
    final accessTokenExp = prefs.getString(_accessToken_expKey);
    final refreshToken = prefs.getString(_refreshTokenKey);
    final refreshTokenExp = prefs.getString(_refreshToken_expKey);
    final userId = prefs.getString(_userIdKey);

    if (accessToken != null && refreshToken != null && userId != null&&
        accessTokenExp != null && refreshTokenExp != null) {
      return {
        'accessToken': accessToken,
        'accessTokenExp': accessTokenExp,
        'refreshToken': refreshToken,
        'refreshTokenExp': refreshTokenExp,
        'userId': userId,
      };
    }
    return null;
  }

  static Future<void> clearTokens() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_accessTokenKey);
    await prefs.remove(_accessToken_expKey);
    await prefs.remove(_refreshTokenKey);
    await prefs.remove(_refreshToken_expKey);
    await prefs.remove(_userIdKey);
  }
}
