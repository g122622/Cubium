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
#include "../../core/Entity.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include <cstddef>

namespace mc::entity::ai::pathfinding {

Vector3d Path::getVectorFromIndex(const Entity* entity, i32 index) const
{
    if (index < 0 || index >= static_cast<i32>(m_points.size())) {
        return Vector3d(0.0, 0.0, 0.0);
    }

    const PathPoint& point = m_points[static_cast<size_t>(index)];

    // 如果实体宽度大于1，需要调整位置到方块中心
    if (entity && entity->width() > 1.0f) {
        // 对于宽实体，将路径点位置调整到方块中心
        // 这确保宽实体能够正确导航通过狭窄空间
        f32 halfWidth = entity->width() / 2.0f;
        return Vector3d(
            static_cast<f64>(point.x()) + 0.5, static_cast<f64>(point.y()), static_cast<f64>(point.z()) + 0.5);
    }

    // 普通实体：返回路径点中心位置
    return Vector3d(static_cast<f64>(point.x()) + 0.5, static_cast<f64>(point.y()), static_cast<f64>(point.z()) + 0.5);
}

Vector3d Path::getPosition(const Entity* entity) const
{
    const PathPoint* currentTarget = getCurrentTarget();
    if (!currentTarget) {
        return Vector3d(0.0, 0.0, 0.0);
    }

    return getVectorFromIndex(entity, m_currentIndex);
}

} // namespace mc::entity::ai::pathfinding
