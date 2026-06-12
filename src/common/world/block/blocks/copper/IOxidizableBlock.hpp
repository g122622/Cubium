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

#include "util/property/Properties.hpp"

namespace mc {

class BlockState;
class Block;

namespace blocks {

/**
 * @brief 可氧化方块接口
 *
 * 所有具有氧化等级的方块都实现此接口，用于统一识别和计算氧化概率。
 * 曼哈顿距离氧化算法需要识别周围方块是否为同类型可氧化方块，
 * 并获取其氧化等级来计算氧化概率。
 */
class IOxidizableBlock {
public:
    virtual ~IOxidizableBlock() = default;

    /**
     * @brief 获取当前氧化等级
     */
    [[nodiscard]] virtual BlockStateProperties::OxidationLevel getOxidationLevel() const = 0;

    /**
     * @brief 获取下一氧化等级对应的方块
     *
     * 返回 nullptr 表示已是最高等级，无法继续氧化。
     */
    [[nodiscard]] virtual Block* getNextOxidationBlock() const = 0;

    /**
     * @brief 获取氧化概率修正系数
     *
     * 未氧化（Unaffected）返回 0.75，其余等级返回 1.0。
     * 这影响氧化速度——未氧化的铜氧化得更慢。
     */
    [[nodiscard]] virtual float getOxidationChanceModifier() const
    {
        return getOxidationLevel() == BlockStateProperties::OxidationLevel::Unaffected ? 0.75f : 1.0f;
    }
};

} // namespace blocks
} // namespace mc
