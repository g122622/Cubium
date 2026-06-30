/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <string>
#include <string_view>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 去除 "minecraft:" 命名空间前缀
 *
 * 数据包 JSON 中类型字符串常带 "minecraft:" 前缀（如 "minecraft:block_ignore"），
 * 解析时需要剥离前缀再匹配枚举/查找表。此工具消除 5+ 处重复的 substr(0, 10) 样板。
 *
 * @param str 可能带 "minecraft:" 前缀的字符串
 * @return 去除前缀后的字符串（若无前缀则原样返回）
 */
inline std::string stripMinecraftPrefix(const std::string& str)
{
    constexpr std::string_view PREFIX = "minecraft:";
    if (str.size() > PREFIX.size() && str.compare(0, PREFIX.size(), PREFIX) == 0) {
        return str.substr(PREFIX.size());
    }
    return str;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
