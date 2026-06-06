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

#include "../vegetation/FlowerBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 眼 blossom 花朵方块
 *
 * 苍白花园中的特殊花朵，有两种状态：开放和关闭。
 * 开放状态发光等级为1。
 *
 * MC ID: minecraft:open_eyeblossom, minecraft:closed_eyeblossom
 *
 * 参考: net.minecraft.block.EyeblossomBlock
 */
class EyeblossomBlock : public FlowerBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param isOpen 是否为开放状态
     * @param suspiciousStewEffect 可疑炖汤效果（可选）
     * @param effectDuration 效果持续时间（秒）
     */
    EyeblossomBlock(
        const BlockProperties& properties, bool isOpen, u32 suspiciousStewEffect = 0, i32 effectDuration = 0);

    ~EyeblossomBlock() noexcept override = default;

    // ========== 光照 ==========

    /**
     * @brief 获取光照等级
     *
     * 开放状态返回1，关闭状态返回0。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

private:
    bool m_isOpen;
};

} // namespace blocks
} // namespace mc
