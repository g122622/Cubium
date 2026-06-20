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

#include "common/world/block/BlockState.hpp"
#include <vector>

namespace mc {

/**
 * @brief 平坦层信息
 *
 * 描述平坦世界中一个层的信息：方块和高度。
 * 层从底部（minY）开始向上堆叠。
 *
 * MC 默认配置：
 * - 1x Bedrock（基岩）
 * - 2x Dirt（泥土）
 * - 1x Grass Block（草方块）
 */
class FlatLayerInfo {
public:
    FlatLayerInfo() = default;

    /**
     * @brief 构造平坦层信息
     * @param height 层高度（方块数）
     * @param blockState 层的方块状态
     */
    FlatLayerInfo(i32 height, const BlockState* blockState)
        : m_height(height)
        , m_blockState(blockState)
    {}

    /** 获取层高度 */
    [[nodiscard]] i32 height() const { return m_height; }

    /** 获取方块状态 */
    [[nodiscard]] const BlockState* blockState() const { return m_blockState; }

    /** 设置层高度 */
    void setHeight(i32 height) { m_height = height; }

    /** 设置方块状态 */
    void setBlockState(const BlockState* state) { m_blockState = state; }

    /**
     * @brief 限制层高度不超过最大值
     * @param maxHeight 最大高度
     * @return 实际使用的高度（可能小于 m_height）
     */
    [[nodiscard]] i32 heightLimited(i32 maxHeight) const { return std::min(m_height, maxHeight); }

private:
    i32 m_height = 0;                         ///< 层高度
    const BlockState* m_blockState = nullptr; ///< 方块状态
};

} // namespace mc
