#include <chrono>

#include "auth_service.h"

namespace
{
/**
 * @brief 将当前时间加上指定的时间，返回格式化后的字符串
 *
 * @param time_type 时间单位: 0-分钟(m), 1-小时(h), 2-天(d)
 * @param time      要加上的时间数值
 * @return std::string 格式为 "YYYY-MM-DD HH:MM:SS" 的时间字符串
 */
std::string addTimeToNow(uint16_t time_type, uint32_t time)
{
    auto now = std::chrono::system_clock::now();

    std::chrono::seconds offset{0};
    switch (time_type)
    {
        case 0:
            offset = std::chrono::seconds(static_cast<long long>(time) * 60);
            break;
        case 1:
            offset = std::chrono::seconds(static_cast<long long>(time) * 3600);
            break;
        case 2:
            offset = std::chrono::seconds(static_cast<long long>(time) * 86400);
            break;
        default:
            break;
    }

    auto        new_time = now + offset;
    std::time_t tt       = std::chrono::system_clock::to_time_t(new_time);

    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    char buf[32];  // "YYYY-MM-DD HH:MM:SS" 共 19 字符 + '\0'
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", local_tm.tm_year + 1900,
                  local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min,
                  local_tm.tm_sec);

    return std::string(buf);
}
}  // namespace

namespace admin::service
{

}