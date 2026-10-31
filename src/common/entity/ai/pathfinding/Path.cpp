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

#include "Path.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

Vector3d Path::entityPosAtNode(f32 width, size_t index) const
{
    const PathPoint& point = m_nodes[index];
    // 半格偏移取 floor(宽度 + 1) * 0.5：宽度 < 1 的实体偏移 0.5（走格子中心），
    // 宽度 >= 1 的实体偏移 1.0（格子中心再右移半格，抵消它们碰撞箱的横向扩展）。
    const f64 centre = static_cast<f64>(static_cast<i32>(std::floor(width + 1.0f))) * 0.5;
    return Vector3d(static_cast<f64>(point.x()) + centre,
        static_cast<f64>(point.y()),
        static_cast<f64>(point.z()) + centre);
}

Vector3d Path::nextEntityPos(f32 width) const
{
    return entityPosAtNode(width, static_cast<size_t>(m_nextNodeIndex));
}

} // namespace mc::entity::ai::pathfinding