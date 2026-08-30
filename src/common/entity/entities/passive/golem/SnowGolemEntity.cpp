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

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/golem/GolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

SnowGolemEntity::SnowGolemEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : GolemEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

// ============================================================================
// 静态工厂方法
// ============================================================================

std::unique_ptr<Entity> SnowGolemEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SnowGolemEntity>(0, registry);
}

// ============================================================================
// IShearable 接口实现
// ============================================================================

std::vector<ItemStack> SnowGolemEntity::shear(Player* /*player*/)
{
    std::vector<ItemStack> drops;

    if (m_hasPumpkin) {
        m_hasPumpkin = false;

        // 播放剪刀音效
        if (world() != nullptr) {
            world()->playSound(SoundEvents::ENTITY_SNOW_GOLEM_SHEAR,
                sound::SoundCategory::Neutral,
                m_builtIn.stateVector->m_pos,
                1.0f,
                1.0f);
        }

        // 掉落雕刻南瓜
        // 通过 BlockItemRegistry 获取 CARVED_PUMPKIN 方块对应的物品
        if (VanillaBlocks::CARVED_PUMPKIN != nullptr) {
            const BlockItem* carvedPumpkinItem =
                BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CARVED_PUMPKIN);
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
    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    // 通过区块获取生物群系
    const ChunkData* chunk = worldPtr->getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return false;
    }

    BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());
    const Biome& biome = BiomeRegistry::instance().get(biomeId);

    // MC 1.21: getHeightAdjustedTemperature 使用位置噪声和高度调整
    f32 temperature = biome.getHeightAdjustedTemperature(pos.x, pos.y, pos.z, world::SEA_LEVEL);

    return temperature > MELT_TEMPERATURE;
}

// ============================================================================
// 远程攻击
// ============================================================================

void SnowGolemEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    if (target == nullptr || !target->isAlive() || world() == nullptr) {
        return;
    }

    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = world()->entityRegistry();
    if (registry == nullptr) {
        return;
    }

    // 创建雪球实体
    auto snowballEntity = entity::SnowballEntity::create(world(), *registry);
    entity::SnowballEntity* snowball = static_cast<entity::SnowballEntity*>(snowballEntity.get());
    // 工厂绕过补救：SnowballEntity::create 直接 make_unique 绕过 EntityType::create 工厂，
    // m_typeId 为空串，getEntities({type:"minecraft:snowball"})/canAttackType 等按 typeId 的
    // 查询全部失效。对齐项目内同类补救惯例（BlazeFireballAttackGoal.cpp / CreeperEntity.cpp）。
    snowball->setTypeId(entity::EntityTypeKeys::SNOWBALL);

    // 设置位置（从眼睛高度发射）
    f32 eyeY = static_cast<f32>(y()) + eyeHeight() - 0.1f;
    snowball->setPosition(x(), eyeY, z());

    // 设置发射者
    snowball->setShooter(this);

    // 计算射击方向
    // 目标眼睛高度 - 雪球高度
    f64 targetEyeY = target->y() + target->eyeHeight();
    f64 dx = target->x() - x();
    f64 dy = targetEyeY - eyeY;
    f64 dz = target->z() - z();

    // 添加基于水平距离的垂直偏移
    f32 horizontalDist = static_cast<f32>(std::sqrt(dx * dx + dz * dz));
    f32 verticalOffset = horizontalDist * 0.2f;

    // 发射雪球
    snowball->shoot(static_cast<f32>(dx),
        static_cast<f32>(dy + verticalOffset),
        static_cast<f32>(dz),
        SNOWBALL_VELOCITY,
        SNOWBALL_INACCURACY);

    // 播放投掷音效
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
    return SoundEvents::ENTITY_SNOW_GOLEM_AMBIENT;
}

std::optional<ResourceLocation> SnowGolemEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_SNOW_GOLEM_HURT;
}

std::optional<ResourceLocation> SnowGolemEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_SNOW_GOLEM_DEATH;
}

// ============================================================================
// 生命周期
// ============================================================================

void SnowGolemEntity::tick()
{
    // 调用父类
    GolemEntity::tick();

    // 只在服务端执行以下逻辑
    IWorld* worldPtr = world();
    if (worldPtr == nullptr || worldPtr->isClientSide()) {
        return;
    }

    // 融化逻辑：检查温度并在高温区域造成伤害
    if (willMelt()) {
        m_meltTimer++;
        if (m_meltTimer >= MELT_DAMAGE_INTERVAL) {
            m_meltTimer = 0;

            auto fireDamage = DamageSources::onFire();
            hurt(fireDamage, MELT_DAMAGE);
        }
    } else {
        m_meltTimer = 0;
    }

    // 放置雪层逻辑：检查 mobGriefing 规则和温度，放置雪层
    if (_canPlaceSnow()) {
        _placeSnowLayer();
    }
}

// ============================================================================
// AI 目标注册
// ============================================================================

void SnowGolemEntity::registerGoals()
{
    // 调用父类方法
    GolemEntity::registerGoals();

    // 优先级 1: RangedAttackGoal（雪球攻击）
    // 参数：移动速度 1.25，攻击间隔 20 ticks，攻击半径 10 格
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::RangedAttackGoal>(this, 1.25, 20, 20, 10.0f));

    // 优先级 2: WaterAvoidingRandomWalkingGoal（避水随机行走）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0, 1.0e-5f));

    // 优先级 3: LookAtGoal（看向玩家）
    m_goalSelector.addGoal(
        3, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return dynamic_cast<const Player*>(entity) != nullptr;
        }));

    // 优先级 4: LookRandomlyGoal（随机看向）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器：
    // 优先级 1: NearestAttackableTargetGoal<MobEntity>（攻击敌对生物）
    // 使用筛选器选择 IMob 类型的实体（敌对生物）
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<MobEntity>>(
            this, true, 10, [](const LivingEntity* entity) -> bool {
                // 检查是否是 IMob（敌对生物）
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

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 4.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

// ============================================================================
// 私有方法
// ============================================================================

bool SnowGolemEntity::_canPlaceSnow() const
{
    // 检查是否可以放置雪层：
    // 1. mobGriefing 规则允许
    // 2. 实体存活
    // 3. 不是客户端
    const IWorld* worldPtr = world();
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

void SnowGolemEntity::_placeSnowLayer()
{
    // 在 4 个位置尝试放置雪层

    // 获取雪方块的默认状态
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    if (snowState == nullptr) {
        return;
    }

    IWorld* worldPtr = world();

    for (i32 i = 0; i < 4; ++i) {
        // 计算偏移位置
        i32 offsetX = (i % 2) * 2 - 1;
        i32 offsetZ = (i / 2) * 2 - 1;

        BlockPos pos(static_cast<i32>(std::floor(x() + static_cast<f32>(offsetX) * 0.25f)),
            static_cast<i32>(std::floor(y())),
            static_cast<i32>(std::floor(z() + static_cast<f32>(offsetZ) * 0.25f)));

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

        f32 temperature = biome.getHeightAdjustedTemperature(pos.x, pos.y, pos.z, world::SEA_LEVEL);
        if (temperature >= SNOW_TEMPERATURE) {
            continue;
        }

        // 放置雪层
        // 放置前需检查雪层能否存活
        if (!blocks::SnowBlock::canSurviveAt(*worldPtr, pos)) {
            continue;
        }
        worldPtr->setBlockState(pos.x, pos.y, pos.z, snowState, 3);
    }
}

} // namespace mc
