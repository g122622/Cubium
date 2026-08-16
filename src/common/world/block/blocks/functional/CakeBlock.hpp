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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;
class BlockActionResult;
class BlockRaycastResult;
enum class Hand : u8;

namespace blocks {

/**
 * @brief 蛋糕方块
 *
 * 可食用的方块，可以被分成7片食用。
 * 每次食用消耗一片，最后一片吃完后方块消失。
 *
 * 状态属性：
 * - BITES_0_6: 已吃的片数 (0-6)
 */
class CakeBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CakeBlock(const BlockProperties& properties);
    ~CakeBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /// 右键吃蛋糕：对齐 vanilla CakeBlock.use。
    /// 玩家 canEat(false)（非创造/旁观且饥饿<20）时吃一片（eatSlice bites+1）并恢复饥饿
    /// （foodStats().addStats(2, 0.1f)），返回 Success；否则返回 Pass（创造模式/满饥饿吃不了）。
    /// 注意：吃蛋糕不消耗手持物（空手右键即可），onBlockActivated 不检查 hand/heldItem。
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 工具方法 ==========

    /**
     * @brief 获取已吃的片数
     */
    [[nodiscard]] static i32 getBites(const BlockState& state) { return state.get(BlockStateProperties::BITES_0_6()); }

    /**
     * @brief 尝试吃蛋糕
     * @return 如果成功吃了返回true
     */
    static bool eatSlice(IWorld& world, const BlockPos& pos, const BlockState& state);

protected:
    /// 各片数的形状缓存
    std::array<CollisionShape, 7> m_shapesByBites;
};

} // namespace blocks
} // namespace mc
