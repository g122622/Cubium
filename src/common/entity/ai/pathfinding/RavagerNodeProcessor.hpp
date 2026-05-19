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

#include "WalkNodeProcessor.hpp"

namespace mc::entity::ai::pathfinding {

/**
 * @brief 劫掠兽节点处理器
 *
 * 继承自 WalkNodeProcessor，将树叶视为可通行。
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.RavagerEntity.Processor
 *
 * 原版代码：
 * protected PathNodeType func_215744_a(IBlockReader p_215744_1_, boolean p_215744_2_, boolean p_215744_3_, BlockPos
 * p_215744_4_, PathNodeType p_215744_5_) { return p_215744_5_ == PathNodeType.LEAVES ? PathNodeType.OPEN :
 * super.func_215744_a(...);
 * }
 *
 * 这意味着劫掠兽可以将树叶视为开放的空空间，从而穿过树叶。
 */
class RavagerNodeProcessor : public WalkNodeProcessor {
public:
    RavagerNodeProcessor() = default;
    ~RavagerNodeProcessor() override = default;

    /**
     * @brief 获取节点类型
     *
     * 重写以将树叶视为开放空间。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 节点类型（树叶返回 OPEN）
     */
    [[nodiscard]] PathNodeType getNodeType(i32 x, i32 y, i32 z) override;

    /**
     * @brief 获取带实体的节点类型
     *
     * 重写以将树叶视为开放空间。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 节点类型（树叶返回 OPEN）
     */
    [[nodiscard]] PathNodeType getNodeTypeWithEntity(i32 x, i32 y, i32 z) override;
};

} // namespace mc::entity::ai::pathfinding
