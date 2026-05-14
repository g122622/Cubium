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
* THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include "SnowGolemEntity.hpp"

#include "sound/SoundCategory.hpp"
#include "sound/SoundEvents.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/biome/Biome.hpp"
#include "world/biome/BiomeRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/gamerule/GameRules.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/utils/ItemDropHelper.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "entity/entities/monster/MonsterEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/ProjectileItemEntity.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

SnowGolemEntity::SnowGolemEntity(LegacyEntityType type, EntityId id)
    : GolemEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

// ============================================================================
// 静态工厂方法
// ============================================================================

std::unique_ptr<Entity> SnowGolemEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SnowGolemEntity>(LegacyEntityType::SnowGolem, EntityId(0));
}

// ============================================================================
// IShearable 接口实现
// ============================================================================

std::vector<ItemStack> SnowGolemEntity::shear(Player* /*player*/)
{
    std::vector<ItemStack> drops;

    if (m_hasPumpkin) {
        m_hasPumpkin = false;

        // MC 1.16.5: 播放剪刀音效
        if (world() != nullptr) {
            world()->playSound(
                SoundEvents::ENTITY_SNOW_GOLEM_SHEAR,
                sound::SoundCategory::Neutral,
                m_position,
                1.0f,
                1.0f
            );
        }

        // MC 1.16.5: 掉落雕刻南瓜
        // 通过 BlockItemRegistry 获取 CARVED_PUMPKIN 方块对应的物品
        if (VanillaBlocks::CARVED_PUMPKIN != nullptr) {
            const BlockItem* carvedPumpkinItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CARVED_PUMPKIN);
            if (carvedPumpkinItem != nullptr) {
                drops.emplace_back(static_cast<const Item*>(carvedPumpkinItem), 1);
            }
        }
    }

    return drops;
}

// ============================================================================
// 融化检查
// ============================================================================

bool SnowGolemEntity::willMelt() const
{
    // MC 1.16.5 SnowGolemEntity.livingTick()
    // 雪傀儡在以下情况下会融化：
    // 1. 在水中
    // 2. 在高温生物群系（温度 > 1.0）

    if (isInWater()) {
        return true;
    }

    // 检查生物群系温度
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return false;
    }

    // 获取当前位置的生物群系
    BlockPos pos(static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    // 通过区块获取生物群系
    const ChunkData* chunk = worldPtr->getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return false;
    }

    BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());
    const Biome& biome = BiomeRegistry::instance().get(biomeId);

    // MC 1.16.5: 温度 > 1.0 时融化
    // 注意：MC 的 getTemperature() 方法还会考虑高度因素
    f32 temperature = biome.getTemperature(pos.y);

    return temperature > MELT_TEMPERATURE;
}

// ============================================================================
// 远程攻击
// ============================================================================

void SnowGolemEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    // MC 1.16.5 SnowGolemEntity.attackEntityWithRangedAttack()
    if (target == nullptr || !target->isAlive() || world() == nullptr) {
        return;
    }

    // 创建雪球实体
    auto snowballEntity = entity::SnowballEntity::create(world());
    entity::SnowballEntity* snowball = static_cast<entity::SnowballEntity*>(snowballEntity.get());

    // 设置位置（从眼睛高度发射）
    f32 eyeY = static_cast<f32>(y()) + eyeHeight() - 0.1f;
    snowball->setPosition(x(), eyeY, z());

    // 设置发射者
    snowball->setShooter(this);

    // 计算射击方向
    // MC 1.16.5: 目标眼睛高度 - 雪球高度
    f64 targetEyeY = target->y() + target->eyeHeight();
    f64 dx = target->x() - x();
    f64 dy = targetEyeY - eyeY;
    f64 dz = target->z() - z();

    // MC 1.16.5: 添加基于水平距离的垂直偏移
    f32 horizontalDist = static_cast<f32>(std::sqrt(dx * dx + dz * dz));
    f32 verticalOffset = horizontalDist * 0.2f;

    // 发射雪球
    // MC 1.16.5: velocity = 1.6, inaccuracy = 12
    snowball->shoot(static_cast<f32>(dx), static_cast<f32>(dy + verticalOffset), static_cast<f32>(dz), SNOWBALL_VELOCITY, SNOWBALL_INACCURACY);

    // 播放投掷音效
    // MC 1.16.5: entity.snow_golem.shoot
    math::Random& random = world()->getRandom();
    playSound(SoundEvents::ENTITY_SNOW_GOLEM_SHOOT, 1.0f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));

    // 生成实体到世界
    world()->spawnEntity(std::move(snowballEntity));
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> SnowGolemEntity::getAmbientSound() const
{
    // MC 1.16.5: entity.snow_golem.ambient
    return SoundEvents::ENTITY_SNOW_GOLEM_AMBIENT;
}

std::optional<ResourceLocation> SnowGolemEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.snow_golem.hurt
    return SoundEvents::ENTITY_SNOW_GOLEM_HURT;
}

std::optional<ResourceLocation> SnowGolemEntity::getDeathSound() const
{
    // MC 1.16.5: entity.snow_golem.death
    return SoundEvents::ENTITY_SNOW_GOLEM_DEATH;
}

// ============================================================================
// 生命周期
// ============================================================================

void SnowGolemEntity::tick()
{
    // 调用父类
    GolemEntity::tick();

    // MC 1.16.5 SnowGolemEntity.livingTick()
    // 只在服务端执行以下逻辑
    IWorld* worldPtr = world();
    if (worldPtr == nullptr || worldPtr->isClientSide()) {
        return;
    }

    // ========== 融化逻辑 ==========
    // MC 1.16.5: 检查温度并在高温区域造成伤害
    if (willMelt()) {
        m_meltTimer++;
        if (m_meltTimer >= MELT_DAMAGE_INTERVAL) {
            m_meltTimer = 0;

            // MC 1.16.5: attackEntityFrom(DamageSource.ON_FIRE, 1.0F)
            auto fireDamage = DamageSources::onFire();
            hurt(fireDamage, MELT_DAMAGE);
        }
    } else {
        m_meltTimer = 0;
    }

    // ========== 放置雪层逻辑 ==========
    // MC 1.16.5: 检查 mobGriefing 规则和温度，放置雪层
    if (canPlaceSnow()) {
        placeSnowLayer();
    }
}

// ============================================================================
// AI 目标注册
// ============================================================================

void SnowGolemEntity::registerGoals()
{
    // 调用父类方法
    GolemEntity::registerGoals();

    // MC 1.16.5 SnowGolemEntity.registerGoals()

    // 优先级 1: RangedAttackGoal（雪球攻击）
    // MC 1.16.5: new RangedAttackGoal(this, 1.25D, 20, 10.0F)
    // 参数：移动速度 1.25，攻击间隔 20 ticks，攻击半径 10 格
    m_goalSelector.addGoal(
        1, std::make_unique<entity::ai::goal::RangedAttackGoal>(this, 1.25, 20, 20, 10.0f));

    // 优先级 2: WaterAvoidingRandomWalkingGoal（避水随机行走）
    // MC 1.16.5: new WaterAvoidingRandomWalkingGoal(this, 1.0D, 1.0000001E-5F)
    m_goalSelector.addGoal(
        2, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0, 1.0e-5f));

    // 优先级 3: LookAtGoal（看向玩家）
    // MC 1.16.5: new LookAtGoal(this, PlayerEntity.class, 6.0F)
    m_goalSelector.addGoal(
        3, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return dynamic_cast<const Player*>(entity) != nullptr;
        }));

    // 优先级 4: LookRandomlyGoal（随机看向）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器：
    // 优先级 1: NearestAttackableTargetGoal<MobEntity>（攻击敌对生物）
    // MC 1.16.5: new NearestAttackableTargetGoal<>(this, MobEntity.class, 10, true, false, (p_213621_0_) -> { return p_213621_0_ instanceof IMob; })
    // 使用筛选器选择 IMob 类型的实体（敌对生物）
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<MobEntity>>(this, true, 10, [](const LivingEntity* entity) -> bool {
            // MC 1.16.5: 检查是否是 IMob（敌对生物）
            // MonsterEntity 继承自 MobEntity 并实现 IMob 语义
            const MonsterEntity* monster = dynamic_cast<const MonsterEntity*>(entity);
            return monster != nullptr;
        }));
}

// ============================================================================
// 属性注册
// ============================================================================

void SnowGolemEntity::registerAttributes()
{
    // 调用父类方法
    GolemEntity::registerAttributes();

    // MC 1.16.5 SnowGolemEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 4.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

// ============================================================================
// 私有方法
// ============================================================================

bool SnowGolemEntity::canPlaceSnow() const
{
    // MC 1.16.5 SnowGolemEntity.livingTick()
    // 检查是否可以放置雪层：
    // 1. mobGriefing 规则允许
    // 2. 实体存活
    // 3. 不是客户端

    // 注意：world() 在 const 方法中返回 const IWorld*
    // isClientSide() 和 getGameRules() 是非 const 方法，需要 const_cast
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (worldPtr == nullptr || worldPtr->isClientSide()) {
        return false;
    }

    // 检查 mobGriefing 规则
    const auto& gameRules = worldPtr->getGameRules();
    if (!gameRules.getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return false;
    }

    return isAlive();
}

void SnowGolemEntity::placeSnowLayer()
{
    // MC 1.16.5 SnowGolemEntity.livingTick()
    // 在 4 个位置尝试放置雪层

    // 获取雪方块的默认状态
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    if (snowState == nullptr) {
        return;
    }

    IWorld* worldPtr = const_cast<IWorld*>(world());
    math::Random& random = worldPtr->getRandom();

    // MC 1.16.5: 在 4 个位置尝试放置
    for (i32 i = 0; i < 4; ++i) {
        // 计算偏移位置
        // MC 1.16.5: (l % 2 * 2 - 1) * 0.25F
        i32 offsetX = (i % 2) * 2 - 1;
        i32 offsetZ = (i / 2) * 2 - 1;

        BlockPos pos(
            static_cast<i32>(std::floor(x() + static_cast<f32>(offsetX) * 0.25f)),
            static_cast<i32>(std::floor(y())),
            static_cast<i32>(std::floor(z() + static_cast<f32>(offsetZ) * 0.25f))
        );

        // 检查位置是否为空气
        const BlockState* currentState = worldPtr->getBlockState(pos.x, pos.y, pos.z);
        if (currentState == nullptr || !currentState->isAir()) {
            continue;
        }

        // 检查生物群系温度
        const ChunkData* chunk = worldPtr->getChunk(pos.chunkX(), pos.chunkZ());
        if (chunk == nullptr) {
            continue;
        }

        BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());
        const Biome& biome = BiomeRegistry::instance().get(biomeId);

        // MC 1.16.5: 温度 < 0.8 时可以放置雪
        f32 temperature = biome.getTemperature(pos.y);
        if (temperature >= SNOW_TEMPERATURE) {
            continue;
        }

        // 检查方块是否可以放置在目标位置
        // MC 1.16.5: blockstate.isValidPosition(this.world, blockpos)
        // 注意：这里简化处理，直接设置雪层
        // 完整实现需要检查 isValidPosition

        // 放置雪层
        worldPtr->setBlockState(pos.x, pos.y, pos.z, snowState, 3);
    }
}

} // namespace mc
