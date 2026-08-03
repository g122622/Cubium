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
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"

namespace mc {

// Forward declarations
class BlockState;

/**
 * @brief 旋转柱状方块基类
 *
 * 用于原木、柱状玄武岩等可绕Y轴旋转的方块。
 * 拥有 axis 属性（X、Y、Z）。
 */
class RotatedPillarBlock : public Block {
public:
    /**
     * @brief 获取AXIS属性
     */
    static const EnumProperty<Axis>& AXIS();

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RotatedPillarBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~RotatedPillarBlock() override = default;

    /**
     * @brief 获取方块的轴
     * @param state 方块状态
     * @return 轴向（X、Y或Z）
     */
    Axis getAxis(const BlockState& state) const;

    /**
     * @brief 设置方块的轴
     * @param state 方块状态
     * @param axis 目标轴向
     * @return 新状态
     */
    const BlockState& withAxis(const BlockState& state, Axis axis) const;

    /**
     * @brief 旋转方块状态
     *
     * 当结构旋转时，X轴和Z轴会互换。
     *
     * @param state 原状态
     * @param rotation 旋转类型
     * @return 旋转后的状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据放置面的轴向设置初始状态。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;
};

} // namespace mc
