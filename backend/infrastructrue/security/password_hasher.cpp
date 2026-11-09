#include "security/password_hasher.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cctype>
#include <vector>

namespace infra
{
namespace
{

constexpr std::size_t kHashHexLength   = 64;
constexpr std::size_t kSaltHexLength   = 32;
constexpr std::size_t kSaltBytes       = kSaltHexLength / 2;
constexpr std::size_t kDerivedKeyBytes = 32;

bool decode_hex(const std::string& hex, std::vector<unsigned char>& out)
{
    if (hex.size() % 2 != 0)
        return false;

    out.resize(hex.size() / 2);

    for (std::size_t i = 0; i < hex.size(); i += 2)
    {
        for (std::size_t j = 0; j < 2; ++j)
        {
            if (!std::isxdigit(static_cast<unsigned char>(hex[i + j])))
                return false;
        }

        out[i / 2] = static_cast<unsigned char>(std::stoi(hex.substr(i, 2), nullptr, 16));
    }

    return true;
}

std::string encode_hex(const unsigned char* data, std::size_t size)
{
    static constexpr char kHexDigits[] = "0123456789abcdef";

    std::string out;
    out.reserve(size * 2);

    for (std::size_t i = 0; i < size; ++i)
    {
        out.push_back(kHexDigits[data[i] >> 4]);
        out.push_back(kHexDigits[data[i] & 0x0F]);
    }

    return out;
}

}  // namespace

bool verify_password(const std::string& password, const PasswordHash& stored)
{
    if (stored.hash.size() != kHashHexLength || stored.salt.size() != kSaltHexLength ||
        stored.iterations <= 0)
    {
        return false;
    }

    std::vector<unsigned char> salt;
    std::vector<unsigned char> expected;
    if (!decode_hex(stored.salt, salt) || !decode_hex(stored.hash, expected))
        return false;

    std::vector<unsigned char> derived(kDerivedKeyBytes);
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt.data(),
                          static_cast<int>(salt.size()), stored.iterations, EVP_sha512(),
                          static_cast<int>(derived.size()), derived.data()) != 1)
    {
        return false;
    }

    return CRYPTO_memcmp(derived.data(), expected.data(), derived.size()) == 0;
}

PasswordHash hash_password(const std::string& password, int iterations)
{
    PasswordHash result;

    if (iterations <= 0)
        return result;

    std::vector<unsigned char> salt(kSaltBytes);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1)
        return result;

    std::vector<unsigned char> derived(kDerivedKeyBytes);
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt.data(),
                          static_cast<int>(salt.size()), iterations, EVP_sha512(),
                          static_cast<int>(derived.size()), derived.data()) != 1)
    {
        return result;
    }

    result.hash       = encode_hex(derived.data(), derived.size());
    result.salt       = encode_hex(salt.data(), salt.size());
    result.iterations = iterations;
    return result;
}

}  // namespace infra
