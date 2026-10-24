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

#include "common/util/TriState.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/attribute/DimensionAttributes.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/timeline/Timelines.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 眼眸花环境属性查询工具
 *
 * 按 EYEBLOSSOM_OPEN 环境属性查询当前方块应处的开合状态：
 * - 主世界：夜晚时间线切换为 True，白天切换为 False
 * - 下界/末地：维度未声明日夜时间线，保持默认值 Default（不主动切换，眼眸花保持当前状态）
 *
 * 查询走维度级环境属性系统（常量层 + 时间线层），与网络/渲染侧使用同一份属性定义。
 * 群系位置层尚未接入，因此当前查询不依赖方块位置。
 */
namespace eyeblossom_environment {

// 不在头文件中 using namespace：以别名限定环境属性子系统
namespace env_attribute = ::mc::world::attribute;

/**
 * @brief 查询当前位置的 EYEBLOSSOM_OPEN 环境属性
 *
 * @param world 世界引用
 * @param pos 方块位置（群系位置层接入后用于空间查询）
 * @return TriState：主世界返回 True/False，下界/末地返回 Default
 */
[[nodiscard]] inline util::TriState getEyeblossomOpen(const IWorld& world, const BlockPos& pos)
{
    // 维度级系统按注册名解析时间线与属性表，故先确保两者已装配（幂等）
    ::mc::world::timeline::Timelines::bootstrap();

    const DimensionId dimId = world.dimension();
    const DimensionType dimType = DimensionType::fromId(dimId);

    const i64 dayTime = world.dayTimeOfDay();
    const i64 gameTime = world.dayTime();

    // 维度级环境属性系统：层序与 vanilla 一致（常量层 → 时间线层）
    const std::unique_ptr<env_attribute::EnvironmentAttributeSystem> system =
        env_attribute::DimensionAttributes::createSystem(
            dimType,
            [dayTime]() {
                return dayTime;
            },
            [gameTime]() {
                return gameTime;
            });

    return system->getValue(env_attribute::EnvironmentAttributes::EYEBLOSSOM_OPEN(),
        Vector3(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z)));
}

} // namespace eyeblossom_environment

} // namespace blocks
} // namespace mc
