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

#include "AbstractPressurePlateBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 测重压力板方块
 *
 * 测重压力板根据检测到的物品数量输出不同的信号强度。
 * - 轻质测重压力板：信号强度 = min(物品数量, 15)
 * - 重质测重压力板：信号强度 = min(物品数量 / 10, 15)
 */
class WeightedPressurePlateBlock : public AbstractPressurePlateBlock {
public:
    /**
     * @brief 测重压力板类型
     */
    enum class Sensitivity : u8 {
        Light = 0, ///< 轻质（每物品+1信号强度）
        Heavy = 1  ///< 重质（每10物品+1信号强度）
    };

    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param sensitivity 灵敏度类型
     */
    WeightedPressurePlateBlock(const BlockProperties& properties, Sensitivity sensitivity);

protected:
    [[nodiscard]] i32 calculateSignalStrength(IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] i32 getTickDelay(i32 oldSignal, i32 newSignal) const override;

    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override;

private:
    Sensitivity m_sensitivity;

    /**
     * @brief 获取压力板上的实体数量
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 实体数量
     */
    [[nodiscard]] i32 _getEntityCount(IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
