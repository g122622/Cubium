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
