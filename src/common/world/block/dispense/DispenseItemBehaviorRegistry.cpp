#include "DispenseItemBehaviorRegistry.hpp"
#include "IDispenseItemBehavior.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/potion/PotionUtils.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/entities/projectile/OtherProjectiles.hpp"
#include "../../../entity/core/Entity.hpp"

namespace mc {
namespace blocks {

DispenseItemBehaviorRegistry::DispenseItemBehaviorRegistry()
    : m_defaultBehavior(std::make_unique<DefaultDispenseItemBehavior>()) {
}

DispenseItemBehaviorRegistry& DispenseItemBehaviorRegistry::instance() {
    static DispenseItemBehaviorRegistry instance;
    return instance;
}

void DispenseItemBehaviorRegistry::registerBehavior(const std::string& itemId, std::unique_ptr<IDispenseItemBehavior> behavior) {
    m_behaviors[itemId] = std::move(behavior);
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const ItemStack& stack) const {
    if (stack.isEmpty()) {
        return nullptr;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return nullptr;
    }
    return getBehavior(item->itemLocation().toString());
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const std::string& itemId) const {
    auto it = m_behaviors.find(itemId);
    if (it != m_behaviors.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool DispenseItemBehaviorRegistry::hasBehavior(const std::string& itemId) const {
    return m_behaviors.find(itemId) != m_behaviors.end();
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getDefaultBehavior() {
    return m_defaultBehavior.get();
}

void DispenseItemBehaviorRegistry::initDefaultBehaviors() {
    // ========================================================================
    // 投掷物发射行为
    // 参考: MC 1.16.5 DispenserBlock.static block()
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
        1.1f, 6.0f
    );

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
        1.1f, 6.0f
    );

    // 药水箭: velocity=1.1, inaccuracy=6.0
    // 参考 MC 1.16.5: AbstractArrowEntity.setPotionEffect(itemStack)
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
        1.1f, 6.0f
    );

    // --- 投掷物品 ---
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
        1.1f, 6.0f
    );

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
        1.1f, 6.0f
    );

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
        1.1f, 6.0f
    );

    // 附魔之瓶: velocity=1.1, inaccuracy=3.0 (更精确)
    // MC 1.16.5: inaccuracy * 0.5 = 3.0
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
        1.1f, 3.0f
    );

    // 喷溅药水: velocity=1.1, inaccuracy=6.0
    // 参考 MC 1.16.5: PotionEntity.setItemStack() 在 onImpact() 中读取效果
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
        1.1f, 6.0f
    );

    // 滞留药水: velocity=1.1, inaccuracy=6.0
    // 参考 MC 1.16.5: PotionEntity.setItemStack() 在 onImpact() 中读取效果
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
        1.1f, 6.0f
    );

    // ========================================================================
    // TODO: 以下发射行为需要额外的系统支持
    // ========================================================================

    // --- 火焰弹 ---
    // 需要实现 SmallFireballEntity 并检测目标位置是否可点燃
    // registerBehavior("minecraft:fire_charge", ...);

    // --- 烟花火箭 ---
    // 需要实现 FireworkRocketEntity 并读取烟花数据
    // registerBehavior("minecraft:firework_rocket", ...);

    // --- 刷怪蛋 ---
    // 需要实体生成系统和实体类型注册表
    // for each spawn egg: registerBehavior(..., SpawnEggDispenseBehavior(...));

    // --- 船 ---
    // 需要 BoatEntity 并检测目标位置是否有水
    // registerBehavior("minecraft:oak_boat", new DispenseBoatBehavior(BoatEntity::Type::OAK));
    // ... 其他船类型

    // --- 桶 ---
    // 需要 FluidState 和流体放置逻辑
    // registerBehavior("minecraft:water_bucket", new BucketDispenseBehavior(...));
    // registerBehavior("minecraft:lava_bucket", new BucketDispenseBehavior(...));

    // --- 打火石 ---
    // 需要 OptionalDispenseBehavior 和火焰方块放置逻辑
    // registerBehavior("minecraft:flint_and_steel", new FlintAndSteelDispenseBehavior());

    // --- 骨粉 ---
    // 需要 BonemealEvent 和作物催熟逻辑
    // registerBehavior("minecraft:bone_meal", new BonemealDispenseBehavior());

    // --- TNT ---
    // 需要 TNTEntity 和点燃逻辑
    // registerBehavior("minecraft:tnt", new TNTDispenseBehavior());

    // --- 潜影盒 ---
    // 需要 OptionalDispenseBehavior 和潜影盒放置逻辑
    // registerBehavior("minecraft:shulker_box", new ShulkerBoxDispenseBehavior());

    // --- 玻璃瓶 ---
    // 需要流体检测和药水瓶填充逻辑
    // registerBehavior("minecraft:glass_bottle", new GlassBottleDispenseBehavior());

    // --- 萤石/重生锚 ---
    // 需要重生锚充能逻辑
    // registerBehavior("minecraft:glowstone", new GlowstoneDispenseBehavior());

    // --- 剪刀 ---
    // 需要蜂巢采集逻辑
    // registerBehavior("minecraft:shears", new ShearsDispenseBehavior());
}

} // namespace blocks
} // namespace mc
