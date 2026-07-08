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

#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

#include <optional>
#include <vector>

namespace mc {
namespace blocks {

class SculkSpreader;
class ChargeCursor;

/**
 * @brief 幽匿行为接口（MC SculkBehaviour）
 *
 * 实现本接口的方块可被 SculkSpreader 的电荷游标（ChargeCursor）作用：
 * - attemptSpreadVein：游标 update 时先尝试从当前面扩散脉络。
 * - attemptUseCharge：消耗电荷、放置幽匿生长物或衰减。
 * - onDischarged：电荷耗尽时清理（脉络把贴在 sculk 上的面移除）。
 *
 * MC 1.21.11 中 SculkBlock 与 SculkVeinBlock 实现本接口。
 */
class SculkBehaviour {
public:
    virtual ~SculkBehaviour() = default;

    /// MC SculkBehaviour.canChangeBlockStateOnSpread：扩散后是否可能改变方块状态。
    [[nodiscard]] virtual bool canChangeBlockStateOnSpread() const { return true; }

    /// MC SculkBehaviour.getSculkSpreadDelay：游标每次 update 后的延迟（tick）。
    [[nodiscard]] virtual u8 getSculkSpreadDelay() const { return 1; }

    /// MC SculkBehaviour.onDischarged：电荷耗尽时回调（默认无操作）。
    virtual void onDischarged(IWorld& world, const BlockState& state, const BlockPos& pos, math::IRandom& random)
    {
        MC_UNUSED(world);
        MC_UNUSED(state);
        MC_UNUSED(pos);
        MC_UNUSED(random);
    }

    /// MC SculkBehaviour.depositCharge：世界生成期间的额外电荷沉积（默认 false）。
    [[nodiscard]] virtual bool depositCharge(IWorld& world, const BlockPos& pos, math::IRandom& random)
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(random);
        return false;
    }

    /// MC SculkBehaviour.updateDecayDelay：更新衰减延迟（默认 1）。
    [[nodiscard]] virtual i32 updateDecayDelay(i32 decayDelay) const { return 1; }

    /**
     * @brief MC SculkBehaviour.attemptSpreadVein：
     *        游标在当前位置先尝试扩散脉络。
     *
     * @param world 世界
     * @param pos 当前游标位置
     * @param state 当前位置方块状态
     * @param facings 游标携带的面数据（worldgen 期可能为 nullopt）
     * @param worldGen 是否世界生成阶段
     * @return 是否成功扩散
     */
    [[nodiscard]] virtual bool attemptSpreadVein(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        std::optional<std::vector<Direction>> facings,
        bool worldGen) = 0;

    /**
     * @brief MC SculkBehaviour.attemptUseCharge：消耗电荷的核心逻辑。
     *
     * @param cursor 当前游标（可读 charge/decayDelay/pos）
     * @param world 世界
     * @param origin 电荷源（SculkSpreader.updateCursors 传入的中心点）
     * @param random 随机源
     * @param spreader 扩散器
     * @param shouldUpdateBlocks 是否允许实际修改方块（worldgen 期始终 true）
     * @return 剩余电荷（<=0 表示耗尽）
     */
    [[nodiscard]] virtual i32 attemptUseCharge(ChargeCursor& cursor,
        IWorld& world,
        const BlockPos& origin,
        math::IRandom& random,
        SculkSpreader& spreader,
        bool shouldUpdateBlocks) = 0;
};

} // namespace blocks
} // namespace mc
