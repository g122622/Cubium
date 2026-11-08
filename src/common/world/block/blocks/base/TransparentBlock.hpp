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

#include "common/world/block/blocks/base/SimpleBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 透明方块（原版 `TransparentBlock`）
 *
 * 玻璃、染色玻璃、铜格栅、遮光玻璃的公共基类。
 *
 * 这类方块有两个与普通方块相反的视觉属性：
 *
 * - **遮光亮度恒为 1.0**：它们的碰撞形状虽然是整格，但光能透过去，
 *   不该在相邻面投下阴影。按基类默认判据（碰撞形状是否覆盖整格）会得到 0.2，
 *   导致玻璃周围出现一圈不该有的暗边。
 * - **天空光直通**：天空光可以从它们上方打到下方，和空气一样。
 *
 * 注意本类只描述「光」的行为，不涉及 `noOcclusion`（那是面剔除与 AO 的开关，
 * 由方块属性单独设置）。
 */
class TransparentBlock : public SimpleBlock {
public:
    explicit TransparentBlock(BlockProperties properties);

    /**
     * @brief 遮光亮度恒为 1.0
     *
     * 碰撞形状是整格却不该投影，这是透明方块与实心方块的核心区别。
     */
    [[nodiscard]] f32 getShadeBrightness(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    /**
     * @brief 天空光直通
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;
};

} // namespace blocks
} // namespace mc
