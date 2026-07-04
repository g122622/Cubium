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

#include "../../../../util/TriState.hpp"
#include "../../../IWorld.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../BlockPos.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 眼眸花环境属性查询工具
 *
 * 提供 EYEBLOSSOM_OPEN 环境属性的轻量级近似查询：
 * - 主世界：根据 dayTimeOfDay 在 12600~23400 之间返回 True（夜晚开放），其他返回 False
 * - 下界/末地：返回 Default（不主动切换，眼眸花保持当前状态）
 *
 * 时间区间 [12600, 23401) 来自 MC 1.21.11 的 Timelines.IN_OVERWORLD 中
 * EYEBLOSSOM_OPEN 关键帧：
 *   addKeyframe(12600, TriState.TRUE)
 *   addKeyframe(23401, TriState.FALSE)
 *
 * TODO: 完整实现 EnvironmentAttributes 系统后，本工具应被替换为
 * world.environmentAttributes().getValue(EnvironmentAttributes::EYEBLOSSOM_OPEN, pos)。
 * 完整系统应包含 EnvironmentAttribute / EnvironmentAttributeMap /
 * EnvironmentAttributeSystem / Timeline / KeyframeTrack 等组件，
 * 支持基于位置/群系/维度的属性查询、网络同步、空间插值等。
 */
namespace eyeblossom_environment {

/// 主世界 EYEBLOSSOM_OPEN 关键帧：夜晚开始（日落 2 小时后）切到 True
inline constexpr i64 EYEBLOSSOM_OPEN_TICK = 12600;
/// 主世界 EYEBLOSSOM_OPEN 关键帧：白天开始（日出 1 小时前）切到 False
inline constexpr i64 EYEBLOSSOM_CLOSE_TICK = 23401;

/**
 * @brief 查询当前位置的 EYEBLOSSOM_OPEN 环境属性
 *
 * @param world 世界引用
 * @param pos 方块位置（保留参数，未来 EnvironmentAttributes 系统使用）
 * @return TriState：主世界返回 True/False，下界/末地返回 Default
 */
[[nodiscard]] inline util::TriState getEyeblossomOpen(const IWorld& world, const BlockPos& pos) noexcept
{
    MC_UNUSED(pos);

    const DimensionId dimId = world.dimension();
    const DimensionType dimType = DimensionType::fromId(dimId);

    // 下界和末地有固定时间，且其 Timeline 不包含 EYEBLOSSOM_OPEN 轨道
    // 因此这两个维度的 EYEBLOSSOM_OPEN 始终为维度默认值 TriState::Default
    if (dimType.hasFixedTime()) {
        return util::TriState::Default;
    }

    // 主世界：按 dayTimeOfDay 判断
    const i64 tod = world.dayTimeOfDay();
    if (tod >= EYEBLOSSOM_OPEN_TICK && tod < EYEBLOSSOM_CLOSE_TICK) {
        return util::TriState::True;
    }
    return util::TriState::False;
}

} // namespace eyeblossom_environment

} // namespace blocks
} // namespace mc
