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

#include "NodeEvaluator.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

BlockPos PathfindingContext::entityBlockPos() const
{
    return BlockPos(static_cast<i32>(std::floor(m_entityPosition.x)),
        static_cast<i32>(std::floor(m_entityPosition.y)),
        static_cast<i32>(std::floor(m_entityPosition.z)));
}

void NodeEvaluator::prepare(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition)
{
    m_context = std::make_unique<PathfindingContext>(region, entity, entityPosition);
    m_nodes.clear();
    // 实体占用的格子数：floor(尺寸 + 1)。宽度 0.6 的僵尸占 1 格，宽度 1.4 的实体占 2 格。
    m_entityWidth = static_cast<i32>(std::floor(entity.pathfindingWidth() + 1.0f));
    m_entityHeight = static_cast<i32>(std::floor(entity.pathfindingHeight() + 1.0f));
    m_entityDepth = static_cast<i32>(std::floor(entity.pathfindingWidth() + 1.0f));
}

void NodeEvaluator::done()
{
    m_context.reset();
    m_nodes.clear();
}

Target NodeEvaluator::_getTargetNodeAt(f64 x, f64 y, f64 z) const
{
    return Target(static_cast<i32>(std::floor(x)), static_cast<i32>(std::floor(y)), static_cast<i32>(std::floor(z)));
}

} // namespace mc::entity::ai::pathfinding