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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, either express or implied,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "OminousItemSpawnerEntity.hpp"

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/potion/LingeringPotionItem.hpp"
#include "common/item/items/potion/SplashPotionItem.hpp"
#include "common/item/items/potion/ThrowablePotionItem.hpp"
#include "common/item/items/trial/WindChargeItem.hpp"
#include "common/item/items/weapon/ThrowableItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

namespace mc {
namespace entity {

using namespace entity::serialization;

// ============================================================================
// 构造函数
// ============================================================================

OminousItemSpawnerEntity::OminousItemSpawnerEntity(EntityId id)
    : Entity(id)
{
    // MC Java: this.noPhysics = true
    // 实体穿透方块，不受碰撞影响
    setNoClip(true);

    // 生成延迟将在 create() 或外部设置时随机指定
    // 此处先设为最大值，确保在未设置物品时不会立即生成
    m_spawnItemAfterTicks = SPAWN_ITEM_DELAY_MAX;
}

std::unique_ptr<Entity> OminousItemSpawnerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<OminousItemSpawnerEntity>(EntityId(0));
}

// ============================================================================
// 生命周期
// ============================================================================

void OminousItemSpawnerEntity::tick()
{
    Entity::tick();

    // MC Java 将 tick 逻辑分为服务端和客户端
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
    // 对应 MC Java: tickCount == spawnItemAfterTicks - 36
    if (!m_warnedSoundPlayed &&
        static_cast<i64>(m_ticksExisted) >= m_spawnItemAfterTicks - TICKS_BEFORE_ABOUT_TO_SPAWN_SOUND) {
        m_warnedSoundPlayed = true;
        if (m_world != nullptr) {
            m_world->playSound(
                SoundEvents::TRIAL_SPAWNER_ABOUT_TO_SPAWN_ITEM, sound::SoundCategory::Neutral, position(), 1.0f, 1.0f);
        }
    }

    // 到达生成时间，投掷物品并移除自身
    // 对应 MC Java: tickCount >= spawnItemAfterTicks
    if (static_cast<i64>(m_ticksExisted) >= m_spawnItemAfterTicks) {
        spawnItem();
        remove();
    }
}

void OminousItemSpawnerEntity::tickClient()
{
    // MC Java: 每 5 个游戏 tick 生成不祥粒子
    // 对应 MC Java: level.getGameTime() % 5L == 0L
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

    // MC Java: if (itemstack.getItem() instanceof ProjectileItem projectileitem)
    // 判断物品是否为弹射物类型，如果是则向下发射弹射物
    // 在 C++ 中，ThrowableItem 和 WindChargeItem 是弹射物物品
    // ThrowableItem 包括: SnowballItem, EggItem, EnderPearlItem, ExperienceBottleItem, ThrowablePotionItem
    // WindChargeItem 是独立的弹射物物品
    const item::ThrowableItem* throwableItem = dynamic_cast<const item::ThrowableItem*>(item);
    const item::WindChargeItem* windChargeItem = dynamic_cast<const item::WindChargeItem*>(item);

    if (throwableItem != nullptr || windChargeItem != nullptr) {
        // 弹射物物品：向下发射弹射物
        spawnedEntity = spawnProjectile(*m_world, *item);
    } else {
        // 普通物品：创建物品实体自然掉落
        // MC Java: new ItemEntity(serverlevel, this.getX(), this.getY(), this.getZ(), itemstack)
        auto itemEntity = std::make_unique<ItemEntity>(
            EntityId(0), itemStack, static_cast<f32>(x()), static_cast<f32>(y()), static_cast<f32>(z()));

        ItemEntity* rawPtr = itemEntity.get();
        EntityId entityId = m_world->spawnEntity(std::move(itemEntity));
        if (entityId != EntityId(0)) {
            spawnedEntity = rawPtr;
        }
    }

    // MC Java: serverlevel.levelEvent(3021, this.blockPosition(), 1)
    // 播放不祥物品生成器的粒子效果
    BlockPos pos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z()));
    m_world->playEvent(world::WorldEvents::TRIAL_SPAWNER_SPAWN_ITEM, pos, 1);

    // MC Java: serverlevel.gameEvent(entity, GameEvent.ENTITY_PLACE, this.position())
    // 触发游戏事件（用于幽匿感测体检测）
    if (spawnedEntity != nullptr) {
        m_world->gameEvent(gameevent::GameEvents::ENTITY_PLACE, pos, gameevent::GameEvent::Context::of(spawnedEntity));
    }

    // MC Java: this.setItem(ItemStack.EMPTY)
    // 清空存储的物品
    m_item = ItemStack{};
}

Entity* OminousItemSpawnerEntity::spawnProjectile(IWorld& world, const Item& item)
{
    // MC Java 的 OminousItemSpawner.spawnProjectile():
    // 1. 获取 ProjectileItem 的 DispenseConfig（默认 power=1.1, uncertainty=6.0）
    // 2. 方向始终为 DOWN (0, -1, 0)
    // 3. 调用 Projectile.spawnProjectileUsingShoot() 创建并发射弹射物
    // 4. 设置 owner 为 this

    // 确定弹射物实体类型和发射参数
    const item::WindChargeItem* windChargeItem = dynamic_cast<const item::WindChargeItem*>(&item);
    const item::ThrowablePotionItem* throwablePotion = dynamic_cast<const item::ThrowablePotionItem*>(&item);

    // 默认 DispenseConfig: power=1.1, uncertainty=6.0
    // ThrowablePotionItem 覆写: power=1.375, uncertainty=3.0
    // WindChargeItem 覆写: power=1.0, uncertainty=6.6666665
    f32 power = 1.1f;
    f32 uncertainty = 6.0f;

    // 根据物品类型确定弹射物实体类型名称
    std::string entityType;

    if (windChargeItem != nullptr) {
        entityType = EntityTypes::WIND_CHARGE;
        power = 1.0f;
        uncertainty = 6.6666665f;
    } else if (throwablePotion != nullptr) {
        // 药水：power=1.375, uncertainty=3.0
        power = 1.375f;
        uncertainty = 3.0f;
        entityType = EntityTypes::POTION;
    } else {
        // 其他投掷物：根据物品的 ResourceLocation 映射到弹射物实体类型
        const auto& itemId = item.itemLocation();
        if (itemId == ResourceLocation("minecraft:snowball")) {
            entityType = EntityTypes::SNOWBALL;
        } else if (itemId == ResourceLocation("minecraft:egg")) {
            entityType = EntityTypes::EGG;
        } else if (itemId == ResourceLocation("minecraft:ender_pearl")) {
            entityType = EntityTypes::ENDER_PEARL;
        } else if (itemId == ResourceLocation("minecraft:experience_bottle")) {
            entityType = EntityTypes::EXPERIENCE_BOTTLE;
        } else {
            // 其他投掷物默认使用雪球实体
            // TODO: 当 ProjectileItem 接口完善后，可通过接口获取弹射物实体类型
            entityType = EntityTypes::SNOWBALL;
        }
    }

    // 通过 EntityRegistry 创建弹射物实体
    auto& registry = EntityRegistry::instance();
    const EntityType* type = registry.getType(entityType);
    if (type == nullptr || !type->canSummon()) {
        // 无法创建弹射物，回退到创建物品实体
        return nullptr;
    }

    std::unique_ptr<Entity> entity = type->create(&world);
    if (entity == nullptr) {
        return nullptr;
    }

    // 设置弹射物位置
    entity->setPosition(x(), y(), z());

    // 将弹射物添加到世界（需在 shoot 之前，因为 shoot 可能访问世界引用）
    ProjectileEntity* projectile = dynamic_cast<ProjectileEntity*>(entity.get());
    EntityId entityId = world.spawnEntity(std::move(entity));
    if (entityId == EntityId(0) || projectile == nullptr) {
        return nullptr;
    }

    // 向下发射弹射物
    // MC Java: direction = Direction.DOWN, 即 (0, -1, 0)
    projectile->shoot(0.0f, -1.0f, 0.0f, power, uncertainty);

    // MC Java: projectile.setOwner(this)
    projectile->setShooter(this);

    return projectile;
}

void OminousItemSpawnerEntity::addParticles()
{
    // MC Java OminousItemSpawner.addParticles():
    // 生成 1-3 个 OMINOUS_SPAWNING 粒子
    // 每个粒子的速度为从实体位置到随机偏移位置的向量
    // 偏移: 0.4 * (random.nextGaussian() - random.nextGaussian()) 各轴
    if (m_world == nullptr) {
        return;
    }

    math::Random& rng = m_world->getRandom();
    i32 count = rng.nextInt(1, 3);

    for (i32 i = 0; i < count; ++i) {
        // MC Java: Vec3 vec31 = new Vec3(
        //     this.getX() + 0.4 * (this.random.nextGaussian() - this.random.nextGaussian()),
        //     this.getY() + 0.4 * (this.random.nextGaussian() - this.random.nextGaussian()),
        //     this.getZ() + 0.4 * (this.random.nextGaussian() - this.random.nextGaussian()));
        f64 offsetX = 0.4 * (rng.nextGaussian() - rng.nextGaussian());
        f64 offsetY = 0.4 * (rng.nextGaussian() - rng.nextGaussian());
        f64 offsetZ = 0.4 * (rng.nextGaussian() - rng.nextGaussian());

        // 粒子速度 = 从实体位置到偏移位置的向量
        f64 velX = offsetX;
        f64 velY = offsetY;
        f64 velZ = offsetZ;

        // MC Java: ParticleTypes.OMINOUS_SPAWNING
        m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::OminousSpawning,
            Vector3(x(), y(), z()),
            Vector3(velX, velY, velZ));
    }
}

} // namespace entity
} // namespace mc
