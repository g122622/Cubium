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

/**
 * @file FluidProperties.hpp
 * @brief 流体专用属性定义
 *
 * 参考: net.minecraft.state.properties.BlockStateProperties中的流体属性
 *
 * 流体属性不同于方块属性：
 * - LEVEL_1_8: 流体等级(1-8)，用于FlowingFluid
 * - FALLING: 是否下落，用于流体从上方落下
 */

#include "BooleanProperty.hpp"
#include "IntegerProperty.hpp"

namespace mc::fluid {

/**
 * @brief 流体属性集合
 *
 * 提供流体状态专用的属性。
 * 所有属性都是懒加载的单例。
 */
class FluidProperties {
public:
    /**
     * @brief 流体等级属性 (1-8)
     *
     * 用于FlowingFluid的状态：
     * - 1 = 最远的流动（即将消失）
     * - 8 = 源头方块
     *
     * 注意：这与方块的LEVEL_0_15属性不同！
     * 方块LEVEL_0_15与流体LEVEL_1_8的映射关系：
     * - 方块level=0 -> 流体level=8（源头）
     * - 方块level=1-7 -> 流体level=1-7
     * - 方块level=8-15 -> 流体level=8 + falling=true
     */
    static const IntegerProperty& LEVEL_1_8()
    {
        static auto prop = IntegerProperty::create("level", 1, 8);
        return *prop;
    }

    /**
     * @brief 流体是否下落
     *
     * 当流体从上方落下时设置为true。
     * 下落的流体总是level=8，但falling=true。
     */
    static const BooleanProperty& FALLING()
    {
        static auto prop = BooleanProperty::create("falling");
        return *prop;
    }

private:
    FluidProperties() = delete;
    FluidProperties(const FluidProperties&) = delete;
    FluidProperties& operator=(const FluidProperties&) = delete;
};

} // namespace mc::fluid
