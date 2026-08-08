/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR USE OF
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "OminousItemSpawnerEntity.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace entity {

using namespace entity::serialization;

// ============================================================================
// 构造函数
// ============================================================================

OminousItemSpawnerEntity::OminousItemSpawnerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    // 实体穿透方块，不受碰撞影响
    setNoClip(true);

    // 生成延迟将在 create() 或外部设置时随机指定
    // 此处先设为最大值，确保在未设置物品时不会立即生成
    m_spawnItemAfterTicks = SPAWN_ITEM_DELAY_MAX;
}

std::unique_ptr<Entity> OminousItemSpawnerEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<OminousItemSpawnerEntity>(EntityInstanceId(0), registry);
}

std::unique_ptr<Entity> OminousItemSpawnerEntity::createWithItem(IWorld& world, const ItemStack& stack)
{
    // 创建实体、设置随机延迟 [60, 120] ticks、设置物品
    // 从 world 获取 ECS 注册表（服务端必非空；客户端不接入 ECS，调用方不应在客户端用此工厂）
    auto* registryPtr = world.entityRegistry();
    MC_ASSERT_RELEASE(registryPtr != nullptr);
    auto entity = std::make_unique<OminousItemSpawnerEntity>(EntityInstanceId(0), *registryPtr);
    entity->m_spawnItemAfterTicks = world.getRandom().nextInt(SPAWN_ITEM_DELAY_MIN, SPAWN_ITEM_DELAY_MAX);
    entity->m_item = stack;
    return entity;
}

// ============================================================================
// 生命周期
// ============================================================================

void OminousItemSpawnerEntity::tick()
{
    Entity::tick();

    // tick 逻辑分为服务端和客户端
    if (m_world != nullptr) {
        if (m_world->isClientSide()) {
            tickClient();
        } else {
            tickServer();
        }
    }
}

void OminousItemSpawnerEntity::tickServer()
{
    // 生成前 36 ticks 播放警告音效
    if (!m_warnedSoundPlayed &&
        static_cast<i64>(m_ticksExisted) >= m_spawnItemAfterTicks - TICKS_BEFORE_ABOUT_TO_SPAWN_SOUND) {
        m_warnedSoundPlayed = true;
        if (m_world != nullptr) {
            m_world->playSound(
                SoundEvents::TRIAL_SPAWNER_ABOUT_TO_SPAWN_ITEM, sound::SoundCategory::Neutral, position(), 1.0f, 1.0f);
        }
    }

    // 到达生成时间，投掷物品并移除自身
    if (static_cast<i64>(m_ticksExisted) >= m_spawnItemAfterTicks) {
        spawnItem();
        remove();
    }
}

void OminousItemSpawnerEntity::tickClient()
{
    // 每 5 个游戏 tick 生成不祥粒子
    if (m_world != nullptr && m_world->getGameTime() % 5 == 0) {
        addParticles();
    }
}

// ============================================================================
// 物品存取
// ============================================================================

void OminousItemSpawnerEntity::setItem(const ItemStack& stack)
{
    m_item = stack;
}

// ============================================================================
// 序列化
// ============================================================================

void OminousItemSpawnerEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    Entity::addAdditionalSaveData(tag);

    // 存储物品（仅当非空时）
    if (!m_item.isEmpty()) {
        nbt::tags::compound_tag itemTag;
        m_item.toNbt(itemTag);
        tag.value.emplace("item", std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
    }

    // 存储生成时间（i64）
    tag.put("spawn_item_after_ticks", static_cast<i64>(m_spawnItemAfterTicks));
}

Result<void> OminousItemSpawnerEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(Entity::readAdditionalSaveData(tag));

    // 读取物品
    const nbt::tags::compound_tag* itemTag = nbt_helper::tryGetCompound(tag, "item");
    if (itemTag != nullptr) {
        auto itemResult = ItemStack::fromNbt(*itemTag);
        if (itemResult.success()) {
            m_item = std::move(itemResult.value());
        }
    }

    // 读取生成时间
    if (auto val = nbt_helper::tryGetLong(tag, "spawn_item_after_ticks")) {
        m_spawnItemAfterTicks = *val;
    }

    return Result<void>::ok();
}

// ============================================================================
// 物品生成
// ============================================================================

void OminousItemSpawnerEntity::spawnItem()
{
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    const ItemStack& itemStack = getItem();
    if (itemStack.isEmpty()) {
        return;
    }

    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return;
    }

    Entity* spawnedEntity = nullptr;

    // 通过 ProjectileItem 接口判断物品是否为弹射物类型，替代硬编码映射。
    // 实现了 ProjectileItem 的物品包括：ThrowableItem（雪球/鸡蛋/末影珍珠/经验瓶/药水）、
    // WindChargeItem（风弹）等。
    const item::ProjectileItem* projectileItem = dynamic_cast<const item::ProjectileItem*>(item);

    if (projectileItem != nullptr) {
        // 弹射物物品：通过 ProjectileItem 接口向下发射弹射物
        spawnedEntity = spawnProjectile(*m_world, *projectileItem, itemStack);
    } else {
        // 普通物品：创建物品实体自然掉落
        // ECS 迁移：实体构造需要 registry 句柄（m_world 为成员所属世界，调用路径已确保非空）
        auto* registry = m_world->entityRegistry();
        if (registry == nullptr) {
            return;
        }
        auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(0),
            itemStack,
            static_cast<f32>(x()),
            static_cast<f32>(y()),
            static_cast<f32>(z()),
            *registry);

        // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
        itemEntity->setTypeId(EntityTypeKeys::ITEM);

        ItemEntity* rawPtr = itemEntity.get();
        EntityInstanceId entityId = m_world->spawnEntity(std::move(itemEntity));
        if (entityId != EntityInstanceId(0)) {
            spawnedEntity = rawPtr;
        }
    }

    // 播放不祥物品生成器的粒子效果
    BlockPos pos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z()));
    m_world->playEvent(world::WorldEvents::TRIAL_SPAWNER_SPAWN_ITEM, pos, 1);

    // 触发游戏事件（用于幽匿感测体检测）
    if (spawnedEntity != nullptr) {
        m_world->gameEvent(gameevent::GameEvents::ENTITY_PLACE, pos, gameevent::GameEvent::Context::of(spawnedEntity));
    }

    // 清空存储的物品
    m_item = ItemStack{};
}

Entity* OminousItemSpawnerEntity::spawnProjectile(
    IWorld& world, const item::ProjectileItem& projectileItem, const ItemStack& itemStack)
{
    // 通过 ProjectileItem 接口创建弹射物：
    // 1. 从 ProjectileItem 获取发射配置
    // 2. 方向始终为 DOWN (0, -1, 0)
    // 3. 通过 asProjectile 创建弹射物
    // 4. 添加到世界并设定射击参数
    // 5. 设置 owner 为 OminousItemSpawner 自身

    auto config = projectileItem.getDispenseConfig();

    // 方向：向下 (Direction.DOWN)
    constexpr f32 dirX = 0.0f;
    constexpr f32 dirY = -1.0f;
    constexpr f32 dirZ = 0.0f;

    // 通过 ProjectileItem 接口创建弹射物实体
    auto entity = projectileItem.asProjectile(world, Vector3(x(), y(), z()), itemStack, dirX, dirY, dirZ);
    if (entity == nullptr) {
        return nullptr;
    }

    // 将弹射物添加到世界（需在 shoot 之前，因为 shoot 可能访问世界引用）
    entity::ProjectileEntity* projectile = dynamic_cast<entity::ProjectileEntity*>(entity.get());
    EntityInstanceId entityId = world.spawnEntity(std::move(entity));
    if (entityId == EntityInstanceId(0) || projectile == nullptr) {
        return nullptr;
    }

    // 设置发射者为自身
    projectile->setShooter(this);

    // 通过 ProjectileItem 的 shoot 方法发射弹射物
    // 注意：WindChargeItem 的 shoot() 为空操作（风弹在 asProjectile 中已设置初速度）
    projectileItem.shoot(*projectile, dirX, dirY, dirZ, config.power, config.uncertainty);

    return projectile;
}

void OminousItemSpawnerEntity::addParticles()
{
    // 生成 1-3 个 OMINOUS_SPAWNING 粒子
    // 每个粒子的速度为从实体位置到随机偏移位置的向量
    // 偏移: 0.4 * (random.nextGaussian() - random.nextGaussian()) 各轴
    if (m_world == nullptr) {
        return;
    }

    math::Random& rng = m_world->getRandom();
    i32 count = rng.nextInt(1, 3);

    for (i32 i = 0; i < count; ++i) {
        f64 offsetX = 0.4 * (rng.nextGaussian() - rng.nextGaussian());
        f64 offsetY = 0.4 * (rng.nextGaussian() - rng.nextGaussian());
        f64 offsetZ = 0.4 * (rng.nextGaussian() - rng.nextGaussian());

        // 粒子速度 = 从实体位置到偏移位置的向量
        f64 velX = offsetX;
        f64 velY = offsetY;
        f64 velZ = offsetZ;

        m_world->addParticle(
            particle::ParticleTypeId::OminousSpawning, Vector3(x(), y(), z()), Vector3(velX, velY, velZ));
    }
}

} // namespace entity
} // namespace mc
