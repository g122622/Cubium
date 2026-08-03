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
 */

#pragma once

#include "../agricultural/BushBlock.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 萤火虫灌木方块
 *
 * 装饰性发光植物（亮度 2），自然生成在水边的草方块/菌丝体上，沼泽中任意位置。
 * 继承 BushBlock 走默认 canSurvive（下方须为 #dirt 标签或耕地），不重写 canSustain，
 * 使 SimpleBlockFeature 的 canSurvive 终判生效，避免在世界生成时浮空于水面。
 *
 * MC ID: minecraft:firefly_bush
 *
 * 参考: net.minecraft.world.level.block.FireflyBushBlock
 */
class FireflyBushBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FireflyBushBlock(const BlockProperties& properties);

    ~FireflyBushBlock() noexcept override = default;
};

} // namespace blocks
} // namespace mc
