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
#include <optional>
#include <string_view>

namespace mc {

// Forward declaration
enum class EquipmentSlot : u8;

namespace entity::serialization::EquipmentSlotNames {

/// 将 EquipmentSlot 转换为 NBT 键名（声明，实现在 .cpp 中）
[[nodiscard]] const char* toName(EquipmentSlot slot) noexcept;

/**
 * @brief 将 NBT 键名转换为 EquipmentSlot
 * @param name 键名（如 "mainhand", "offhand", "feet", "legs", "chest", "head"）
 * @return 对应的 EquipmentSlot，无效名称返回 std::nullopt
 */
[[nodiscard]] std::optional<EquipmentSlot> fromName(const std::string_view name) noexcept;

} // namespace entity::serialization::EquipmentSlotNames
} // namespace mc
