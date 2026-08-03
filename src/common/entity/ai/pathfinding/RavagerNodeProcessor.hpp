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
#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/PathNodeType.hpp"

namespace mc::entity::ai::pathfinding {

/**
 * @brief 劫掠兽节点处理器
 *
 * 继承自 WalkNodeProcessor，将树叶视为可通行。
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

private:
    /**
     * @brief 检查位置是否为树叶并将其视为开放空间
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return true 如果是树叶，false 否则
     */
    [[nodiscard]] bool _checkLeavesAsOpen(i32 x, i32 y, i32 z) const;
};

} // namespace mc::entity::ai::pathfinding
