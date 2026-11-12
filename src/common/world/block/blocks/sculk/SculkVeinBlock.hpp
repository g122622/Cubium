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

#include "SculkBehaviour.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/multiface/MultifaceBlock.hpp"
#include "common/world/block/blocks/multiface/MultifaceSpreader.hpp"
#include "common/world/block/capability/IWaterLoggable.hpp"
#include "common/world/block/core/Block.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 幽匿脉络
 *
 * 多面方块（继承 MultifaceBlock），可附着在方块六面，支持含水。
 * 可被骨粉催生（spreadFromRandomFaceTowardRandomDirection）。
 *
 * 实现 SculkBehaviour：worldgen 电荷作用下把相邻可替换方块转化为 sculk，并在其周围蔓延脉络。
 */
class SculkVeinBlock : public MultifaceBlock, public SculkBehaviour {
public:
    explicit SculkVeinBlock(const BlockProperties& properties);

    ~SculkVeinBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // 注意：vanilla SculkVeinBlock 及其父类 MultifaceBlock 都没有重写
    // useShapeForLightOcclusion，取 BlockBehaviour 的默认值 false。早期这里照抄了
    // SculkSensorBlock 的写法返回 true，把幽匿脉络的按形状面遮挡逻辑错误打开。

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /// MC MultifaceSpreadeableBlock.getSpreader：DEFAULT_SPREAD_ORDER 的脉络扩散器。
    [[nodiscard]] const MultifaceSpreader& getSpreader() const
    {
        return m_veinSpreader;
    }

    /// MC SculkVeinBlock.getSameSpaceSpreader：仅 SAME_POSITION 的扩散器。
    [[nodiscard]] const MultifaceSpreader& getSameSpaceSpreader() const
    {
        return m_sameSpaceSpreader;
    }

    // ========== SculkBehaviour ==========

    /// MC SculkVeinBlock.attemptUseCharge：worldgen 时尝试把相邻可替换方块转化为 sculk。
    [[nodiscard]] i32 attemptUseCharge(ChargeCursor& cursor,
        IWorld& world,
        const BlockPos& origin,
        math::IRandom& random,
        SculkSpreader& spreader,
        bool shouldUpdateBlocks) override;

    /// MC SculkBehaviour.attemptSpreadVein（接口默认分支）：facings 为空时走 sameSpaceSpreader。
    [[nodiscard]] bool attemptSpreadVein(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        std::optional<std::vector<Direction>> facings,
        bool worldGen) override;

    /// MC SculkVeinBlock.onDischarged：移除贴在 sculk 上的面，无面则变空气/水。
    void onDischarged(IWorld& world, const BlockState& state, const BlockPos& pos, math::IRandom& random) override;

    /// MC SculkBehaviour.canChangeBlockStateOnSpread = true（接口默认）。
    [[nodiscard]] bool canChangeBlockStateOnSpread() const override
    {
        return true;
    }

    // ========== 静态方法 ==========

    /// MC SculkVeinBlock.hasSubstrateAccess：vein 某面朝向 SCULK_REPLACEABLE 方块。
    [[nodiscard]] static bool hasSubstrateAccess(IWorld& world, const BlockState& state, const BlockPos& pos);

    /// MC SculkVeinBlock.regrow：从一组方向重建 vein（仅可 attach 的方向加面）。
    [[nodiscard]] static bool regrow(
        IWorld& world, const BlockPos& pos, const BlockState& current, const std::vector<Direction>& directions);

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override
    {
        MC_UNUSED(container);
    }

private:
    /// MC SculkVeinBlock.attemptPlaceSculk：随机方向遍历，把相邻可替换方块转化为 sculk 并蔓延脉络。
    [[nodiscard]] bool attemptPlaceSculk(
        SculkSpreader& spreader, IWorld& world, const BlockPos& pos, math::IRandom& random);

    /// 预计算 64 种面组合形状（2^6）。
    std::array<CollisionShape, 64> m_shapes;
    static size_t shapeIndex(const BlockState& state);

    MultifaceSpreader m_veinSpreader;
    MultifaceSpreader m_sameSpaceSpreader;
};

} // namespace blocks
} // namespace mc
