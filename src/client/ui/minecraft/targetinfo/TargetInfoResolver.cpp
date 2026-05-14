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

#include "TargetInfoResolver.hpp"

#include "client/world/ClientWorld.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "client/world/entity/ClientEntityManager.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

namespace mc::client::ui::minecraft::targetinfo {

namespace {

constexpr f32 ENTITY_SEARCH_MARGIN = 1.0f;
constexpr u32 BLOCK_ACCENT_COLOR = 0xFFB88A4A;
constexpr u32 ENTITY_ACCENT_COLOR = 0xFF58A7FF;
constexpr u32 ITEM_ACCENT_COLOR = 0xFFF0C96C;
constexpr u32 PLAYER_ACCENT_COLOR = 0xFF72D5FF;
constexpr u32 XP_ACCENT_COLOR = 0xFF9EF06C;

struct SegmentAabbHit {
    f32 distanceSq = 0.0f;
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
    const Vector3 hitPosition(start.x + delta.x * hitT, start.y + delta.y * hitT, start.z + delta.z * hitT);

    return SegmentAabbHit{hitPosition.distanceSquared(start), hitPosition};
}

[[nodiscard]] std::optional<SegmentAabbHit> rayTraceEntity(
    const Vector3& start, const Vector3& end, const ClientEntity& entity)
{
    const AxisAlignedBB entityBox = AxisAlignedBB::fromPosition(entity.position(), entity.width(), entity.height())
                                        .grow(ENTITY_SEARCH_MARGIN * 0.1f);

    if (entityBox.contains(start)) {
        return SegmentAabbHit{0.0f, start};
    }

    return intersectSegmentAabb(start, end, entityBox);
}

[[nodiscard]] std::string makeEntityTitle(
    const ClientEntity& entity, const TargetInfoResolver::PlayerNameLookup& playerNameLookup)
{
    const ResourceLocation typeLocation = ResourceLocation::parse(entity.typeId());
    const std::string typePath = typeLocation.path();

    if (typePath == "player") {
        if (playerNameLookup) {
            const std::string playerName = playerNameLookup(entity.id());
            if (!playerName.empty()) {
                return playerName;
            }
        }
    }

    if (typePath == "item" && entity.hasItem()) {
        const ItemStack* stack = entity.itemStack();
        if (stack != nullptr) {
            if (stack->hasCustomName()) {
                return stack->getCustomName();
            }

            if (stack->getItem() != nullptr) {
                std::string title = humanizeResourceLocation(stack->getItem()->itemLocation());
                if (stack->getCount() > 1) {
                    title += " x" + std::to_string(stack->getCount());
                }
                return title;
            }
        }
    }

    if (typePath == "experience_orb") {
        return "Experience Orb";
    }

    return humanizeResourceLocation(typeLocation);
}

[[nodiscard]] std::vector<std::string> makeEntityDetails(const ClientEntity& entity, f32 distance)
{
    std::vector<std::string> details;
    details.reserve(4);

    details.emplace_back("Type: " + entity.typeId());
    details.emplace_back("Entity ID: " + std::to_string(entity.id()));
    details.emplace_back("Distance: " + formatDistance(distance));

    const ResourceLocation typeLocation = ResourceLocation::parse(entity.typeId());
    if (typeLocation.path() == "item" && entity.hasItem()) {
        const ItemStack* stack = entity.itemStack();
        if (stack != nullptr && stack->getItem() != nullptr) {
            details.emplace_back("Item: " + stack->getItem()->itemLocation().toString());
            details.emplace_back("Count: " + std::to_string(stack->getCount()));
        }
    } else if (typeLocation.path() == "experience_orb") {
        details.emplace_back("XP: " + std::to_string(entity.xpValue()));
    }

    return details;
}

[[nodiscard]] TargetInfoSnapshot makeBlockSnapshot(const BlockRaycastResult& blockRaycast, const ClientWorld& world)
{
    const BlockState* state =
        world.getBlockState(blockRaycast.blockPos().x, blockRaycast.blockPos().y, blockRaycast.blockPos().z);

    if (state == nullptr) {
        return TargetInfoSnapshot::none();
    }

    std::vector<std::string> details;
    details.reserve(5);
    details.emplace_back("Type: " + state->blockLocation().toString());
    details.emplace_back("Pos: " + formatBlockPos(blockRaycast.blockPos()));
    details.emplace_back("Face: " + formatDirection(blockRaycast.face()));
    details.emplace_back("Distance: " + formatDistance(blockRaycast.distance()));

    const std::string blockStateKey = state->toModelKey();
    if (!blockStateKey.empty()) {
        details.emplace_back("State: " + blockStateKey);
    }

    return TargetInfoSnapshot(TargetInfoKind::Block,
        humanizeResourceLocation(state->blockLocation()),
        std::move(details),
        BLOCK_ACCENT_COLOR);
}

[[nodiscard]] TargetInfoSnapshot makeEntitySnapshot(
    const ClientEntity& entity, f32 distance, const TargetInfoResolver::PlayerNameLookup& playerNameLookup)
{
    const ResourceLocation typeLocation = ResourceLocation::parse(entity.typeId());
    const std::string typePath = typeLocation.path();

    std::string title = makeEntityTitle(entity, playerNameLookup);
    std::vector<std::string> details = makeEntityDetails(entity, distance);

    u32 accentColor = ENTITY_ACCENT_COLOR;
    if (typePath == "player") {
        accentColor = PLAYER_ACCENT_COLOR;
    } else if (typePath == "item") {
        accentColor = ITEM_ACCENT_COLOR;
    } else if (typePath == "experience_orb") {
        accentColor = XP_ACCENT_COLOR;
    }

    return TargetInfoSnapshot(TargetInfoKind::Entity, std::move(title), std::move(details), accentColor);
}

[[nodiscard]] std::optional<SegmentAabbHit> rayTraceClientEntities(
    const Vector3& start, const Vector3& end, const ClientEntityManager& entityManager, const ClientEntity*& hitEntity)
{
    hitEntity = nullptr;

    AxisAlignedBB searchBox(std::min(start.x, end.x) - ENTITY_SEARCH_MARGIN,
        std::min(start.y, end.y) - ENTITY_SEARCH_MARGIN,
        std::min(start.z, end.z) - ENTITY_SEARCH_MARGIN,
        std::max(start.x, end.x) + ENTITY_SEARCH_MARGIN,
        std::max(start.y, end.y) + ENTITY_SEARCH_MARGIN,
        std::max(start.z, end.z) + ENTITY_SEARCH_MARGIN);

    std::optional<SegmentAabbHit> nearestHit;
    entityManager.forEachEntity([&](const ClientEntity& entity) {
        const AxisAlignedBB entityBox = AxisAlignedBB::fromPosition(entity.position(), entity.width(), entity.height())
                                            .grow(ENTITY_SEARCH_MARGIN * 0.1f);
        if (!searchBox.intersects(entityBox)) {
            return;
        }

        const auto hit = rayTraceEntity(start, end, entity);
        if (!hit.has_value()) {
            return;
        }

        if (!nearestHit.has_value() || hit->distanceSq < nearestHit->distanceSq) {
            nearestHit = hit;
            hitEntity = &entity;
        }
    });

    return nearestHit;
}

} // namespace

TargetInfoSnapshot TargetInfoResolver::resolve(const Vector3& eyePosition,
    const Vector3& forward,
    const ClientWorld& world,
    const ClientEntityManager& entityManager,
    const BlockRaycastResult& blockRaycast,
    f32 reachDistance,
    const PlayerNameLookup& playerNameLookup)
{
    const Vector3 endPosition = eyePosition + forward * reachDistance;

    const ClientEntity* hitEntity = nullptr;
    const auto entityHit = rayTraceClientEntities(eyePosition, endPosition, entityManager, hitEntity);

    if (entityHit.has_value() && hitEntity != nullptr) {
        const f32 entityDistance = std::sqrt(entityHit->distanceSq);
        const f32 blockDistance = blockRaycast.isHit() ? blockRaycast.distance() : reachDistance;
        if (!blockRaycast.isHit() || entityDistance < blockDistance) {
            return makeEntitySnapshot(*hitEntity, entityDistance, playerNameLookup);
        }
    }

    if (blockRaycast.isHit()) {
        return makeBlockSnapshot(blockRaycast, world);
    }

    return TargetInfoSnapshot::none();
}

} // namespace mc::client::ui::minecraft::targetinfo