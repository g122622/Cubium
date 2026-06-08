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

#include "Template.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 铜灯降级结构处理器
 *
 * 用于试炼密室结构生成，将涂蜡的铜灯随机降级为较低氧化等级的涂蜡铜灯。
 * 这模拟了试炼密室中铜灯自然老化的效果，为内部提供不同等级的照明。
 *
 * 处理规则（根据 MC 1.21 trial_chambers_copper_bulb_degradation 处理器）：
 * - 涂蜡铜灯 (waxed_copper_bulb[lit=true]) → 保持不变或降级为斑驳/锈蚀/氧化
 * - 降级概率与位置有关，使用位置哈希确定降级结果
 */
class CopperBulbDegradationProcessor : public StructureProcessor {
public:
    CopperBulbDegradationProcessor();

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<CopperBulbDegradationProcessor>();
    }

private:
    /**
     * @brief 根据位置哈希确定铜灯的氧化等级
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 氧化等级 (0=未氧化, 1=斑驳, 2=锈蚀, 3=氧化)
     */
    [[nodiscard]] static i32 getOxidationLevel(i32 x, i32 y, i32 z);
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
