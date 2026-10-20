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

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

class Entity;

namespace blocks {

/**
 * @brief 青蛙卵方块
 *
 * 放置在水面上的青蛙卵，可孵化出蝌蚪。
 * 是一种非常薄的方块，只能放置在水源上方。
 *
 * 参考: net.minecraft.block.FrogspawnBlock
 */
class FrogspawnBlock : public Block {
public:
    /// 孵化 tick 延迟范围（含下限、不含上限，单位 tick）
    static constexpr i32 MIN_HATCH_TICK_DELAY = 3600;
    static constexpr i32 MAX_HATCH_TICK_DELAY = 12000;
    /// 单次孵化生成的蝌蚪数量范围（含下限、不含上限）
    static constexpr i32 MIN_TADPOLES_SPAWN = 2;
    static constexpr i32 MAX_TADPOLES_SPAWN = 5;

    explicit FrogspawnBlock(const BlockProperties& properties);

    ~FrogspawnBlock() override = default;

    /// 随机孵化延迟：nextInt(MIN_HATCH_TICK_DELAY, MAX_HATCH_TICK_DELAY)
    [[nodiscard]] static i32 getHatchDelayForTest(math::IRandom& random);

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /// 放置在水源方块正上方时可存活（下方为水、上方无流体）
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /// 放置时调度随机孵化 tick
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /// 孵化 tick 到期：仍可存活则孵化出蝌蚪，否则消失
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /// 面上方块变化时即时校验存活（下方水流走/上方被占用时立即消失）
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /// 下落方块落入时击碎青蛙卵
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

private:
    /// 在 pos 周围生成 2~5 只蝌蚪（位置在 0.2~0.8 方块内随机偏移，随机朝向）
    static void _spawnTadpoles(IWorld& world, const BlockPos& pos, math::IRandom& random);

    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
