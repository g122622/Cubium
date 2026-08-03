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

#include "common/core/Result.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp" // ir::play::ItemStackView

namespace mc {

class ItemStack;

namespace network {
namespace ir {

/**
 * @brief ItemStack ↔ ItemStackView 桥接（网络层视角）
 *
 * ItemStack（业务侧，src/common/item/core）持有丰富的组件成员状态；
 * ItemStackView（网络 IR，ir::play）持有 itemId + count + 组件补丁原始字节。
 * 本桥接在网络层完成双向转换，避免 item/core 反向依赖 network/ir。
 *
 * - toItemStackView：把 ItemStack 的组件字段导出为 DataComponentPatch，序列化为
 *   1.21.11 wire 字节存入 view.componentsPatch；itemId/count 直接取自 ItemStack。
 * - fromItemStackView：反序列化 view.componentsPatch 为 DataComponentPatch，应用到
 *   新建 ItemStack（按 itemId 查注册表还原 Item*）。
 */
[[nodiscard]] play::ItemStackView toItemStackView(const ItemStack& stack);

/**
 * @brief 由 ItemStackView 还原 ItemStack
 *
 * itemId=0 或 count<=0 返回空 ItemStack；否则按 itemId 查 ItemRegistry，再应用
 * 组件补丁。查不到 item 时返回错误。
 */
[[nodiscard]] Result<ItemStack> fromItemStackView(const play::ItemStackView& view);

} // namespace ir
} // namespace network
} // namespace mc
