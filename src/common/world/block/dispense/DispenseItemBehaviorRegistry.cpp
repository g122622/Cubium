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

#include "DispenseItemBehaviorRegistry.hpp"

#include "IDispenseItemBehavior.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

namespace mc {
namespace blocks {

DispenseItemBehaviorRegistry::DispenseItemBehaviorRegistry()
    : m_defaultBehavior(std::make_unique<DefaultDispenseItemBehavior>())
{}

DispenseItemBehaviorRegistry& DispenseItemBehaviorRegistry::instance()
{
    static DispenseItemBehaviorRegistry instance;
    return instance;
}

void DispenseItemBehaviorRegistry::registerBehavior(
    const std::string& itemId, std::unique_ptr<IDispenseItemBehavior> behavior)
{
    m_behaviors[itemId] = std::move(behavior);
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return nullptr;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return nullptr;
    }
    return getBehavior(item->itemLocation().toString());
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const std::string& itemId) const
{
    auto it = m_behaviors.find(itemId);
    if (it != m_behaviors.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool DispenseItemBehaviorRegistry::hasBehavior(const std::string& itemId) const
{
    return m_behaviors.find(itemId) != m_behaviors.end();
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getDefaultBehavior()
{
    return m_defaultBehavior.get();
}

void DispenseItemBehaviorRegistry::initDefaultBehaviors()
{
    // ========================================================================
    // 投掷物发射行为
    // ========================================================================

    // --- 箭矢 ---
    // 普通箭矢: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:arrow",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::ArrowEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
                // 设置可拾取状态
                auto* arrow = dynamic_cast<entity::ArrowEntity*>(entity.get());
                if (arrow) {
                    arrow->setPickupStatus(entity::PickupStatus::Allowed);
                }
            }
            return entity;
        },
        1.1f,
        6.0f);

    // 光灵箭: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:spectral_arrow",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::SpectralArrowEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
                auto* arrow = dynamic_cast<entity::SpectralArrowEntity*>(entity.get());
                if (arrow) {
                    arrow->setPickupStatus(entity::PickupStatus::Allowed);
                }
            }
            return entity;
        },
        1.1f,
        6.0f);

    // 药水箭: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:tipped_arrow",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            auto entity = entity::ArrowEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
                auto* arrow = dynamic_cast<entity::ArrowEntity*>(entity.get());
                if (arrow) {
                    arrow->setPickupStatus(entity::PickupStatus::Allowed);
                    // 从 ItemStack 读取药水效果并应用到箭矢
                    auto effects = potion::PotionUtils::getEffects(stack);
                    if (!effects.empty()) {
                        arrow->setEffects(effects);
                        arrow->setColor(potion::PotionUtils::getColor(effects));
                    }
                }
            }
            return entity;
        },
        1.1f,
        6.0f);

    // --- 投掷物品 ---
    // TODO(ProjectileItem): 当前 ProjectileDispenseBehavior 使用 lambda 工厂函数注册弹射物发射行为。
    // 当 ProjectileItem 接口完善后，应重构为通过 ProjectileItem::asProjectile() 和
    // ProjectileItem::getDispenseConfig() 自动注册，避免在注册表中维护与 ProjectileItem
    // 接口重复的硬编码映射。新增投掷物物品时需要同步更新此处的注册和 ProjectileItem 接口实现。
    // 雪球: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:snowball",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::SnowballEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
            }
            return entity;
        },
        1.1f,
        6.0f);

    // 鸡蛋: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:egg",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::EggEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
            }
            return entity;
        },
        1.1f,
        6.0f);

    // 末影珍珠: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:ender_pearl",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::EnderPearlEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
            }
            return entity;
        },
        1.1f,
        6.0f);

    // 附魔之瓶: velocity=1.1, inaccuracy=3.0 (更精确)
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:experience_bottle",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::ExperienceBottleEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
            }
            return entity;
        },
        1.1f,
        3.0f);

    // 喷溅药水: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:splash_potion",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            auto entity = entity::PotionEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
                auto* potion = dynamic_cast<entity::PotionEntity*>(entity.get());
                if (potion) {
                    potion->setLingering(false);
                    // 设置 ItemStack 以便 onImpact() 读取药水效果
                    potion->setItemStack(stack);
                }
            }
            return entity;
        },
        1.1f,
        6.0f);

    // 滞留药水: velocity=1.1, inaccuracy=6.0
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:lingering_potion",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            auto entity = entity::PotionEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
                auto* potion = dynamic_cast<entity::PotionEntity*>(entity.get());
                if (potion) {
                    potion->setLingering(true);
                    // 设置 ItemStack 以便 onImpact() 读取药水效果
                    potion->setItemStack(stack);
                }
            }
            return entity;
        },
        1.1f,
        6.0f);

    // ========================================================================
    // 火焰弹发射行为
    // ========================================================================
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:fire_charge",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::SmallFireballEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
            }
            return entity;
        },
        1.0f,  // velocity
        6.0f); // inaccuracy

    // ========================================================================
    // 风弹发射行为
    // ========================================================================
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:wind_charge",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            MC_UNUSED(stack);
            auto entity = entity::WindChargeEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
            }
            return entity;
        },
        1.0f,     // velocity
        6.6667f); // inaccuracy

    // ========================================================================
    // 烟花火箭发射行为
    // ========================================================================
    registerBehavior<ProjectileDispenseBehavior>(
        "minecraft:firework_rocket",
        [](IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
            auto entity = entity::FireworkRocketEntity::create(&world);
            if (entity) {
                entity->setPosition(pos.x, pos.y, pos.z);
                auto* firework = dynamic_cast<entity::FireworkRocketEntity*>(entity.get());
                if (firework && !stack.isEmpty()) {
                    // 设置烟花物品数据（飞行时间、爆炸效果等）
                    firework->setFireworkItem(stack);
                }
            }
            return entity;
        },
        0.5f,  // velocity - 烟花速度较慢
        1.0f); // inaccuracy - 烟花偏差小

    // ========================================================================
    // TNT 发射行为
    // 生成点燃的 TNT 实体；如果 tntExplodes 游戏规则为 false 则不发射
    // ========================================================================
    registerBehavior("minecraft:tnt", std::make_unique<TNTDispenseBehavior>());

    // ========================================================================
    // 船发射行为
    // 需要检测目标位置是否有水
    // ========================================================================

    // 橡木船
    registerBehavior("minecraft:oak_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::OAK));
    // 云杉木船
    registerBehavior("minecraft:spruce_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::SPRUCE));
    // 白桦木船
    registerBehavior("minecraft:birch_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::BIRCH));
    // 丛林木船
    registerBehavior("minecraft:jungle_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::JUNGLE));
    // 金合欢木船
    registerBehavior("minecraft:acacia_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::ACACIA));
    // 深色橡木船
    registerBehavior(
        "minecraft:dark_oak_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::DARK_OAK));
    // 红树木船
    registerBehavior(
        "minecraft:mangrove_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::MANGROVE));
    // 樱花木船
    registerBehavior("minecraft:cherry_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::CHERRY));
    // 苍白橡木船
    registerBehavior(
        "minecraft:pale_oak_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::PALE_OAK));
    // 竹筏
    registerBehavior("minecraft:bamboo_raft", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::BAMBOO));

    // ========================================================================
    // 水桶/岩浆桶发射行为
    // ========================================================================
    // 获取流体实例
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    fluid::Fluid* lavaFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID);

    if (waterFluid != nullptr) {
        registerBehavior("minecraft:water_bucket", std::make_unique<BucketDispenseBehavior>(*waterFluid));
    }
    if (lavaFluid != nullptr) {
        registerBehavior("minecraft:lava_bucket", std::make_unique<BucketDispenseBehavior>(*lavaFluid));
    }

    // ========================================================================
    // 空桶发射行为（收集流体）
    // ========================================================================
    registerBehavior("minecraft:bucket", std::make_unique<EmptyBucketDispenseBehavior>());

    // ========================================================================
    // 打火石发射行为
    // ========================================================================
    registerBehavior("minecraft:flint_and_steel", std::make_unique<FlintAndSteelDispenseBehavior>());

    // ========================================================================
    // 骨粉发射行为
    // ========================================================================
    registerBehavior("minecraft:bone_meal", std::make_unique<BonemealDispenseBehavior>());
}

} // namespace blocks
} // namespace mc
