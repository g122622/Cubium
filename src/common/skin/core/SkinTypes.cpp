/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SkinTypes.hpp"
#include <algorithm>
#include <cctype>

namespace mc::skin {

SkinType parseSkinType(const std::string& typeStr)
{
    // 转换为小写进行比较
    std::string lower;
    lower.reserve(typeStr.size());
    for (char c : typeStr) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (lower == "slim") {
        return SkinType::Slim;
    }
    // "default" 或任何无法识别的值都返回 Default
    return SkinType::Default;
}

std::string skinTypeToString(SkinType type)
{
    switch (type) {
        case SkinType::Slim:
            return "slim";
        case SkinType::Default:
        default:
            return "default";
    }
}

i32 calculateUUIDHashCode(const std::array<u8, 16>& uuid)
{
    // UUID 格式: mostSigBits(8字节) + leastSigBits(8字节)
    // Java UUID.hashCode(): (int)(mostSigBits >> 32) ^ (int)mostSigBits ^
    //                       (int)(leastSigBits >> 32) ^ (int)leastSigBits

    // 读取 mostSigBits (big-endian)
    u64 mostSigBits = 0;
    for (size_t i = 0; i < 8; ++i) {
        mostSigBits = (mostSigBits << 8) | uuid[i];
    }

    // 读取 leastSigBits (big-endian)
    u64 leastSigBits = 0;
    for (size_t i = 8; i < 16; ++i) {
        leastSigBits = (leastSigBits << 8) | uuid[i];
    }

    // 计算 hashCode
    // Java: int 是 32 位有符号整数，long 是 64 位有符号整数
    // (int)(mostSigBits >> 32) 取高 32 位
    // (int)mostSigBits 取低 32 位

    i32 mostHigh = static_cast<i32>(mostSigBits >> 32);
    i32 mostLow = static_cast<i32>(mostSigBits & 0xFFFFFFFF);
    i32 leastHigh = static_cast<i32>(leastSigBits >> 32);
    i32 leastLow = static_cast<i32>(leastSigBits & 0xFFFFFFFF);

    return mostHigh ^ mostLow ^ leastHigh ^ leastLow;
}

SkinType getDefaultSkinTypeForUUID(const std::array<u8, 16>& uuid)
{
    // 基于 UUID 哈希值的最低位确定皮肤类型
    i32 hashCode = calculateUUIDHashCode(uuid);
    return (hashCode & 1) == 1 ? SkinType::Slim : SkinType::Default;
}

} // namespace mc::skin
