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
#include "common/item/core/AdventureModePredicate.hpp"
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
 * @brief 激活铁轨方块
 *
 * 激活铁轨用于激活矿车：
 * - 接收红石信号时激活
 * - 激活时可以：
 *   - 弹出矿车中的实体（运输矿车）
 *   - 切换矿车轨道（TNT矿车、漏斗矿车）
 * - 只支持直轨和斜轨（不支持弯轨）
 */
class ActivatorRailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ActivatorRailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 红石 ==========

    /**
     * @brief 邻居更新
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

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
     *
     * 激活铁轨只支持直轨和斜轨（6 值），不含弯轨，对齐 vanilla。
     */
    static const EnumProperty<RailShape>& SHAPE()
    {
        static auto prop = RailShapeProperty::createStraight("shape");
        return *prop;
    }

    /**
     * @brief 获取激活属性
     */
    static const BooleanProperty& POWERED() { return BlockStateProperties::POWERED(); }
};

} // namespace blocks
} // namespace mc
