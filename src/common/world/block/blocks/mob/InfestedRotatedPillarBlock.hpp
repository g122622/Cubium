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

#include "../../Block.hpp"
#include "../RotatedPillarBlock.hpp"
#include "InfestedBlock.hpp"

namespace mc {

class BlockItemUseContext;

namespace blocks {

/**
 * @brief 虫蚀旋转柱状方块（infested_deepslate）
 *
 * 在 InfestedBlock 基础上增加 AXIS（X/Y/Z）属性，可绕 Y 轴旋转。
 * 被破坏时同样生成蠹虫（继承 InfestedBlock.spawnAfterBreak）。
 *
 * 状态属性：
 * - AXIS：柱状轴向（默认 Y）
 *
 * MC ID: minecraft:infested_deepslate
 *
 * 参考: net.minecraft.world.level.block.InfestedRotatedPillarBlock
 */
class InfestedRotatedPillarBlock : public InfestedBlock {
public:
    /**
     * @brief 构造函数
     * @param hostBlock 被感染的方块 ID
     * @param properties 方块属性
     */
    InfestedRotatedPillarBlock(u32 hostBlock, const BlockProperties& properties);

    ~InfestedRotatedPillarBlock() override = default;

    /**
     * @brief 旋转方块状态（90 度时 X/Z 轴互换）
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 获取放置状态（按放置面轴向设置 AXIS）
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;
};

} // namespace blocks
} // namespace mc
