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

#include "ProjectileHelper.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>

namespace mc {
namespace entity {

namespace {

// 线段与AABB相交检测结果
struct SegmentAabbHit {
    f32 t = 0.0f;     // 相交点在线段上的参数 (0-1)
    Vector3 position; // 相交点的世界坐标
};

/**
 * @brief 计算线段与轴对齐包围盒的交点
 * @param start 线段起点
 * @param end 线段终点
 * @param box 轴对齐包围盒
 * @return 如果相交，返回交点信息；否则返回空
 */
[[nodiscard]] std::optional<SegmentAabbHit> intersectSegmentAabb(
    const Vector3& start, const Vector3& end, const AxisAlignedBB& box)
{
    constexpr f32 EPSILON = 1.0e-7f;

    const Vector3 delta = end - start;
    f32 tMin = 0.0f;
    f32 tMax = 1.0f;

    // 使用滑动平面算法计算各轴的相交区间
    const auto updateAxis = [&](f32 origin, f32 axisDelta, f32 axisMin, f32 axisMax) -> bool {
        if (std::abs(axisDelta) < EPSILON) {
            // 线段与该轴平行，检查起点是否在包围盒范围内
            return origin >= axisMin && origin <= axisMax;
        }

        // 计算进入和离开包围盒的参数值
        f32 t1 = (axisMin - origin) / axisDelta;
        f32 t2 = (axisMax - origin) / axisDelta;
        if (t1 > t2) {
            std::swap(t1, t2);
        }

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    // 依次检测三个轴
    if (!updateAxis(start.x, delta.x, box.minX, box.maxX)) {
        return std::nullopt;
    }
    if (!updateAxis(start.y, delta.y, box.minY, box.maxY)) {
        return std::nullopt;
    }
    if (!updateAxis(start.z, delta.z, box.minZ, box.maxZ)) {
        return std::nullopt;
    }

    // 检查交点是否在线段范围内
    if (tMax < 0.0f || tMin > 1.0f) {
        return std::nullopt;
    }

    // 计算交点位置
    const f32 hitT = std::clamp(tMin, 0.0f, 1.0f);
    return SegmentAabbHit{hitT, Vector3(start.x + delta.x * hitT, start.y + delta.y * hitT, start.z + delta.z * hitT)};
}

} // namespace

void ProjectileHelper::rotateTowardsMovement(Entity& projectile, f32 rotationSpeed)
{
    // 获取当前速度，如果速度太小则不进行旋转
    const Vector3 velocity = projectile.velocity();
    if (velocity.lengthSquared() <= 1.0e-6f) {
        return;
    }

    // 计算当前朝向和目标朝向
    const f32 horizontal = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    f32 currentYaw = projectile.yaw();
    f32 currentPitch = projectile.pitch();

    // 根据速度方向计算目标偏航角和俯仰角
    const f32 targetYaw = std::atan2(velocity.z, velocity.x) * math::RAD_TO_DEG + 90.0f;
    const f32 targetPitch = std::atan2(horizontal, velocity.y) * math::RAD_TO_DEG - 90.0f;

    // 将角度差归一化到 [-180, 180) 范围，避免绕远路旋转
    while (targetPitch - currentPitch < -180.0f) {
        currentPitch -= 360.0f;
    }
    while (targetPitch - currentPitch >= 180.0f) {
        currentPitch += 360.0f;
    }
    while (targetYaw - currentYaw < -180.0f) {
        currentYaw -= 360.0f;
    }
    while (targetYaw - currentYaw >= 180.0f) {
        currentYaw += 360.0f;
    }

    // 插值旋转，使投掷物平滑地朝向运动方向
    projectile.setRotation(currentYaw + (targetYaw - currentYaw) * rotationSpeed,
        currentPitch + (targetPitch - currentPitch) * rotationSpeed);
}

AxisAlignedBB ProjectileHelper::createMovementSearchBox(const Entity& projectile, const Vector3& movement, f32 margin)
{
    // 获取实体当前包围盒，根据位移方向构建搜索范围
    const AxisAlignedBB box = projectile.boundingBox();
    return AxisAlignedBB(std::min(box.minX, box.minX + movement.x) - margin,
        std::min(box.minY, box.minY + movement.y) - margin,
        std::min(box.minZ, box.minZ + movement.z) - margin,
        std::max(box.maxX, box.maxX + movement.x) + margin,
        std::max(box.maxY, box.maxY + movement.y) + margin,
        std::max(box.maxZ, box.maxZ + movement.z) + margin);
}

RayTraceResult ProjectileHelper::rayTraceEntities(const IWorld& world,
    const Entity& projectile,
    const Vector3& start,
    const Vector3& end,
    const AxisAlignedBB& searchBox,
    const std::function<bool(const Entity&)>& filter,
    f32 collisionExpansion)
{
    mc::Entity* nearestEntity = nullptr;
    Vector3 nearestHitPosition;
    f32 nearestDistanceSq = std::numeric_limits<f32>::max();

    // 遍历搜索范围内的所有实体
    for (mc::Entity* candidate : world.getEntitiesInAABB(searchBox, &projectile)) {
        if (candidate == nullptr || !filter(*candidate)) {
            continue;
        }

        // 扩展实体的碰撞箱以检测"擦边"命中
        const AxisAlignedBB candidateBox =
            candidate->boundingBox().grow(candidate->getCollisionBorderSize() + collisionExpansion);

        // 特殊情况：起点在碰撞箱内部，直接命中
        if (candidateBox.contains(start)) {
            nearestEntity = candidate;
            nearestHitPosition = start;
            nearestDistanceSq = 0.0f;
            continue;
        }

        // 计算线段与碰撞箱的交点
        const auto hit = intersectSegmentAabb(start, end, candidateBox);
        if (!hit.has_value()) {
            continue;
        }

        // 更新最近的命中实体
        const f32 distanceSq = start.distanceSquared(hit->position);
        if (distanceSq < nearestDistanceSq) {
            nearestEntity = candidate;
            nearestHitPosition = hit->position;
            nearestDistanceSq = distanceSq;
        }
    }

    // 未命中任何实体
    if (nearestEntity == nullptr) {
        return RayTraceResult::miss();
    }

    return RayTraceResult::entity(nearestHitPosition, nearestEntity);
}

} // namespace entity

Hand getWeaponHoldingHand(const LivingEntity& entity, const Item* item)
{
    // 先检查主手是否持有指定物品，如果是则返回主手，否则返回副手
    const ItemStack& mainHand = entity.getMainHandItem();
    if (mainHand.getItem() == item) {
        return Hand::MainHand;
    }
    return Hand::OffHand;
}

} // namespace mc
