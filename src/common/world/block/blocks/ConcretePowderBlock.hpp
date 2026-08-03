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

#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"

namespace mc {

// 前向声明
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 混凝土粉末方块
 *
 * 混凝土粉末是受重力影响的方块，接触水时会固化为对应颜色的混凝土。
 * 继承自 FallingBlock，实现以下特殊行为：
 *
 * 1. 放置时检查接触水源，如果接触水则直接固化为混凝土
 * 2. 邻居方块更新时检查是否接触水，如果接触水则固化为混凝土
 * 3. 下落落地时检查落地点是否接触水，如果接触水则固化为混凝土
 * 4. 下落过程中穿过水时提前固化（由 FallingBlockEntity 处理）
 *
 * 参考: net.minecraft.world.level.block.ConcretePowderBlock
 */
class ConcretePowderBlock : public FallingBlock {
public:
    /**
     * @brief 构造函数
     *
     * @param properties 方块属性
     * @param concrete 对应颜色的混凝土方块指针
     */
    ConcretePowderBlock(const BlockProperties& properties, const Block* concrete);

    ~ConcretePowderBlock() override = default;

    /**
     * @brief 放置时检查接触水源
     *
     * 如果放置位置已接触水，直接返回混凝土的默认状态而非粉末状态。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居更新时检查接触水源
     *
     * 如果检测到接触水，返回混凝土状态，否则走 FallingBlock 的正常更新逻辑。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 下落落地时的回调
     *
     * 如果落地点接触水，将自身替换为对应颜色的混凝土。
     */
    void onEndFalling(IWorld& world,
        const BlockPos& pos,
        const BlockState& fallingState,
        const BlockState& hitState,
        entity::FallingBlockEntity& entity) override;

    /**
     * @brief 获取对应颜色的混凝土方块
     */
    [[nodiscard]] const Block* getConcreteBlock() const { return m_concrete; }

private:
    /**
     * @brief 检查指定位置是否应该固化
     *
     * 当前方块的流体状态为水，或任意邻居方向有水源时返回 true。
     *
     * @param world 世界
     * @param pos 位置
     * @param state 位置上的方块状态
     * @return 如果应该固化返回 true
     */
    [[nodiscard]] static bool shouldSolidify(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查指定方块状态是否可以导致固化
     *
     * 如果方块的流体状态为水（水源或流动水），则可导致固化。
     *
     * @param state 方块状态
     * @return 如果该方块的水可以导致固化返回 true
     */
    [[nodiscard]] static bool canSolidify(const BlockState& state);

    /**
     * @brief 检查指定位置是否接触液体
     *
     * 遍历六个方向检查是否有水流体。
     * 对齐 MC 1.21.11 ConcretePowderBlock.touchesLiquid()。
     *
     * @param world 世界
     * @param pos 位置
     * @return 如果接触水返回 true
     */
    [[nodiscard]] static bool touchesLiquid(IWorld& world, const BlockPos& pos);

    const Block* m_concrete; ///< 对应颜色的混凝土方块
};

} // namespace blocks
} // namespace mc
