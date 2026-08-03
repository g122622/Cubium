/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "AbstractRailBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 普通铁轨方块
 *
 * 普通铁轨用于矿车行驶：
 * - 自动连接到相邻铁轨（包括上下Y层级）
 * - 支持弯轨和斜轨
 * - 三连接时支持红石道岔切换（T型道岔）
 * - 无碰撞箱
 *
 * 与动力铁轨/探测铁轨/激活铁轨的区别：
 * - isStraight() 返回 false，支持弯轨形状
 * - 三连接时红石信号可以切换弯轨方向
 *
 * 参考: net.minecraft.block.RailBlock
 */
class RailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 铁轨状态更新 ==========

    /**
     * @brief 铁轨状态更新
     *
     * 普通铁轨在三连接情况下，当邻居信号源变化时会重新计算方向，
     * 实现红石道岔切换功能。
     *
     * 参考: net.minecraft.block.RailBlock.updateState
     */
    void updateState(IWorld& world, const BlockPos& pos, const BlockState& state, Block& neighborBlock) override;

    // ========== 属性访问 ==========

    /**
     * @brief 获取铁轨形状
     */
    [[nodiscard]] RailShape getRailShape(const BlockState& state) const override;

    /**
     * @brief 设置铁轨形状
     */
    [[nodiscard]] BlockState withRailShape(const BlockState& state, RailShape shape) const override;

    /**
     * @brief 检查状态是否有铁轨形状属性
     */
    [[nodiscard]] bool hasRailShapeProperty(const BlockState& state) const override
    {
        return state.hasProperty(SHAPE());
    }

    /**
     * @brief 获取形状属性
     */
    static const EnumProperty<RailShape>& SHAPE()
    {
        static auto prop = RailShapeProperty::create("shape");
        return *prop;
    }
};

} // namespace blocks
} // namespace mc
