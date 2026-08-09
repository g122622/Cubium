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

namespace mc::entity {

/**
 * @brief 箭矢拾取状态
 *
 * 对齐 vanilla 1.21.11 AbstractArrow.Pickup 枚举（NBT pickup 键 byte 值）。
 * 本枚举原内联于 AbstractArrowEntity.hpp，批次6 子目标2 将箭矢状态字段迁入
 * ProjectileArrowStateComponent 时提取为独立头，使组件能以值类型承载而不循环
 * 依赖 AbstractArrowEntity.hpp（参照 EntityFlags/EquipmentSlot 提取先例）。
 */
enum class PickupStatus : u8 {
    Disallowed,  // 不允许拾取
    Allowed,     // 允许拾取
    CreativeOnly // 仅创造模式拾取
};

} // namespace mc::entity
