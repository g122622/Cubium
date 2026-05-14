#include "ProjectileHelper.hpp"

#include "../../../util/math/MathUtils.hpp"
#include "../../../world/IWorld.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace mc {
namespace entity {

namespace {

struct SegmentAabbHit {
    f32 t = 0.0f;
    Vector3 position;
};

[[nodiscard]] std::optional<SegmentAabbHit> intersectSegmentAabb(
    const Vector3& start, const Vector3& end, const AxisAlignedBB& box)
{
    constexpr f32 EPSILON = 1.0e-7f;

    const Vector3 delta = end - start;
    f32 tMin = 0.0f;
    f32 tMax = 1.0f;

    const auto updateAxis = [&](f32 origin, f32 axisDelta, f32 axisMin, f32 axisMax) -> bool {
        if (std::abs(axisDelta) < EPSILON) {
            return origin >= axisMin && origin <= axisMax;
        }

        f32 t1 = (axisMin - origin) / axisDelta;
        f32 t2 = (axisMax - origin) / axisDelta;
        if (t1 > t2) {
            std::swap(t1, t2);
        }

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    if (!updateAxis(start.x, delta.x, box.minX, box.maxX)) {
        return std::nullopt;
    }
    if (!updateAxis(start.y, delta.y, box.minY, box.maxY)) {
        return std::nullopt;
    }
    if (!updateAxis(start.z, delta.z, box.minZ, box.maxZ)) {
        return std::nullopt;
    }

    if (tMax < 0.0f || tMin > 1.0f) {
        return std::nullopt;
    }

    const f32 hitT = std::clamp(tMin, 0.0f, 1.0f);
    return SegmentAabbHit{hitT, Vector3(start.x + delta.x * hitT, start.y + delta.y * hitT, start.z + delta.z * hitT)};
}

} // namespace

void ProjectileHelper::rotateTowardsMovement(Entity& projectile, f32 rotationSpeed)
{
    const Vector3 velocity = projectile.velocity();
    if (velocity.lengthSquared() <= 1.0e-6f) {
        return;
    }

    const f32 horizontal = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    f32 currentYaw = projectile.yaw();
    f32 currentPitch = projectile.pitch();

    const f32 targetYaw = std::atan2(velocity.z, velocity.x) * math::RAD_TO_DEG + 90.0f;
    const f32 targetPitch = std::atan2(horizontal, velocity.y) * math::RAD_TO_DEG - 90.0f;

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

    projectile.setRotation(currentYaw + (targetYaw - currentYaw) * rotationSpeed,
        currentPitch + (targetPitch - currentPitch) * rotationSpeed);
}

AxisAlignedBB ProjectileHelper::createMovementSearchBox(const Entity& projectile, const Vector3& movement, f32 margin)
{
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

    for (mc::Entity* candidate : world.getEntitiesInAABB(searchBox, &projectile)) {
        if (candidate == nullptr || !filter(*candidate)) {
            continue;
        }

        const AxisAlignedBB candidateBox =
            candidate->boundingBox().grow(candidate->getCollisionBorderSize() + collisionExpansion);

        if (candidateBox.contains(start)) {
            nearestEntity = candidate;
            nearestHitPosition = start;
            nearestDistanceSq = 0.0f;
            continue;
        }

        const auto hit = intersectSegmentAabb(start, end, candidateBox);
        if (!hit.has_value()) {
            continue;
        }

        const f32 distanceSq = start.distanceSquared(hit->position);
        if (distanceSq < nearestDistanceSq) {
            nearestEntity = candidate;
            nearestHitPosition = hit->position;
            nearestDistanceSq = distanceSq;
        }
    }

    if (nearestEntity == nullptr) {
        return RayTraceResult::miss();
    }

    return RayTraceResult::entity(nearestHitPosition, nearestEntity);
}

} // namespace entity
} // namespace mc
