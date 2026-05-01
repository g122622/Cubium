#include "TridentEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity) {
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

TridentEntity::TridentEntity(LegacyEntityType type, EntityId id)
    : AbstractArrowEntity(type, id)
{
    m_damage = 8.0f;  // 三叉戟伤害更高
    setPickupStatus(PickupStatus::Allowed);
}

std::unique_ptr<Entity> TridentEntity::create(IWorld* /*world*/) {
    return std::make_unique<TridentEntity>(LegacyEntityType::Unknown, 0);
}

void TridentEntity::tick() {
    // 参考 MC 1.16.5 TridentEntity.tick() 第60-93行
    // 检查是否应该开始返回
    if (m_timeInGround > 4) {
        m_dealtDamage = true;
    }

    // 检查忠诚附魔状态
    Entity* shooter = getShooter();
    if ((m_dealtDamage || isInGround()) && shooter != nullptr) {
        const i32 loyaltyLevel = m_loyaltyLevel;

        if (loyaltyLevel > 0 && !shouldReturnToThrower()) {
            // 忠诚附魔但无法返回（射手是观察者模式），掉落物品
            if (!m_world->isRemote() && pickupStatus() == PickupStatus::Allowed) {
                // entityDropItem(getArrowStack(), 0.1F);
            }
            remove();
        } else if (loyaltyLevel > 0) {
            // 开始返回
            setNoClip(true);
            tickReturning();
            return;
        }
    }

    // 调用父类tick
    AbstractArrowEntity::tick();
}

bool TridentEntity::shouldReturnToThrower() {
    // 参考 MC 1.16.5 TridentEntity.shouldReturnToThrower() 第95-102行
    Entity* shooter = getShooter();
    if (shooter != nullptr && shooter->isAlive()) {
        // 如果是玩家，检查是否在观察者模式
        // Player* player = dynamic_cast<Player*>(shooter);
        // if (player && player->isSpectator()) {
        //     return false;
        // }
        return true;
    }
    return false;
}

void TridentEntity::tickReturning() {
    // 参考 MC 1.16.5 TridentEntity.tick() 第76-88行
    Entity* shooter = getShooter();
    if (!shooter || !shooter->isAlive()) {
        // 射手已死亡或不存在，移除三叉戟
        remove();
        return;
    }

    // 计算到射手的方向
    Vector3 direction(
        shooter->x() - m_position.x,
        shooter->y() + shooter->eyeHeight() * 0.5 - m_position.y,
        shooter->z() - m_position.z
    );

    // 更新旋转朝向运动方向
    ProjectileHelper::rotateTowardsMovement(*this, 0.2f);

    // 参考 MC 1.16.5 第77行：Y轴微小偏移
    m_position.y += direction.y * 0.015 * static_cast<f32>(m_loyaltyLevel);

    // 计算距离
    f32 distance = direction.length();

    // 参考 MC 1.16.5 第82-83行：返回速度
    // double d0 = 0.05D * (double)i;
    f32 speed = 0.05f * static_cast<f32>(m_loyaltyLevel);

    // 设置速度：当前速度缩放 0.95 后加上朝向射手的方向
    Vector3 currentVel = m_velocity;
    direction = direction.normalized();
    m_velocity = Vector3(
        currentVel.x * 0.95 + direction.x * speed,
        currentVel.y * 0.95 + direction.y * speed,
        currentVel.z * 0.95 + direction.z * speed
    );

    // 更新位置
    m_prevPosition = m_position;
    m_position = m_position + m_velocity;

    // 检查是否到达射手
    if (distance < 2.0f) {
        // 到达射手，添加到背包
        Player* player = dynamic_cast<Player*>(shooter);
        if (player) {
            onPlayerPickup(*player);
        } else {
            // 非玩家射手，直接移除
            remove();
        }
        return;
    }

    // 播放返回音效（首次）
    if (m_returningTicks == 0) {
        // playSound(SoundEvents.ITEM_TRIDENT_RETURN, 10.0F, 1.0F);
    }
    ++m_returningTicks;

    // 检查是否在水中
    if (isInWater()) {
        for (int i = 0; i < 4; ++i) {
            // TODO: 生成气泡粒子
        }
    }

    Entity::tick();
}

void TridentEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 参考 MC 1.16.5 TridentEntity.onEntityHit() 第124-167行
    Entity* target = result.hitEntity;

    // 计算基础伤害
    f32 damage = 8.0f;

    // TODO: 根据目标类型增加伤害（亡灵生物额外伤害）
    // if (target instanceof LivingEntity) {
    //     damage += EnchantmentHelper.getModifierForCreature(m_tridentStack, ((LivingEntity)target).getCreatureAttribute());
    // }

    // 获取射击者
    Entity* shooter = getShooter();

    // 创建伤害来源
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Trident, shooter, this, shooter != nullptr);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Trident, this, this, false);
    }

    // 标记已造成伤害
    m_dealtDamage = true;

    // TODO: 应用伤害
    // bool hurt = target->hurt(*damageSource, damage);

    // 击退效果
    if (m_knockbackStrength > 0) {
        f32 ratio = 0.6f * static_cast<f32>(m_knockbackStrength);
        Vector3 horizontalVel(m_velocity.x, 0.0f, m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // TODO: 雷击附魔
    // 参考 MC 1.16.5 第154-164行
    // if (world instanceof ServerWorld && world.isThundering() && EnchantmentHelper.hasChanneling(m_tridentStack)) {
    //     BlockPos blockpos = target.getPosition();
    //     if (world.canSeeSky(blockpos)) {
    //         LightningBoltEntity lightning = EntityType.LIGHTNING_BOLT.create(world);
    //         lightning.moveForced(Vector3d.copyCenteredHorizontally(blockpos));
    //         lightning.setCaster(shooter instanceof ServerPlayerEntity ? (ServerPlayerEntity)shooter : null);
    //         world.addEntity(lightning);
    //         playSound(SoundEvents.ITEM_TRIDENT_THUNDER, 5.0F, 1.0F);
    //     }
    // }

    // 速度反转为轻微反弹
    m_velocity = Vector3(
        m_velocity.x * -0.01,
        m_velocity.y * -0.1,
        m_velocity.z * -0.01
    );

    // 三叉戟不移除，而是等待返回
    // 如果没有忠诚附魔，会进入 m_inGround 状态
}

void TridentEntity::onBlockHit(const RayTraceResult& result) {
    // 参考 MC 1.16.5 三叉戟命中方块的行为
    m_inGround = true;
    m_hitBlock = true;
    m_hitBlockPos = result.blockPos;

    // 保存方块状态
    if (m_world && result.type == RayTraceResultType::Block) {
        m_inBlockState = m_world->getBlockState(
            result.blockPos.x, result.blockPos.y, result.blockPos.z);
    }

    // 清除暴击和穿透状态
    m_critical = false;
    m_pierceLevel = 0;
    clearPiercedEntities();

    // 三叉戟有特殊的命中地面音效
    // playSound(SoundEvents.ITEM_TRIDENT_HIT_GROUND, 1.0F, 1.0F);
}

f32 TridentEntity::getWaterDrag() const {
    // 参考 MC 1.16.5 TridentEntity.getWaterDrag() 第213-215行
    // 三叉戟在水中阻力很小
    return 0.99f;
}

void TridentEntity::setEnchantmentEffectsFrom(LivingEntity& shooter, f32 baseVelocity) {
    // 参考 MC 1.16.5 AbstractArrowEntity.setEnchantmentEffectsFromEntity()
    math::Random rng = createRandomFromEntity(*this);
    m_damage = static_cast<f32>(baseVelocity * 2.0 + rng.nextGaussian() * 0.25 +
               static_cast<f64>(m_world ? m_world->getDifficulty().getId() * 0.11 : 0.0));

    // TODO: 从三叉戟物品获取附魔
    // int power = EnchantmentHelper.getMaxEnchantmentLevel(Enchantments.POWER, shooter);
    // int punch = EnchantmentHelper.getMaxEnchantmentLevel(Enchantments.PUNCH, shooter);
    // int flame = EnchantmentHelper.getMaxEnchantmentLevel(Enchantments.FLAME, shooter);

    (void)shooter;
}

void TridentEntity::setItemStack(const ItemStack& stack) {
    m_tridentStack = stack;
    // 参考 MC 1.16.5 TridentEntity 构造函数第42-43行
    // 从物品堆获取忠诚附魔等级
    m_loyaltyLevel = static_cast<u8>(
        mc::item::enchant::EnchantmentHelper::getEnchantmentLevel(stack, "minecraft:loyalty"));
}

bool TridentEntity::onPlayerPickup(Player& player) {
    // 参考 MC 1.16.5 AbstractArrowEntity.onCollideWithPlayer()
    // 只有当三叉戟在地上或返回时才能被拾取
    if (!m_inGround && !noClip()) {
        return false;
    }

    if (m_arrowShake > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = (pickupStatus() == PickupStatus::Allowed) ||
                     (pickupStatus() == PickupStatus::CreativeOnly && player.isCreative()) ||
                     (noClip() && getShooter() != nullptr &&
                      getShooter()->uuid() == player.uuid());

    if (canPickup) {
        // TODO: 添加到玩家背包
        // player.inventory().addItem(m_tridentStack);
        // player.onItemPickup(this, 1);
        remove();
        return true;
    }

    return false;
}

void TridentEntity::tickInGroundTrident() {
    // 三叉戟特殊的地面tick逻辑
    // 参考 MC 1.16.5 TridentEntity.func_225516_i_() 第205-209行
    // 如果不允许拾取或没有忠诚附魔，则使用普通超时逻辑
    if (pickupStatus() != PickupStatus::Allowed || m_loyaltyLevel <= 0) {
        AbstractArrowEntity::tickInGround();
    }
    // 否则不超时，等待返回
}

} // namespace entity
} // namespace mc
