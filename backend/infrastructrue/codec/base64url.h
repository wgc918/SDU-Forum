#pragma once

#include <openssl/evp.h>

#include <cstddef>
#include <string>
#include <vector>

namespace infra::codec
{

/// Base64url 编码（RFC 4648 §5，去掉 '=' padding）
/// @return 成功返回 true 并写入 output
inline bool base64_url_encode(const std::string& input, std::string& output)
{
    std::string out;
    out.resize(static_cast<std::size_t>(4 * ((input.size() + 2) / 3)) + 1);

    int written = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out.data()),
                                  reinterpret_cast<const unsigned char*>(input.data()),
                                  static_cast<int>(input.size()));
    if (written < 0)
        return false;

    out.resize(static_cast<std::size_t>(written));

    for (char& ch : out)
    {
        if (ch == '+')
            ch = '-';
        else if (ch == '/')
            ch = '_';
    }

    while (!out.empty() && out.back() == '=')
        out.pop_back();

    output = std::move(out);
    return true;
}

/// Base64url 解码（容忍缺失 padding）
inline bool base64_url_decode(const std::string& input, std::string& output)
{
    std::string base64 = input;
    for (char& ch : base64)
    {
        if (ch == '-')
            ch = '+';
        else if (ch == '_')
            ch = '/';
    }

    if (base64.size() % 4 == 1)
        return false;

    std::size_t padding = (4 - base64.size() % 4) % 4;
    base64.append(padding, '=');

    std::vector<unsigned char> buffer(base64.size() / 4 * 3);
    int                        written =
        EVP_DecodeBlock(buffer.data(), reinterpret_cast<const unsigned char*>(base64.data()),
                        static_cast<int>(base64.size()));
    if (written < 0 || static_cast<std::size_t>(written) < padding)
        return false;

    output.assign(reinterpret_cast<char*>(buffer.data()),
                  static_cast<std::size_t>(written) - padding);
    return true;
}

}  // namespace infra::codec
