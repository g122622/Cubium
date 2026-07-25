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
#include <vector>

namespace mc::network::ir::play {

/**
 * @brief 物品栈（网络层视角：itemId + count + 组件补丁原始字节）
 *
 * 1.21.11 数据组件模型：count<=0 即空；否则 itemId(VarInt) + DataComponentPatch wire。
 * componentsPatch 承载 DataComponentPatch 的 1.21.11 wire 序列化字节（见
 * item/component/DataComponentPatchWire.hpp）；与业务侧 ItemStack 的双向转换由
 * network/ir/ItemStackBridge.hpp 提供。
 *
 * 独立成头是为了让 entity/core（EntityDataManager 的 DataValue variant）能以最小代价
 * 持有物品元数据变体，而不拖入整个 play 包定义。
 */
struct ItemStackView {
    u32 itemId = 0; // 0=空
    i32 count = 0;
    std::vector<u8> componentsPatch; // DataComponentPatch 的 1.21.11 wire 字节

    // std::variant 比较要求备选项类型可相等比较（DataValue::operator==/!= 经由 variant 默认 == 实例化）。
    [[nodiscard]] friend bool operator==(const ItemStackView&, const ItemStackView&) noexcept = default;
};

} // namespace mc::network::ir::play
