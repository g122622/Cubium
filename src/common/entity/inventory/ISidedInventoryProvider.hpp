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

#include "entity/inventory/ISidedInventory.hpp"
#include "util/Direction.hpp"
#include <memory>

namespace mc {

// Forward declarations
class BlockState;
class IWorld;
class BlockPos;

/**
 * @brief 侧面背包提供者接口
 *
 * 方块可以实现此接口来提供 ISidedInventory，用于复合方块（如熔炉组）的侧面访问。
 * 当方块需要根据世界状态动态创建 ISidedInventory 时使用此接口。
 *
 * 与直接实现 ISidedInventory 的区别：
 * - ISidedInventory: 方块实体直接实现，状态固定
 * - ISidedInventoryProvider: 根据世界状态动态创建，用于复合方块
 *
 * 参考: net.minecraft.inventory.ISidedInventoryProvider
 */
class ISidedInventoryProvider {
public:
    virtual ~ISidedInventoryProvider() = default;

    /**
     * @brief 创建侧面背包
     *
     * 根据方块状态和世界位置创建 ISidedInventory。
     * 用于复合方块（如多方块结构）的侧面访问。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @return 侧面背包指针，如果不可用返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<ISidedInventory> createInventory(
        const BlockState& state, IWorld& world, const BlockPos& pos) = 0;
};

} // namespace mc
