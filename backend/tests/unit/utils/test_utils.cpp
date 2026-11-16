/**
 * @file test_utils.cpp
 * @brief utils 时间格式化单元测试
 *
 * 覆盖范围:
 * - epoch 起点
 * - 固定业务时间
 * - 32 位 time_t 上界
 */

#include "doctest.h"

#include "utils/utils.h"

TEST_SUITE("Utils")
{
    TEST_CASE("timestamp_to_string formats epoch deterministically")
    {
        CHECK(utils::timestamp_to_string(0) == "1970-01-01 00:00:00");
        CHECK(utils::timestamp_to_string(1700000000) == "2023-11-14 22:13:20");
        CHECK(utils::timestamp_to_string(2147483647) == "2038-01-19 03:14:07");
    }
}
