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

#include "common/util/property/Properties.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {

namespace blocks {

/**
 * @brief 茎类果实方块（南瓜、西瓜）
 *
 * 由茎类作物生成的果实方块。
 *
 */
class StemGrownBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit StemGrownBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    /**
     * @brief 析构函数
     */
    ~StemGrownBlock() override = default;

    /**
     * @brief 获取对应的普通茎方块
     */
    [[nodiscard]] virtual const Block* getStem() const = 0;

    /**
     * @brief 获取对应的连接茎方块
     */
    [[nodiscard]] virtual const Block* getAttachedStem() const = 0;
};

} // namespace blocks
} // namespace mc
