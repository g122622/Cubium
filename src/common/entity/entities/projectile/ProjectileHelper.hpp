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

#include "ProjectileEntity.hpp"

#include <functional>

namespace mc {
namespace entity {

/**
 * @brief 投掷物公共辅助工具
 */
class ProjectileHelper {
public:
    /**
     * @brief 按运动方向更新实体朝向
     */
    static void rotateTowardsMovement(Entity& projectile, f32 rotationSpeed);

    /**
     * @brief 为当前位移构造实体搜索范围
     */
    [[nodiscard]] static AxisAlignedBB createMovementSearchBox(
        const Entity& projectile, const Vector3& movement, f32 margin = 1.0f);

    /**
     * @brief 在线段上查找最近的实体命中
     */
    [[nodiscard]] static RayTraceResult rayTraceEntities(const IWorld& world,
        const Entity& projectile,
        const Vector3& start,
        const Vector3& end,
        const AxisAlignedBB& searchBox,
        const std::function<bool(const Entity&)>& filter,
        f32 collisionExpansion = 0.3f);
};

} // namespace entity
} // namespace mc
