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

#include "common/core/Types.hpp"
#include <string>
#include <string_view>

namespace mc {
namespace loot {

/**
 * @brief 列表操作的组合方式（对应原版 ListOperation.append / insert / replace_all）
 */
enum class ListOperationKind : u8 {
    ReplaceAll, ///< 用新列表整体替换
    Append,     ///< 追加到末尾（超出上限时截断）
    Insert,     ///< 在指定索引处插入
};

/**
 * @brief 解析 JSON 中的操作方式枚举
 *
 * @param name 操作名（"append" / "insert" / "replace_all"，可带 minecraft: 前缀）
 * @return 对应操作，未知名称返回 nullopt
 */
[[nodiscard]] std::optional<ListOperationKind> listOperationKindFromName(std::string_view name);

} // namespace loot
} // namespace mc
