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

#include "AbstractRailBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 探测铁轨方块
 *
 * 探测铁轨用于检测矿车：
 * - 矿车经过时输出红石信号
 * - 信号强度根据矿车内容物变化
 * - 只支持直轨和斜轨（不支持弯轨）
 */
class DetectorRailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DetectorRailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== Tick ==========

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 执行刻
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 红石 ==========

    /**
     * @brief 获取弱信号
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取强信号
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 比较器信号 ==========

    /**
     * @brief 探测铁轨有比较器信号覆盖（基于矿车内容物）
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override;

    /**
     * @brief 获取比较器信号覆盖值
     *
     * 命令方块矿车优先（返回成功次数），其次是容器矿车（基于填充率），
     * 最后是普通矿车（返回15）。
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

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
     * @brief 是否被激活
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 获取形状属性
     */
    static const EnumProperty<RailShape>& SHAPE()
    {
        static auto prop = RailShapeProperty::create("shape");
        return *prop;
    }

    /**
     * @brief 获取激活属性
     */
    static const BooleanProperty& POWERED() { return BlockStateProperties::POWERED(); }
};

} // namespace blocks
} // namespace mc
