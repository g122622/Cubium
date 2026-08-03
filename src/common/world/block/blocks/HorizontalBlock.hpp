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
#include "common/util/property/DirectionProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 水平方向方块基类
 *
 * 只支持4个水平方向（北南东西）的方块基类。
 * 提供旋转和镜像的默认实现。
 * 常用于门、床、活塞、熔炉等方块。
 *
 * 参考: net.minecraft.block.HorizontalBlock
 */
class HorizontalBlock : public Block {
public:
    /**
     * @brief 获取 HORIZONTAL_FACING 属性
     */
    static const DirectionProperty& FACING() { return BlockStateProperties::HORIZONTAL_FACING(); }

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit HorizontalBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~HorizontalBlock() override = default;

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家的水平朝向设置 FACING 属性。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

protected:
    /**
     * @brief 获取水平朝向
     * @param state 方块状态
     * @return 朝向（仅水平方向）
     */
    [[nodiscard]] Direction getFacing(const BlockState& state) const;

    /**
     * @brief 设置水平朝向
     * @param state 方块状态
     * @param facing 朝向（仅水平方向）
     * @return 新状态
     */
    [[nodiscard]] const BlockState& withFacing(const BlockState& state, Direction facing) const;
};

} // namespace blocks
} // namespace mc
