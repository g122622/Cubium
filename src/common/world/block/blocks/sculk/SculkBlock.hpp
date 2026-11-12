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
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"

#include <optional>
#include <vector>

namespace mc {
namespace blocks {

class SculkSpreader;

/**
 * @brief 幽匿块
 *
 * 深暗之域的主要构成方块，可通过幽匿催化体蔓延。
 * 当被精确采集时掉落经验。
 *
 * 实现 SculkBehaviour：在 SculkSpreader 电荷作用下生成生长物（sensor/shrieker）。
 */
class SculkBlock : public Block, public SculkBehaviour {
public:
    explicit SculkBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    ~SculkBlock() override = default;

    /// 原版 SculkBlock 继承 DropExperienceBlock，挖掉固定掉 1 点经验（ConstantInt.of(1)）。
    [[nodiscard]] i32 dropExperience(math::IRandom& /*random*/) const override
    {
        return 1;
    }

    // ========== SculkBehaviour ==========

    /// MC SculkBlock.canChangeBlockStateOnSpread = false。
    [[nodiscard]] bool canChangeBlockStateOnSpread() const override
    {
        return false;
    }

    /// MC SculkBehaviour.updateDecayDelay = 1。
    [[nodiscard]] i32 updateDecayDelay(i32 decayDelay) const override
    {
        return 1;
    }

    /**
     * @brief MC SculkBehaviour.attemptSpreadVein：sculk 块不主动扩散脉络。
     *
     * SculkPatchFeature 的电荷游标在 sculk 块上时仅消耗电荷生成生长物（attemptUseCharge），
     * 不调用 vein 的脉络扩散。返回 false 与 MC SculkBlock（未覆写 attemptSpreadVein，
     * 走接口默认的 vein.spreadAll）行为不同——但 MC 中 sculk 块上 facings 为 nullopt，
     * 接口默认分支 attemptSpreadVein(facings==null) 才会触发 vein spreadAll。
     * 此处 worldgen 路径游标 facings 恒为 availableFaces（非空），故不扩散。
     */
    [[nodiscard]] bool attemptSpreadVein(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        std::optional<std::vector<Direction>> facings,
        bool worldGen) override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        MC_UNUSED(facings);
        MC_UNUSED(worldGen);
        return false;
    }

    /// MC SculkBlock.attemptUseCharge：消耗电荷、概率生成 sensor/shrieker 生长物。
    [[nodiscard]] i32 attemptUseCharge(ChargeCursor& cursor,
        IWorld& world,
        const BlockPos& origin,
        math::IRandom& random,
        SculkSpreader& spreader,
        bool shouldUpdateBlocks) override;

private:
    /// MC SculkBlock.getRandomGrowthState：1/11 概率 shrieker，否则 sensor；按流体水化。
    [[nodiscard]] static const BlockState* getRandomGrowthState(
        IWorld& world, const BlockPos& pos, math::IRandom& random, bool worldGen);

    /// MC SculkBlock.canPlaceGrowth：上方为空气/水，且 4×3×4 范围内 sensor+shrieker ≤ 2。
    [[nodiscard]] static bool canPlaceGrowth(IWorld& world, const BlockPos& pos);

    /// MC SculkBlock.getDecayPenalty：距中心越远衰减越多。
    [[nodiscard]] static i32 getDecayPenalty(
        const SculkSpreader& spreader, const BlockPos& pos, const BlockPos& origin, i32 charge);
};

} // namespace blocks
} // namespace mc
