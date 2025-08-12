#include"utils_hash.h"


namespace Hash
{
	void Hash::handle_openssl_error()
	{
		throw std::runtime_error("Openssl error occurred!");
	}

	PasswordData Hash::create_password_hash(const std::string& password)
	{
		const int iterations = 1500;
		const int salt_bytes = 16;  // 16字节盐值 → 32字符十六进制
		const int hash_bytes = 32;  // 32字节哈希 → 64字符十六进制

		//生成盐值
		std::vector<unsigned char> salt(salt_bytes);
		if (RAND_bytes(salt.data(), salt_bytes) != 1)
		{
			handle_openssl_error();
		}

		//计算哈希
		std::vector<unsigned char> key(hash_bytes);
		if (PKCS5_PBKDF2_HMAC(
			password.c_str(), password.length(),
			salt.data(), salt.size(),
			iterations,
			EVP_sha512(),
			key.size(), key.data()) != 1)
		{
			handle_openssl_error();
		}

		//转换为16进制字符串
		auto  to_hex = [](const unsigned char* data, size_t len)
			{
				std::stringstream ss;
				for (size_t i = 0; i < len; i++)
				{
					ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
				}
				return ss.str();
			};

		return {
			to_hex(key.data(),key.size()),
			to_hex(salt.data(),salt.size()),
			iterations

		};
	}

	bool Hash::verify_password(const std::string& password, const PasswordData& stored)
	{
		//检查长度是否符合
		if (stored.hash.length() != 64 || stored.salt.length() != 32)
		{
			throw std::runtime_error("Invalid hash or salt length in stored data");
		}

		// 将存储的盐值转换回字节
		const int salt_bytes = 16;
		std::vector<unsigned char> salt(salt_bytes);

		for (size_t i = 0; i < stored.salt.length(); i += 2)
		{
			salt[i / 2] = static_cast<unsigned char>(
				std::stoi(stored.salt.substr(i, 2), nullptr, 16));
		}

		// 计算输入密码的哈希
		const int hash_bytes = 32;
		std::vector<unsigned char> key(hash_bytes);

		if (PKCS5_PBKDF2_HMAC(
			password.c_str(), password.length(),
			salt.data(), salt.size(),
			stored.iterations,
			EVP_sha512(),
			key.size(), key.data()) != 1) {
			handle_openssl_error();
		}

		// 比较哈希值
		std::stringstream ss;
		for (size_t i = 0; i < key.size(); i++)
		{
			ss << std::hex << std::setw(2) << std::setfill('0') << (int)key[i];
		}
		return ss.str() == stored.hash;
	}

}
