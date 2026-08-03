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

#include "ChestBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <memory>

namespace mc {
namespace blocks {

/**
 * @brief 陷阱箱方块
 *
 * 继承自箱子方块，额外提供红石信号输出功能。
 * 输出的红石信号强度等于打开箱子的玩家数量（最大15）。
 */
class TrappedChestBlock : public ChestBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TrappedChestBlock(const BlockProperties& properties);

    // ========== 方块实体 ==========

    /**
     * @brief 创建陷阱箱方块实体
     * @param pos 方块位置
     * @return 陷阱箱方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 红石 ==========

    /**
     * @brief 检查是否可以提供红石信号
     * @return 始终返回true
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override { return true; }

    /**
     * @brief 获取弱红石信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取强红石信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 仅从顶面输出强信号
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取方块实体类型
     * @return TrappedChest类型
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const override { return BlockEntityType::TrappedChest; }
};

} // namespace blocks
} // namespace mc
