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

#pragma once

#include "common/util/nbt/Nbt.hpp"

#include <string_view>

#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace village {
namespace detail {

/**
 * @brief 读取村庄存档中的数值字段，字段缺失或类型不符时记录告警并返回兜底值
 *
 * 村庄存档来自磁盘，属于不可信输入：键缺失、类型被写成别的标签都必须降级处理，
 * 不能用 `compound_tag::get<TagT>`（它会断言终止进程），否则单个损坏的村庄数据
 * 就能打死整个服务器。
 *
 * @param tag 待读取的复合标签
 * @param key 字段名
 * @param fallback 字段缺失或类型不符时使用的兜底值
 * @param context 归属的读取函数名，仅用于告警定位
 */
template <typename TagT>
[[nodiscard]] typename TagT::value_type readFieldOr(const nbt::tags::compound_tag& tag,
    std::string_view key,
    typename TagT::value_type fallback,
    std::string_view context)
{
    if (const auto* found = tag.findAs<TagT>(key); found != nullptr) {
        return found->value;
    }

    spdlog::warn("{}: missing or invalid NBT field '{}', using fallback value", context, key);
    return fallback;
}

} // namespace detail
} // namespace village
} // namespace world
} // namespace mc
