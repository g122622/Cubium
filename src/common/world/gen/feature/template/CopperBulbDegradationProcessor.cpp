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

#include "CopperBulbDegradationProcessor.hpp"

#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

CopperBulbDegradationProcessor::CopperBulbDegradationProcessor() = default;

i32 CopperBulbDegradationProcessor::getOxidationLevel(i32 x, i32 y, i32 z)
{
    // 使用位置哈希来确定氧化等级
    // 模拟 MC 1.21 中 trial_chambers_copper_bulb_degradation 的行为
    u64 hash = math::hashBlockPos(x, y, z);
    // 将哈希值映射到 0-3 的氧化等级
    // 未氧化和斑驳概率较高，锈蚀和氧化概率较低
    return static_cast<i32>((hash >> 16) % 4);
}

std::optional<ProcessedBlockInfo> CopperBulbDegradationProcessor::process(const BlockPos& seedPos,
    const BlockPos& pos,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    MC_UNUSED(seedPos);
    MC_UNUSED(rawBlockInfo);
    MC_UNUSED(settings);

    // TODO(trial_chambers): 实现铜灯降级逻辑
    // 需要查找涂蜡铜灯的方块ID，并根据位置哈希将其替换为对应氧化等级的涂蜡铜灯
    // 当铜块注册系统完善后实现具体映射：
    //   waxed_copper_bulb -> waxed_copper_bulb / waxed_exposed_copper_bulb /
    //                       waxed_weathered_copper_bulb / waxed_oxidized_copper_bulb
    // 当前仅返回原始方块，不做替换

    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
