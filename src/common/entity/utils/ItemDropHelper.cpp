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

#include "ItemDropHelper.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../util/math/MathConstants.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../core/Entity.hpp"
#include "../entities/item/ItemEntity.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// 随机速度计算
// ============================================================================

Vector3 ItemDropHelper::getBlockDropVelocity(math::Random& rng)
{
    // 参考 MC 1.16.5 InventoryHelper.spawnItemStack()
    // 速度范围：
    // X: (random - 0.5) * 0.1 + random * 0.2 => [-0.05, 0.25]
    // Y: random * 0.2 => [0, 0.2]
    // Z: (random - 0.5) * 0.1 + random * 0.2 => [-0.05, 0.25]
    f32 vx = (rng.nextFloat() - 0.5f) * 0.1f + rng.nextFloat() * 0.2f;
    f32 vy = rng.nextFloat() * 0.2f;
    f32 vz = (rng.nextFloat() - 0.5f) * 0.1f + rng.nextFloat() * 0.2f;
    return Vector3(vx, vy, vz);
}

Vector3 ItemDropHelper::getSimpleDropVelocity(math::Random& rng)
{
    // 参考 MC 1.16.5 ItemEntity 构造函数
    // 速度范围：
    // X: random * 0.2 - 0.1 => [-0.1, 0.1]
    // Y: 0.2
    // Z: random * 0.2 - 0.1 => [-0.1, 0.1]
    f32 vx = rng.nextFloat() * 0.2f - 0.1f;
    f32 vy = 0.2f;
    f32 vz = rng.nextFloat() * 0.2f - 0.1f;
    return Vector3(vx, vy, vz);
}

Vector3 ItemDropHelper::getPlayerDropVelocity(math::Random& rng, bool dropAround, f32 yaw, f32 pitch)
{
    if (dropAround) {
        // 参考 MC 1.16.5 PlayerEntity.dropItem(dropAround=true)
        // 向四周散射
        f32 f = rng.nextFloat() * 0.5f;
        f32 angle = rng.nextFloat() * math::TWO_PI;
        f32 vx = -std::sin(angle) * f;
        f32 vy = 0.2f;
        f32 vz = std::cos(angle) * f;
        return Vector3(vx, vy, vz);
    } else {
        // 参考 MC 1.16.5 PlayerEntity.dropItem(dropAround=false)
        // 按玩家朝向投掷
        constexpr f32 BASE_SPEED = 0.3f;
        constexpr f32 RANDOM_OFFSET = 0.02f;

        f32 pitchRad = pitch * math::DEG_TO_RAD;
        f32 yawRad = yaw * math::DEG_TO_RAD;

        f32 cosPitch = std::cos(pitchRad);
        f32 sinPitch = std::sin(pitchRad);
        f32 cosYaw = std::cos(yawRad);
        f32 sinYaw = std::sin(yawRad);

        f32 randomAngle = rng.nextFloat() * math::TWO_PI;
        f32 randomOffset = rng.nextFloat() * RANDOM_OFFSET;

        f32 vx = -sinYaw * cosPitch * BASE_SPEED + std::cos(randomAngle) * randomOffset;
        f32 vy = -sinPitch * BASE_SPEED + 0.1f + (rng.nextFloat() - rng.nextFloat()) * 0.1f;
        f32 vz = cosYaw * cosPitch * BASE_SPEED + std::sin(randomAngle) * randomOffset;

        return Vector3(vx, vy, vz);
    }
}

Vector3 ItemDropHelper::getGaussianVelocity(math::Random& rng, f32 baseVelocity, f32 inaccuracy)
{
    // 参考 MC 1.16.5 DispenserBlock 和 ProjectileEntity.shoot()
    constexpr f32 GAUSSIAN_FACTOR = 0.007499999832361937f; // MC 原版常量

    f32 gaussianX = static_cast<f32>(rng.nextGaussian(0.0, GAUSSIAN_FACTOR * inaccuracy));
    f32 gaussianY = static_cast<f32>(rng.nextGaussian(0.0, GAUSSIAN_FACTOR * inaccuracy));
    f32 gaussianZ = static_cast<f32>(rng.nextGaussian(0.0, GAUSSIAN_FACTOR * inaccuracy));

    return Vector3(baseVelocity + gaussianX, 0.1f + gaussianY, baseVelocity + gaussianZ);
}

// ============================================================================
// 物品实体生成
// ============================================================================

ItemEntity* ItemDropHelper::spawnItemEntity(IWorld* world,
    const ItemStack& stack,
    f64 x,
    f64 y,
    f64 z,
    math::Random& rng,
    i32 pickupDelay,
    const std::string& ownerUuid)
{
    if (world == nullptr || stack.isEmpty()) {
        return nullptr;
    }

    // 获取随机速度
    Vector3 velocity = getBlockDropVelocity(rng);

    return spawnItemEntity(world, stack, x, y, z, velocity.x, velocity.y, velocity.z, pickupDelay, ownerUuid);
}

ItemEntity* ItemDropHelper::spawnItemEntity(IWorld* world,
    const ItemStack& stack,
    f64 x,
    f64 y,
    f64 z,
    f32 vx,
    f32 vy,
    f32 vz,
    i32 pickupDelay,
    const std::string& ownerUuid)
{
    if (world == nullptr || stack.isEmpty()) {
        return nullptr;
    }

    // 创建物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityId(0), // ID 将由世界分配
        stack,
        static_cast<f32>(x),
        static_cast<f32>(y),
        static_cast<f32>(z),
        vx,
        vy,
        vz);

    // 设置拾取延迟
    itemEntity->setPickupDelay(pickupDelay);

    // 设置所有者
    if (!ownerUuid.empty()) {
        itemEntity->setOwner(ownerUuid, ownerUuid);
    }

    // 保存原始指针用于返回
    ItemEntity* result = itemEntity.get();

    // 生成到世界
    EntityId entityId = world->spawnEntity(std::move(itemEntity));
    if (entityId == EntityId(0)) {
        return nullptr; // 生成失败
    }

    return result;
}

ItemEntity* ItemDropHelper::spawnItemAtEntity(
    Entity* entity, const ItemStack& stack, f32 offsetY, math::Random& rng, i32 pickupDelay)
{
    if (entity == nullptr || entity->world() == nullptr || stack.isEmpty()) {
        return nullptr;
    }

    return spawnItemEntity(entity->world(), stack, entity->x(), entity->y() + offsetY, entity->z(), rng, pickupDelay);
}

std::vector<EntityId> ItemDropHelper::spawnItemEntities(IWorld* world,
    const BlockPos& pos,
    const std::vector<ItemStack>& drops,
    math::Random& rng,
    const std::string& throwerUuid)
{
    std::vector<EntityId> spawnedEntities;

    if (world == nullptr || drops.empty()) {
        return spawnedEntities;
    }

    // 在方块中心位置生成物品实体
    f64 centerX = static_cast<f64>(pos.x) + 0.5;
    f64 centerY = static_cast<f64>(pos.y) + 0.5;
    f64 centerZ = static_cast<f64>(pos.z) + 0.5;

    for (const auto& stack : drops) {
        if (stack.isEmpty()) {
            continue;
        }

        // 使用随机偏移，模拟 MC 的方块内随机位置
        // 参考 MC 1.16.5 Block.spawnAsEntity()
        f64 offsetX = centerX + (rng.nextFloat() * 0.5 - 0.25);
        f64 offsetY = centerY + (rng.nextFloat() * 0.5);
        f64 offsetZ = centerZ + (rng.nextFloat() * 0.5 - 0.25);

        // 获取随机速度
        Vector3 velocity = getBlockDropVelocity(rng);

        // 创建物品实体
        auto itemEntity = std::make_unique<ItemEntity>(EntityId(0),
            stack,
            static_cast<f32>(offsetX),
            static_cast<f32>(offsetY),
            static_cast<f32>(offsetZ),
            velocity.x,
            velocity.y,
            velocity.z);

        // 设置拾取延迟
        itemEntity->setPickupDelay(DEFAULT_PICKUP_DELAY);

        // 设置投掷者UUID
        if (!throwerUuid.empty()) {
            itemEntity->setOwner(throwerUuid, throwerUuid);
        }

        // 生成到世界
        EntityId entityId = world->spawnEntity(std::move(itemEntity));
        if (entityId != EntityId(0)) {
            spawnedEntities.push_back(entityId);
        }
    }

    return spawnedEntities;
}

} // namespace mc
