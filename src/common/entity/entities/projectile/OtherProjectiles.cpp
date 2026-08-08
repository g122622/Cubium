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

#include "OtherProjectiles.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/UuidUtils.hpp"
#include "../../../util/math/MathConstants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/ray/Raycast.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/registry/NaturalBlocks.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../effect/EffectInstance.hpp"
#include "../../effect/EffectType.hpp"
#include "../../entities/item/ItemEntity.hpp"
#include "../../entities/orb/ExperienceOrbEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../inventory/PlayerInventory.hpp"
#include "../../serialization/EntityNbtKeys.hpp"
#include "../../serialization/NbtHelper.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "ProjectileHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/ray/Ray.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace entity {

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 从实体创建随机数生成器
 *
 * 使用实体 ID 和存活时间作为种子。
 */
math::Random createRandomFromEntity(const Entity& entity)
{
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

// ============================================================================
// LlamaSpitEntity 常量
// ============================================================================

namespace {
// 羊驼口水伤害常量
constexpr f32 LLAMA_SPIT_DAMAGE = 1.0f; // 0.5 颗心

// 钓鱼时间常量
constexpr i32 MIN_WAIT_TICKS = 100;     // 最小等待时间 (5秒)
constexpr i32 MAX_WAIT_TICKS = 600;     // 最大等待时间 (30秒)
constexpr i32 LURE_REDUCTION = 100;     // 饵钓每级减少的时间 (5秒)
constexpr i32 MIN_CATCHABLE_TICKS = 20; // 最小捕获窗口 (1秒)
constexpr i32 MAX_CATCHABLE_TICKS = 40; // 最大捕获窗口 (2秒)
} // namespace

LlamaSpitEntity::LlamaSpitEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ThrowableEntity(id, registry)
{}

std::unique_ptr<Entity> LlamaSpitEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<LlamaSpitEntity>(0, registry);
}

void LlamaSpitEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    // 造成 1.0 点伤害（0.5 颗心）
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, this, this, false);
    }

    // 对 LivingEntity 造成伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr && livingTarget->isAlive()) {
        livingTarget->hurt(*damageSource, LLAMA_SPIT_DAMAGE);
    }
}

void LlamaSpitEntity::onImpact(const RayTraceResult& /*result*/)
{
    // 播放命中粒子
    remove();
}

// ============================================================================
// FishingBobberEntity
// ============================================================================

// 静态数据参数定义（对应 MC 1.21.11 FishingHook.DATA_HOOKED_ENTITY / DATA_BITING）
// 在静态初始化阶段通过 EntityDataManager::createKey<T>() 分配全局唯一 ID。
entity::DataParameter<i32> FishingBobberEntity::DATA_HOOKED_ENTITY_PARAM = entity::EntityDataManager::createKey<i32>();
entity::DataParameter<bool> FishingBobberEntity::DATA_BITING_PARAM = entity::EntityDataManager::createKey<bool>();

const EntityClassInfo& FishingBobberEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"FishingBobberEntity", &Entity::classInfo()};
    return s_classInfo;
}

FishingBobberEntity::FishingBobberEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    m_noGravity = false;
    // C++ 虚函数在构造函数中不会派生到子类，因此 Entity 基类构造函数中
    // 调用的 registerData() 只会执行 Entity::registerData()。
    // 子类必须在此显式调用自身的 registerData() 以注册子类专属数据参数。
    registerData();
}

void FishingBobberEntity::registerData()
{
    // 先调用基类注册基础参数（FLAGS/AIR/CUSTOM_NAME 等）
    Entity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册钓鱼浮标专属同步参数
    // 对应 MC 1.21.11 FishingHook.defineSynchedData():
    //   define(DATA_HOOKED_ENTITY, 0)
    //   define(DATA_BITING, false)
    m_dataManager.registerParam(DATA_HOOKED_ENTITY_PARAM, static_cast<i32>(0));
    m_dataManager.registerParam(DATA_BITING_PARAM, false);
}

std::unique_ptr<Entity> FishingBobberEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<FishingBobberEntity>(0, registry);
}

void FishingBobberEntity::setShooter(Entity* shooter)
{
    // 设置钓鱼者（仅支持玩家）
    m_angler = dynamic_cast<Player*>(shooter);
}

void FishingBobberEntity::shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy)
{
    // 设置发射者
    setShooter(&shooter);

    // 计算发射方向
    f32 radPitch = (pitch + pitchOffset) * math::DEG_TO_RAD;
    f32 radYaw = -yaw * math::DEG_TO_RAD;

    f32 x = -std::sin(radYaw) * std::cos(radPitch);
    f32 y = -std::sin(radPitch);
    f32 z = std::cos(radYaw) * std::cos(radPitch);

    // 归一化并乘以速度
    f32 length = std::sqrt(x * x + y * y + z * z);
    if (length > 0.0f) {
        x = x / length * velocity;
        y = y / length * velocity;
        z = z / length * velocity;
    }

    // 添加不准确性
    if (inaccuracy > 0.0f) {
        // 使用世界的随机数生成器添加高斯偏移
        math::Random& random = shooter.world()->getRandom();
        f32 offsetX = random.nextGaussian() * inaccuracy * 0.0075f;
        f32 offsetY = random.nextGaussian() * inaccuracy * 0.0075f;
        f32 offsetZ = random.nextGaussian() * inaccuracy * 0.0075f;
        x += offsetX;
        y += offsetY;
        z += offsetZ;
    }

    // 设置速度
    m_builtIn.velocity->m_velocity.x = x;
    m_builtIn.velocity->m_velocity.y = y;
    m_builtIn.velocity->m_velocity.z = z;
}

void FishingBobberEntity::tick()
{
    Entity::tick();

    m_lifetime++;

    // 检查钓鱼者是否存在
    if (!m_angler || !m_angler->isAlive()) {
        remove();
        return;
    }

    // 检查玩家是否还持有钓鱼竿
    // 如果玩家切换了物品或钓鱼浮标ID已被清除，则移除浮标
    if (!m_angler->isFishing() || m_angler->fishingBobber() != id()) {
        remove();
        return;
    }

    // 更新水面状态
    _updateWaterState();

    switch (m_state) {
        case State::Flying: {
            // 浮标在飞行中，执行射线检测
            if (m_world != nullptr) {
                RayTraceResult hitResult = _performRayTrace();
                if (hitResult.type == RayTraceResultType::Entity) {
                    // 命中实体，钩住它
                    _onEntityHit(hitResult);
                    return;
                }
                if (hitResult.type == RayTraceResultType::Block) {
                    // 命中方块，进入 Bobbing 状态
                    _onBlockHit(hitResult);
                    return;
                }
            }

            // 检测是否入水
            if (isInWater()) {
                m_state = State::Bobbing;
                // 设置初始等待时间
                _setWaitTime();
                // 检测是否在开放水域
                m_inOpenWater = _checkOpenWater();
            }
            break;
        }

        case State::Bobbing:
            // 浮标浮在水面，执行钓鱼逻辑
            if (isInWater()) {
                m_outOfWaterTime = std::max(0, m_outOfWaterTime - 1);
                // 检查开放水域状态（进入水后延迟检查）
                if (m_outOfWaterTime < 10) {
                    m_inOpenWater = m_inOpenWater && _checkOpenWater();
                }
                _catchingFish();
            } else {
                m_outOfWaterTime = std::min(10, m_outOfWaterTime + 1);
            }
            _spawnFishingParticles();
            break;

        case State::Fishing:
            // 咬钩中，等待玩家收杆
            if (m_ticksCatchable > 0) {
                m_ticksCatchable--;
                m_fishAngle += 0.15f; // 鱼游动动画
                // 如果超时未收杆，重置状态
                if (m_ticksCatchable <= 0) {
                    m_state = State::Bobbing;
                    _setWaitTime();
                    // 对应 MC 1.21.11 FishingHook.catchingFish(): nibble 归零时
                    // 设置 DATA_BITING = false，客户端停止咬钩动画。
                    m_dataManager.set(DATA_BITING_PARAM, false);
                }
            } else {
                m_state = State::Bobbing;
                // 防御性：确保 DATA_BITING 已清除（虽然理论上不应进入此分支时仍为 true）
                m_dataManager.set(DATA_BITING_PARAM, false);
            }
            break;

        case State::Hooked:
            // 钩住实体
            if (m_caughtEntity != nullptr) {
                if (m_caughtEntity->isRemoved() || !m_caughtEntity->isAlive()) {
                    // 实体被移除或死亡，恢复飞行状态
                    // 先清除 m_caughtEntity 再调用 _syncCaughtEntityId()，
                    // 以确保客户端同步收到 DATA_HOOKED_ENTITY=0 的更新。
                    m_caughtEntity = nullptr;
                    _syncCaughtEntityId();
                    m_state = State::Flying;
                } else {
                    // 浮标跟随实体位置（设置位置到实体高度的 80% 处）
                    setPosition(m_caughtEntity->x(), static_cast<f32>(m_caughtEntity->getY(0.8)), m_caughtEntity->z());
                }
            } else {
                // 没有被钩住的实体，恢复飞行状态
                m_state = State::Flying;
            }
            break;
    }
}

void FishingBobberEntity::_updateWaterState()
{
    // 通过检查碰撞箱判断是否在水中
    // Entity::isInWater() 已在 tick() 中更新
}

bool FishingBobberEntity::isInWater() const
{
    return Entity::isInWater();
}

bool FishingBobberEntity::_checkOpenWater()
{
    // 对应 MC Java FishingHook.calculateOpenWater
    // 检查浮标位置周围 4 层（Y-1 到 Y+2），每层 5×5 范围
    // 水类型必须从 InsideWater → AboveWater 单调过渡，不能回退或出现 Invalid
    if (m_world == nullptr) {
        return false;
    }

    BlockPos bobberPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));

    WaterType prevType = WaterType::Invalid;

    for (i32 dy = -1; dy <= 2; ++dy) {
        BlockPos from(bobberPos.x - 2, bobberPos.y + dy, bobberPos.z - 2);
        BlockPos to(bobberPos.x + 2, bobberPos.y + dy, bobberPos.z + 2);
        WaterType layerType = _getOpenWaterTypeForArea(from, to);

        switch (layerType) {
            case WaterType::Invalid:
                return false;
            case WaterType::AboveWater:
                // 水上方块不能出现在最底层（前一层还是 Invalid 表示第一层就是 AboveWater）
                if (prevType == WaterType::Invalid) {
                    return false;
                }
                break;
            case WaterType::InsideWater:
                // 水内部不能出现在水上方块之后（不能从水面再回到水下）
                if (prevType == WaterType::AboveWater) {
                    return false;
                }
                break;
        }

        prevType = layerType;
    }

    return true;
}

FishingBobberEntity::WaterType FishingBobberEntity::_getOpenWaterTypeForBlock(const BlockPos& pos) const
{
    // 对应 MC Java FishingHook.getOpenWaterTypeFor
    const BlockState* blockState = m_world->getBlockState(pos);
    if (blockState == nullptr) {
        return WaterType::Invalid;
    }

    // 空气 → AboveWater
    if (blockState->isAir()) {
        return WaterType::AboveWater;
    }

    // 睡莲 → AboveWater
    if (blockState->is(block_registry::NaturalBlocks::LILY_PAD)) {
        return WaterType::AboveWater;
    }

    // 非空气、非睡莲：检查是否为水源方块且碰撞箱为空
    const fluid::FluidState* fluidState = blockState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER()) &&
        fluidState->isSource() && blockState->getCollisionShape().isEmpty()) {
        return WaterType::InsideWater;
    }

    return WaterType::Invalid;
}

FishingBobberEntity::WaterType FishingBobberEntity::_getOpenWaterTypeForArea(
    const BlockPos& from, const BlockPos& to) const
{
    // 对应 MC Java FishingHook.getOpenWaterTypeForArea
    // 区域内所有方块必须为同一 WaterType，否则整个区域为 Invalid
    WaterType result = WaterType::Invalid;
    bool first = true;

    for (i32 x = from.x; x <= to.x; ++x) {
        for (i32 y = from.y; y <= to.y; ++y) {
            for (i32 z = from.z; z <= to.z; ++z) {
                WaterType type = _getOpenWaterTypeForBlock(BlockPos(x, y, z));
                if (first) {
                    result = type;
                    first = false;
                } else if (type != result) {
                    return WaterType::Invalid;
                }
            }
        }
    }

    return result;
}

void FishingBobberEntity::_catchingFish()
{
    // 阶段1：等待咬钩
    if (m_ticksCaughtDelay > 0) {
        m_ticksCaughtDelay--;

        // 接近咬钩时产生水花
        if (m_ticksCaughtDelay < 100 && m_ticksCaughtDelay % 10 == 0 && m_world) {
            // 生成水花粒子
            math::Random rng;
            f32 angle = rng.nextFloat() * 360.0f * math::DEG_TO_RAD;
            f32 radius = rng.nextFloat(25.0f, 60.0f) * 0.1f;
            f32 px = x() + std::sin(angle) * radius;
            f32 py = std::floor(y()) + 1.0f;
            f32 pz = z() + std::cos(angle) * radius;
            m_world->addParticle(particle::ParticleTypeId::Splash,
                Vector3(px, py, pz),
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.1f, 0.0f, 0.1f),
                2 + rng.nextInt(2));
        }
        return;
    }

    // 阶段2：鱼接近浮标
    if (m_ticksCatchableDelay > 0) {
        m_ticksCatchableDelay--;

        // 产生气泡和钓鱼粒子
        if (m_ticksCatchableDelay % 5 == 0 && m_world) {
            math::Random rng;
            f32 angle = m_fishAngle * math::DEG_TO_RAD;
            f32 sinAngle = std::sin(angle);
            f32 cosAngle = std::cos(angle);
            f32 d0 = x() + sinAngle * static_cast<f32>(m_ticksCatchableDelay) * 0.1f;
            f32 d1 = std::floor(y()) + 1.0f;
            f32 d2 = z() + cosAngle * static_cast<f32>(m_ticksCatchableDelay) * 0.1f;

            // 15% 概率生成气泡
            if (rng.nextFloat() < 0.15f) {
                m_world->addParticle(
                    particle::ParticleTypeId::Bubble, Vector3(d0, d1 - 0.1f, d2), Vector3(sinAngle, 0.1f, cosAngle));
            }

            // 钓鱼涟漪粒子
            m_world->addParticle(particle::ParticleTypeId::Fishing,
                Vector3(d0, d1, d2),
                Vector3(cosAngle * 0.04f, 0.01f, -sinAngle * 0.04f));
            m_world->addParticle(particle::ParticleTypeId::Fishing,
                Vector3(d0, d1, d2),
                Vector3(-cosAngle * 0.04f, 0.01f, sinAngle * 0.04f));
        }

        // 鱼接近角度动画
        m_fishAngle += 0.1f;

        // 接近完成，进入可捕获状态
        if (m_ticksCatchableDelay <= 0) {
            m_ticksCatchable = math::Random().nextInt(MIN_CATCHABLE_TICKS, MAX_CATCHABLE_TICKS);
            m_state = State::Fishing;
            // 播放水溅音效
            playSound(SoundEvents::ENTITY_FISHING_BOBBER_SPLASH,
                0.25f,
                1.0f + (math::Random().nextFloat() - math::Random().nextFloat()) * 0.4f);
            // 对应 MC 1.21.11 FishingHook.catchingFish(): 鱼咬钩时设置 DATA_BITING = true
            // 客户端收到后会驱动浮标下沉动画。
            m_dataManager.set(DATA_BITING_PARAM, true);
        }
        return;
    }

    // 阶段0：初始化等待
    if (m_ticksCaughtDelay <= 0 && m_ticksCatchableDelay <= 0 && m_ticksCatchable <= 0) {
        // 设置下一轮等待时间
        _setWaitTime();
    }
}

void FishingBobberEntity::_spawnFishingParticles()
{
    // 浮标在水面时的涟漪效果
    if (isInWater() && m_world) {
        math::Random rng;
        if (rng.nextInt(5) == 0) {
            m_world->addParticle(
                particle::ParticleTypeId::Fishing, m_builtIn.stateVector->m_pos, Vector3(0.0f, 0.01f, 0.0f));
        }
    }
}

void FishingBobberEntity::_setWaitTime()
{
    // 设置咬钩等待时间
    // 基础时间: 100-600 ticks
    // 饵钓附魔: 每级减少 100 ticks
    math::Random rng;
    m_ticksCaughtDelay = rng.nextInt(MIN_WAIT_TICKS, MAX_WAIT_TICKS);
    m_ticksCaughtDelay -= m_speedBonus * LURE_REDUCTION;
    m_ticksCaughtDelay = std::max(20, m_ticksCaughtDelay); // 最小 1 秒

    // 鱼接近时间
    m_ticksCatchableDelay = 0;
    m_ticksCatchable = 0;
}

i32 FishingBobberEntity::_spawnCatchItems()
{
    // 使用钓鱼掉落表生成物品
    if (!m_world || !m_angler) {
        return 0;
    }

    // 获取掉落表管理器
    const loot::LootTable* fishingTable =
        m_world->lootTableManager() ? m_world->lootTableManager()->getTable("minecraft:gameplay/fishing") : nullptr;
    if (!fishingTable) {
        // 如果掉落表不存在，返回默认值
        return 1;
    }

    // 计算幸运值 = 海之眷顾附魔等级 + 玩家基础幸运
    f32 totalLuck = static_cast<f32>(m_luckBonus);
    totalLuck += static_cast<f32>(m_angler->getAttributeValue(entity::attribute::Attributes::LUCK, 0.0));

    // 获取随机数生成器
    math::Random& random = m_world->getRandom();

    // 构建掉落上下文
    auto context = loot::LootContextBuilder(*m_world)
                       .withRandom(random)
                       .withLuck(totalLuck)
                       .withParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(this))
                       .withParameter(loot::LootParams::KILLER_ENTITY, static_cast<Entity*>(m_angler))
                       .withOwnedValue(loot::LootParams::IS_IN_OPEN_WATER, m_inOpenWater)
                       .withLootTableResolver([this](const std::string& id) -> const loot::LootTable* {
                           return m_world->lootTableManager() ? m_world->lootTableManager()->getTable(id) : nullptr;
                       })
                       .withPredicateResolver([this](const std::string& id) -> const loot::LootCondition* {
                           return m_world->lootTableManager() ? m_world->lootTableManager()->getPredicate(id) : nullptr;
                       })
                       .build(loot::LootParameterSets::fishing());

    if (!context) {
        return 1;
    }

    // 生成掉落物品
    std::vector<ItemStack> drops = fishingTable->generate(*context);

    // 计算从浮标到玩家的方向
    f64 dx = m_angler->x() - x();
    f64 dy = m_angler->y() + m_angler->eyeHeight() * 0.5 - y();
    f64 dz = m_angler->z() - z();
    f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    f64 sqrtDistance = std::sqrt(distance);

    // 速度因子（参考 MC 1.16.5）
    f32 vx = static_cast<f32>(dx * 0.1);
    f32 vy = static_cast<f32>(dy * 0.1 + sqrtDistance * 0.08);
    f32 vz = static_cast<f32>(dz * 0.1);

    // 生成物品实体
    for (const auto& drop : drops) {
        if (drop.isEmpty()) {
            continue;
        }

        // 使用 ItemDropHelper 生成物品实体
        ItemDropHelper::spawnItemEntity(m_world,
            drop,
            x(),
            y() + 0.5,
            z(), // 在浮标位置生成
            vx,
            vy,
            vz,              // 朝玩家方向飞
            10,              // 拾取延迟 10 ticks
            m_angler->uuid() // 所有者 UUID，防止立即拾取自己的物品
        );
    }

    // 生成经验球 (1-6 经验)
    i32 experience = random.nextInt(1, 6);
    _spawnExperienceOrbs(experience);

    // 返回钓到物品数量（用于耐久消耗）
    return static_cast<i32>(drops.size());
}

void FishingBobberEntity::_spawnExperienceOrbs(i32 totalXp)
{
    // 生成经验球
    if (totalXp <= 0 || !m_world) {
        return;
    }

    math::Random& random = m_world->getRandom();

    // 分割经验值为多个经验球
    // 经验分割值: 2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3, 1
    static constexpr i32 XP_SPLIT_VALUES[] = {2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3, 1};

    while (totalXp > 0) {
        // 找到适合当前经验值的分割值
        i32 orbXp = 1;
        for (i32 splitValue : XP_SPLIT_VALUES) {
            if (totalXp >= splitValue) {
                orbXp = splitValue;
                break;
            }
        }

        totalXp -= orbXp;

        // 在浮标位置生成经验球，添加随机偏移
        f64 offsetX = random.nextDouble() * 0.2 - 0.1;
        f64 offsetY = random.nextDouble() * 0.2;
        f64 offsetZ = random.nextDouble() * 0.2 - 0.1;

        // ECS 迁移：实体构造需要 registry 句柄（m_world 在调用路径已确保非空）
        auto* registry = m_world->entityRegistry();
        if (registry == nullptr) {
            return;
        }

        auto orb = std::make_unique<ExperienceOrbEntity>(
            m_world, x() + offsetX, y() + 0.5 + offsetY, z() + offsetZ, orbXp, *registry);

        // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
        orb->setTypeId(EntityTypeKeys::EXPERIENCE_ORB);

        // 设置拾取延迟
        orb->setPickupDelay(10);

        // 添加到世界
        m_world->spawnEntity(std::move(orb));
    }
}

i32 FishingBobberEntity::reelIn()
{
    // 收杆
    i32 damage = 0; // 钓鱼竿耐久消耗

    if (m_state == State::Fishing && m_ticksCatchable > 0) {
        // 成功钓到鱼
        damage = _spawnCatchItems();
        remove();
    } else if (m_state == State::Hooked) {
        // 钩住实体，拉过来
        if (m_caughtEntity != nullptr && m_caughtEntity->isAlive()) {
            _bringInHookedEntity();
            // 耐久消耗取决于实体类型：物品实体 3，其他实体 5
            if (dynamic_cast<ItemEntity*>(m_caughtEntity) != nullptr) {
                damage = 3;
            } else {
                damage = 5;
            }
        }
        remove();
    } else {
        // 未咬钩时收杆，无耐久消耗
        remove();
    }

    return damage;
}

// ============================================================================
// FishingBobberEntity - 钩住实体逻辑
// ============================================================================

RayTraceResult FishingBobberEntity::_performRayTrace()
{
    // 使用 ProjectileHelper 进行射线检测
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    // 计算射线起点和终点
    const Vector3 start = m_builtIn.stateVector->m_pos;
    const Vector3 end = m_builtIn.stateVector->m_pos + m_builtIn.velocity->m_velocity;

    // 先检测方块
    const Vector3 delta = end - start;
    if (delta.lengthSquared() <= 1.0e-6f) {
        return RayTraceResult::miss();
    }

    // 创建搜索盒
    const AxisAlignedBB searchBox = ProjectileHelper::createMovementSearchBox(*this, delta, 1.0f);

    // 执行实体射线检测
    RayTraceResult entityResult = ProjectileHelper::rayTraceEntities(
        *m_world,
        *this,
        start,
        end,
        searchBox,
        [this](const Entity& candidate) { return _canHitEntity(candidate); },
        0.3f // collisionExpansion
    );

    if (entityResult.type == RayTraceResultType::Entity) {
        return entityResult;
    }

    // 执行方块射线检测
    const RaycastContext context(Ray(start, delta.normalized()), delta.length());
    const BlockRaycastResult blockResult = raycastBlocks(context, *m_world);
    if (!blockResult.isMiss()) {
        return RayTraceResult::block(blockResult.hitPosition(), blockResult.blockPos());
    }

    return RayTraceResult::miss();
}

bool FishingBobberEntity::_canHitEntity(const Entity& target) const
{
    // 对应 MC Java FishingHook.canHitEntity:
    // super.canHitEntity(target) || target.isAlive() && target instanceof ItemEntity
    // 即：普通弹射物命中逻辑 + 物品实体可被钩住
    //
    // 物品实体的 canBeHitByProjectile() 返回 false（因为 ItemEntity 的
    // canBeCollidedWith() 返回 false，即 MC Java 的 isPickable() 为 false），
    // 但钓鱼浮标需要能钩住水中的物品实体，因此需要特殊处理。

    // 不能命中钓鱼者自己
    if (&target == m_angler) {
        return false;
    }

    // 物品实体可被钩住（绕过 canBeHitByProjectile 检查）
    // 对应 MC Java: target.isAlive() && target instanceof ItemEntity
    if (target.isAlive() && dynamic_cast<const ItemEntity*>(&target) != nullptr) {
        return true;
    }

    // 其他实体使用标准弹射物命中判断（包含 canBeHitByProjectile 检查）
    return target.canBeHitByProjectile();
}

void FishingBobberEntity::_onEntityHit(const RayTraceResult& result)
{
    if (result.type != RayTraceResultType::Entity || result.hitEntity == nullptr) {
        return;
    }

    // 记录被钩住的实体
    m_caughtEntity = result.hitEntity;

    // 同步实体ID（用于客户端）
    _syncCaughtEntityId();

    // 清零速度
    m_builtIn.velocity->m_velocity = Vector3(0.0, 0.0, 0.0);

    // 切换到钩住状态
    m_state = State::Hooked;
}

void FishingBobberEntity::_onBlockHit(const RayTraceResult& result)
{
    // 命中方块时停止移动，进入漂浮状态
    m_builtIn.velocity->m_velocity = Vector3(0.0, 0.0, 0.0);

    // 如果在水上方块，设置 BOBBING 状态
    if (isInWater()) {
        m_state = State::Bobbing;
        _setWaitTime();
        m_inOpenWater = _checkOpenWater();
    }
}

void FishingBobberEntity::_bringInHookedEntity()
{
    if (m_caughtEntity == nullptr || m_angler == nullptr) {
        return;
    }

    // 计算从浮标指向钓鱼者的方向向量，缩放到 10% 的力
    Vector3d direction(m_angler->x() - x(), m_angler->y() - y(), m_angler->z() - z());
    direction = direction * 0.1;

    // 叠加到被钩实体的速度上
    m_caughtEntity->addVelocity(
        static_cast<f32>(direction.x), static_cast<f32>(direction.y), static_cast<f32>(direction.z));
}

void FishingBobberEntity::_syncCaughtEntityId()
{
    // 存储时 +1，因为 0 表示"无实体"
    // 对应 MC 1.21.11 FishingHook.setHookedEntity():
    //   this.getEntityData().set(DATA_HOOKED_ENTITY, p_150158_ == null ? 0 : p_150158_.getId() + 1);
    i32 syncedId = (m_caughtEntity != nullptr) ? static_cast<i32>(m_caughtEntity->id()) + 1 : 0;
    m_caughtEntityId = syncedId;

    // 通过 EntityDataManager 同步到客户端
    // EntityTracker 会在 tick() 中检测脏数据并广播 ir::play::SetEntityData
    m_dataManager.set(DATA_HOOKED_ENTITY_PARAM, syncedId);
}

// ============================================================================
// ShulkerBulletEntity
// ============================================================================

ShulkerBulletEntity::ShulkerBulletEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
    , m_direction(Direction::Up)
    , m_targetDelta(0.0, 0.0, 0.0)
{
    m_noGravity = true;
    m_noClip = true; // 穿墙
}

ShulkerBulletEntity::ShulkerBulletEntity(
    IWorld* world, LivingEntity* shooter, Entity* target, Axis axis, ecs::EntityRegistry& registry)
    : ShulkerBulletEntity(0, registry)
{
    if (shooter) {
        setShooter(shooter);
        // 设置初始位置在潜影贝中心
        BlockPos shooterPos(static_cast<i32>(std::floor(shooter->x())),
            static_cast<i32>(std::floor(shooter->y())),
            static_cast<i32>(std::floor(shooter->z())));
        setPosition(shooterPos.x + 0.5, shooterPos.y + 0.5, shooterPos.z + 0.5);
    }

    m_target = target;
    if (target) {
        m_targetUuid = target->uuid();
    }

    m_direction = Direction::Up;
    _selectNextMoveDirection(axis);
}

std::unique_ptr<Entity> ShulkerBulletEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ShulkerBulletEntity>(0, registry);
}

void ShulkerBulletEntity::setTarget(Entity* target)
{
    m_target = target;
    if (target) {
        m_targetUuid = target->uuid();
    }
}

void ShulkerBulletEntity::_setDirection(Direction dir)
{
    m_direction = dir;
}

void ShulkerBulletEntity::tick()
{
    // 服务端逻辑
    if (m_world != nullptr) {
        // 检查目标是否有效
        Player* playerTarget = dynamic_cast<Player*>(m_target);
        if (m_target == nullptr || !m_target->isAlive() || (playerTarget != nullptr && playerTarget->isSpectator())) {
            // 目标无效，下落
            if (!m_noGravity) {
                m_builtIn.velocity->m_velocity.y -= 0.04;
            }
        } else {
            // 加速追踪
            m_targetDelta.x = std::clamp(m_targetDelta.x * ACCELERATION, -1.0, 1.0);
            m_targetDelta.y = std::clamp(m_targetDelta.y * ACCELERATION, -1.0, 1.0);
            m_targetDelta.z = std::clamp(m_targetDelta.z * ACCELERATION, -1.0, 1.0);

            // 向目标方向加速
            m_builtIn.velocity->m_velocity.x += (m_targetDelta.x - m_builtIn.velocity->m_velocity.x) * 0.2;
            m_builtIn.velocity->m_velocity.y += (m_targetDelta.y - m_builtIn.velocity->m_velocity.y) * 0.2;
            m_builtIn.velocity->m_velocity.z += (m_targetDelta.z - m_builtIn.velocity->m_velocity.z) * 0.2;
        }

        // 执行射线检测
        RayTraceResult hitResult = performRayTrace();
        if (hitResult.type != RayTraceResultType::Miss) {
            onImpact(hitResult);
            return;
        }
    }

    // 更新位置
    m_builtIn.stateVector->m_pos.x += m_builtIn.velocity->m_velocity.x;
    m_builtIn.stateVector->m_pos.y += m_builtIn.velocity->m_velocity.y;
    m_builtIn.stateVector->m_pos.z += m_builtIn.velocity->m_velocity.z;

    // 更新旋转朝向运动方向
    ProjectileEntity::updateRotation();

    // 更新飞行逻辑
    if (m_world != nullptr && m_target != nullptr && m_target->isAlive()) {
        // 更新飞行步数
        if (m_flightSteps > 0) {
            m_flightSteps--;

            // 步数用完时重新选择方向
            if (m_flightSteps == 0) {
                Axis excludeAxis = (m_direction != Direction::None) ? Directions::getAxis(m_direction) : Axis::Y;
                _selectNextMoveDirection(excludeAxis);
            }
        }

        // 检查是否需要改变方向
        if (m_direction != Direction::None) {
            BlockPos currentPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
            Axis axis = Directions::getAxis(m_direction);

            // 检查前方是否有方块
            BlockPos nextPos(currentPos.x + Directions::xOffset(m_direction),
                currentPos.y + Directions::yOffset(m_direction),
                currentPos.z + Directions::zOffset(m_direction));

            const BlockState* nextState = m_world->getBlockState(nextPos);
            if (nextState != nullptr && nextState->blocksMovement()) {
                // 前方有方块，选择新方向
                _selectNextMoveDirection(axis);
            } else {
                // 检查是否与目标对齐
                BlockPos targetPos(static_cast<i32>(std::floor(m_target->x())),
                    static_cast<i32>(std::floor(m_target->y())),
                    static_cast<i32>(std::floor(m_target->z())));
                bool aligned = false;
                switch (axis) {
                    case Axis::X:
                        aligned = (currentPos.x == targetPos.x);
                        break;
                    case Axis::Y:
                        aligned = (currentPos.y == targetPos.y);
                        break;
                    case Axis::Z:
                        aligned = (currentPos.z == targetPos.z);
                        break;
                }
                if (aligned) {
                    _selectNextMoveDirection(axis);
                }
            }
        }
    }
}

void ShulkerBulletEntity::_selectNextMoveDirection(Axis excludedAxis)
{
    f64 targetOffsetY = 0.5;
    BlockPos targetPos;

    if (m_target == nullptr) {
        targetPos = BlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)) - 1,
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    } else {
        targetOffsetY = static_cast<f64>(m_target->height()) * 0.5;
        targetPos = BlockPos(static_cast<i32>(std::floor(m_target->x())),
            static_cast<i32>(std::floor(m_target->y() + targetOffsetY)),
            static_cast<i32>(std::floor(m_target->z())));
    }

    f64 targetX = targetPos.x + 0.5;
    f64 targetY = targetPos.y + targetOffsetY;
    f64 targetZ = targetPos.z + 0.5;

    Direction newDirection = Direction::None;

    // 如果距离足够近（<2格），直接向目标移动
    BlockPos myPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    f64 distSq = static_cast<f64>(myPos.x - targetPos.x) * (myPos.x - targetPos.x) +
        static_cast<f64>(myPos.y - targetPos.y) * (myPos.y - targetPos.y) +
        static_cast<f64>(myPos.z - targetPos.z) * (myPos.z - targetPos.z);

    if (distSq >= 4.0 && m_world != nullptr) { // 距离 >= 2格
        std::vector<Direction> possibleDirs;

        // 收集可行方向
        if (excludedAxis != Axis::X) {
            if (myPos.x < targetPos.x) {
                const BlockState* eastState = m_world->getBlockState(BlockPos(myPos.x + 1, myPos.y, myPos.z));
                if (eastState == nullptr || !eastState->blocksMovement()) {
                    possibleDirs.push_back(Direction::East);
                }
            } else if (myPos.x > targetPos.x) {
                const BlockState* westState = m_world->getBlockState(BlockPos(myPos.x - 1, myPos.y, myPos.z));
                if (westState == nullptr || !westState->blocksMovement()) {
                    possibleDirs.push_back(Direction::West);
                }
            }
        }

        if (excludedAxis != Axis::Y) {
            if (myPos.y < targetPos.y) {
                const BlockState* upState = m_world->getBlockState(BlockPos(myPos.x, myPos.y + 1, myPos.z));
                if (upState == nullptr || !upState->blocksMovement()) {
                    possibleDirs.push_back(Direction::Up);
                }
            } else if (myPos.y > targetPos.y) {
                const BlockState* downState = m_world->getBlockState(BlockPos(myPos.x, myPos.y - 1, myPos.z));
                if (downState == nullptr || !downState->blocksMovement()) {
                    possibleDirs.push_back(Direction::Down);
                }
            }
        }

        if (excludedAxis != Axis::Z) {
            if (myPos.z < targetPos.z) {
                const BlockState* southState = m_world->getBlockState(BlockPos(myPos.x, myPos.y, myPos.z + 1));
                if (southState == nullptr || !southState->blocksMovement()) {
                    possibleDirs.push_back(Direction::South);
                }
            } else if (myPos.z > targetPos.z) {
                const BlockState* northState = m_world->getBlockState(BlockPos(myPos.x, myPos.y, myPos.z - 1));
                if (northState == nullptr || !northState->blocksMovement()) {
                    possibleDirs.push_back(Direction::North);
                }
            }
        }

        // 随机选择一个方向
        if (!possibleDirs.empty()) {
            math::Random& rng = m_world->getRandom();
            newDirection = possibleDirs[rng.nextInt(static_cast<i32>(possibleDirs.size()))];
        } else {
            // 没有可行方向，随机选择
            math::Random& rng = m_world->getRandom();
            for (i32 i = 0; i < 5; ++i) {
                Direction randomDir = static_cast<Direction>(rng.nextInt(6));
                BlockPos testPos(myPos.x + Directions::xOffset(randomDir),
                    myPos.y + Directions::yOffset(randomDir),
                    myPos.z + Directions::zOffset(randomDir));
                const BlockState* testState = m_world->getBlockState(testPos);
                if (testState == nullptr || !testState->blocksMovement()) {
                    newDirection = randomDir;
                    break;
                }
            }
        }

        // 计算目标速度
        if (newDirection != Direction::None) {
            targetX = m_builtIn.stateVector->m_pos.x + Directions::xOffset(newDirection);
            targetY = m_builtIn.stateVector->m_pos.y + Directions::yOffset(newDirection);
            targetZ = m_builtIn.stateVector->m_pos.z + Directions::zOffset(newDirection);
        }
    }

    _setDirection(newDirection);

    // 计算速度增量
    f64 dx = targetX - m_builtIn.stateVector->m_pos.x;
    f64 dy = targetY - m_builtIn.stateVector->m_pos.y;
    f64 dz = targetZ - m_builtIn.stateVector->m_pos.z;
    f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (dist == 0.0) {
        m_targetDelta = Vector3d(0.0, 0.0, 0.0);
    } else {
        m_targetDelta = Vector3d(dx / dist * BULLET_SPEED, dy / dist * BULLET_SPEED, dz / dist * BULLET_SPEED);
    }

    // 设置飞行步数
    if (m_world != nullptr) {
        math::Random& rng = m_world->getRandom();
        m_flightSteps = MIN_STEPS + rng.nextInt(MAX_STEPS_EXTRA) * 10;
    }
}

[[nodiscard]] bool ShulkerBulletEntity::canHitEntity(const Entity& target) const
{
    // 不能击中发射者、noClip实体或自己
    if (&target == this) {
        return false;
    }
    if (target.noClip()) {
        return false;
    }
    Entity* shooter = getShooter();
    if (&target == shooter) {
        return false;
    }
    return ProjectileEntity::canHitEntity(target);
}

void ShulkerBulletEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    Entity* target = result.hitEntity;
    Entity* shooter = getShooter();
    LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);

    // 创建伤害源
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, this, this, false);
    }

    // 造成伤害
    bool damaged = target->hurt(*damageSource, DAMAGE);

    if (damaged) {
        // 如果目标是 LivingEntity，目标的荆棘附魔会对发射者反伤
        // 注意：荆棘附魔已在 LivingEntity::actuallyHurt() 中自动触发，无需在此重复调用

        // 发射者的攻击型附魔（节肢杀手等）对目标生效
        if (livingShooter != nullptr) {
            item::enchant::EnchantmentHelper::applyArthropodEnchantments(*livingShooter, *target);
        }

        // 对 LivingEntity 施加漂浮效果
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
        if (livingTarget != nullptr) {
            // 200 ticks = 10秒漂浮
            livingTarget->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Levitation,
                static_cast<i32>(LEVITATION_DURATION),
                0, // amplifier = 0 (I级效果)
                false,
                true,
                true));
        }
    }
}

void ShulkerBulletEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // 命中方块时生成爆炸粒子
    if (m_world != nullptr) {
        m_world->addParticle(particle::ParticleTypeId::Explosion,
            m_builtIn.stateVector->m_pos,
            Vector3(0.0f, 0.0f, 0.0f), // 速度为 0
            Vector3(0.2f, 0.2f, 0.2f), // 随机偏移范围
            2);                        // 数量 2
    }
    playSound(SoundEvents::ENTITY_SHULKER_BULLET_HIT, 1.0f, 1.0f);
}

void ShulkerBulletEntity::onImpact(const RayTraceResult& result)
{
    // 调用父类
    ProjectileEntity::onImpact(result);

    // 命中后移除
    remove();
}

// ============================================================================
// EvokerFangsEntity
// ============================================================================

EvokerFangsEntity::EvokerFangsEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    m_warmupDelay = 0;
}

std::unique_ptr<Entity> EvokerFangsEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EvokerFangsEntity>(0, registry);
}

void EvokerFangsEntity::setOwner(LivingEntity* owner)
{
    m_owner = owner;
    if (owner != nullptr) {
        m_ownerUuid = owner->uuid();
    } else {
        m_ownerUuid.clear();
    }
}

LivingEntity* EvokerFangsEntity::getOwner()
{
    // 如果缓存指针有效且实体未被移除，直接返回
    if (m_owner != nullptr && m_owner->isAlive()) {
        return m_owner;
    }

    // 缓存失效，尝试通过 UUID 在世界中重新查找
    // 使用 IWorld::getEntityByUuid() 进行 O(1) 查找
    if (!m_ownerUuid.empty() && m_world != nullptr) {
        Entity* entity = m_world->getEntityByUuid(m_ownerUuid);
        if (entity != nullptr && entity->isAlive()) {
            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living != nullptr) {
                m_owner = living;
                return m_owner;
            }
        }
    }

    // UUID 查找也失败，清空缓存指针
    m_owner = nullptr;
    return nullptr;
}

void EvokerFangsEntity::setOwnerUuid(const std::string& uuid)
{
    m_ownerUuid = uuid;
    // 不设置 m_owner 指针，等到 getOwner() 被调用时再通过 UUID 懒加载查找
    m_owner = nullptr;
}

void EvokerFangsEntity::tick()
{
    Entity::tick();

    // 服务端逻辑
    if (m_warmupDelay > 0) {
        m_warmupDelay--;
    } else {
        // warmupDelayTicks < 0 后开始攻击逻辑
        if (m_warmupDelay == -8) {
            // 在 -8 tick 时对范围内实体造成伤害
            _damageEntities();
        }

        if (!m_sentAttackEvent) {
            // 发送攻击事件给客户端（用于播放音效和粒子）
            m_sentAttackEvent = true;
        }

        m_lifeTicks--;
        if (m_lifeTicks < 0) {
            remove();
        }
    }
}

f32 EvokerFangsEntity::getAnimationProgress(f32 partialTicks) const
{
    if (!m_clientSideAttackStarted) {
        return 0.0f;
    } else {
        i32 i = m_lifeTicks - 2;
        return i <= 0 ? 1.0f : 1.0f - (static_cast<f32>(i) - partialTicks) / 20.0f;
    }
}

void EvokerFangsEntity::_damageEntities()
{
    // 对碰撞箱扩展范围内的 LivingEntity 造成伤害
    if (m_world == nullptr) {
        return;
    }

    // 获取 owner（可能触发 UUID 懒加载查找）
    LivingEntity* owner = getOwner();

    // 获取碰撞箱扩展 0.2 范围内的所有实体
    AxisAlignedBB searchBox = m_builtIn.aabbShape->m_aabb.expand(0.2f, 0.0f, 0.2f);
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : entities) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living == nullptr || living == owner) {
            continue;
        }

        // 检查实体是否存活且非无敌
        if (!living->isAlive() || living->isInvulnerable()) {
            continue;
        }

        // 检查队伍关系：不伤害唤魔者及其盟友
        // 参考 MC 1.21.11 EvokerFangs.dealDamageTo()
        if (owner != nullptr && owner->isAlliedTo(*living)) {
            continue;
        }

        // 造成魔法伤害
        if (owner != nullptr) {
            auto damageSource = DamageSources::indirectMagic(this, owner);
            living->hurt(damageSource, 6.0f);
        } else {
            // 如果没有所有者，使用普通魔法伤害
            auto damageSource = DamageSources::magic();
            living->hurt(damageSource, 6.0f);
        }
    }
}

void EvokerFangsEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    Entity::addAdditionalSaveData(tag);

    using namespace serialization::nbt_keys;

    // 预热延迟
    tag.put(WARMUP, m_warmupDelay);

    // 所有者 UUID
    // 参考 MC 1.21.11 EvokerFangs.addAdditionalSaveData()，NBT 键名为 "Owner"
    // 使用 OwnerUUIDMost/OwnerUUIDLeast 双 long 格式存储，与 AreaEffectCloudEntity 一致
    if (!m_ownerUuid.empty()) {
        auto uuidBytes = util::uuidFromString(m_ownerUuid);
        if (uuidBytes.size() == 16) {
            i64 most = (static_cast<i64>(uuidBytes[0]) << 56) | (static_cast<i64>(uuidBytes[1]) << 48) |
                (static_cast<i64>(uuidBytes[2]) << 40) | (static_cast<i64>(uuidBytes[3]) << 32) |
                (static_cast<i64>(uuidBytes[4]) << 24) | (static_cast<i64>(uuidBytes[5]) << 16) |
                (static_cast<i64>(uuidBytes[6]) << 8) | static_cast<i64>(uuidBytes[7]);
            i64 least = (static_cast<i64>(uuidBytes[8]) << 56) | (static_cast<i64>(uuidBytes[9]) << 48) |
                (static_cast<i64>(uuidBytes[10]) << 40) | (static_cast<i64>(uuidBytes[11]) << 32) |
                (static_cast<i64>(uuidBytes[12]) << 24) | (static_cast<i64>(uuidBytes[13]) << 16) |
                (static_cast<i64>(uuidBytes[14]) << 8) | static_cast<i64>(uuidBytes[15]);
            tag.put(FANGS_OWNER_UUID_MOST, most);
            tag.put(FANGS_OWNER_UUID_LEAST, least);
        }
    }
}

Result<void> EvokerFangsEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(Entity::readAdditionalSaveData(tag));

    using namespace serialization::nbt_keys;

    // 预热延迟
    if (auto val = serialization::nbt_helper::tryGetInt(tag, WARMUP)) {
        m_warmupDelay = *val;
    }

    // 所有者 UUID
    // 读取 FANGS_OWNER_UUID_MOST/FANGS_OWNER_UUID_LEAST，转换为 UUID 字符串
    auto mostVal = serialization::nbt_helper::tryGetLong(tag, FANGS_OWNER_UUID_MOST);
    auto leastVal = serialization::nbt_helper::tryGetLong(tag, FANGS_OWNER_UUID_LEAST);
    if (mostVal.has_value() && leastVal.has_value()) {
        i64 m = mostVal.value();
        i64 l = leastVal.value();
        std::array<u8, 16> uuidBytes{};
        for (i32 i = 7; i >= 0; --i) {
            uuidBytes[i] = static_cast<u8>(m & 0xFF);
            m >>= 8;
        }
        for (i32 i = 15; i >= 8; --i) {
            uuidBytes[i] = static_cast<u8>(l & 0xFF);
            l >>= 8;
        }
        m_ownerUuid = util::uuidToString(uuidBytes);
        m_owner = nullptr; // 等 getOwner() 被调用时再通过 UUID 懒加载查找
    } else {
        m_ownerUuid.clear();
        m_owner = nullptr;
    }

    return Result<void>::ok();
}

// ============================================================================
// EyeOfEnderEntity
// ============================================================================

EyeOfEnderEntity::EyeOfEnderEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    m_noGravity = false;
}

std::unique_ptr<Entity> EyeOfEnderEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EyeOfEnderEntity>(0, registry);
}

void EyeOfEnderEntity::tick()
{
    Entity::tick();

    m_lifetime++;

    // 向目标移动
    if (m_targetX != 0 || m_targetZ != 0) {
        // 计算方向
        f32 dx = static_cast<f32>(m_targetX) - m_builtIn.stateVector->m_pos.x;
        f32 dz = static_cast<f32>(m_targetZ) - m_builtIn.stateVector->m_pos.z;
        f32 dist = std::sqrt(dx * dx + dz * dz);

        if (dist > 0.0f) {
            // 设置速度
            m_builtIn.velocity->m_velocity.x = dx / dist * 0.5f;
            m_builtIn.velocity->m_velocity.z = dz / dist * 0.5f;

            // Y轴波动
            m_builtIn.velocity->m_velocity.y = std::sin(m_lifetime * 0.1f) * 0.1f;
        }
    }

    // 随机碎裂
    // 15% 概率碎裂
    // if (rand.nextInt(700) == 0) {
    //     m_break = true;
    //     remove();
    // }

    // 超时移除
    if (m_lifetime > 1200) { // 60秒
        remove();
    }
}

void EyeOfEnderEntity::moveTo(BlockCoord targetX, BlockCoord targetZ)
{
    m_targetX = targetX;
    m_targetZ = targetZ;
}

// ============================================================================
// FireworkRocketEntity
// ============================================================================

FireworkRocketEntity::FireworkRocketEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
    , m_fireworkItem(Items::AIR, 0) // 初始化为空物品
{
    m_noGravity = false;
}

std::unique_ptr<Entity> FireworkRocketEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<FireworkRocketEntity>(0, registry);
}

void FireworkRocketEntity::setFireworkItem(const ItemStack& item)
{
    m_fireworkItem = item;

    // 从物品 NBT 读取飞行时间
    const nlohmann::json* tag = item.getTag();
    if (tag != nullptr) {
        auto fireworksIt = tag->find("Fireworks");
        if (fireworksIt != tag->end() && fireworksIt->is_object()) {
            auto flightIt = fireworksIt->find("Flight");
            if (flightIt != fireworksIt->end() && flightIt->is_number()) {
                m_flightTime = std::max(1, flightIt->get<i32>());
            }
        }
    }

    // 物品变更后，已计算的 lifeTime 失效，需重新懒初始化
    // （NBT 反序列化路径会在 setFireworkItem 之后显式 setLifeTime 覆盖此处的重置）
    m_lifeTime = -1;
}

void FireworkRocketEntity::_ensureLifeTimeComputed()
{
    if (m_lifeTime >= 0) {
        return; // 已计算或已从 NBT 恢复
    }

    // lifeTime = flightTime * 10 + rand.nextInt(6) + rand.nextInt(7)
    // 使用世界随机数生成器一次性确定，保证服务端确定性；
    // 客户端不跑 FireworkRocketEntity::tick，无需此值
    if (m_world != nullptr && !m_world->isClientSide()) {
        math::Random& rng = m_world->getRandom();
        m_lifeTime = m_flightTime * 10 + rng.nextInt(6) + rng.nextInt(7);
    }
}

i32 FireworkRocketEntity::getExplosionCount() const
{
    // 从物品 NBT 读取爆炸效果数量
    const nlohmann::json* tag = m_fireworkItem.getTag();
    if (tag == nullptr) {
        return 0;
    }

    auto fireworksIt = tag->find("Fireworks");
    if (fireworksIt == tag->end() || !fireworksIt->is_object()) {
        return 0;
    }

    auto explosionsIt = fireworksIt->find("Explosions");
    if (explosionsIt == fireworksIt->end() || !explosionsIt->is_array()) {
        return 0;
    }

    return static_cast<i32>(explosionsIt->size());
}

void FireworkRocketEntity::tick()
{
    ProjectileEntity::tick();

    m_lifetime++;

    // 生成飞行粒子（每 2 tick 生成一次粒子，仅在客户端执行）
    if (m_world != nullptr && m_world->isClientSide() && m_lifetime % 2 == 0) {
        // 粒子位置在火箭下方 0.3 格
        Vector3 particlePos(x(), y() - 0.3, z());

        // 使用高斯分布随机速度
        mc::math::Random rng = createRandomFromEntity(*this);
        f32 vx = static_cast<f32>(rng.nextGaussian() * 0.05);
        f32 vy = static_cast<f32>(-m_builtIn.velocity->m_velocity.y * 0.5); // Y速度与火箭运动方向相反
        f32 vz = static_cast<f32>(rng.nextGaussian() * 0.05);

        m_world->addParticle(particle::ParticleTypeId::Firework, particlePos, Vector3(vx, vy, vz));
    }

    // 懒计算总生命时间（仅服务端，客户端不跑此 tick）
    _ensureLifeTimeComputed();

    // 检查是否爆炸：lifeTime = flightTime * 10 + rand.nextInt(6) + rand.nextInt(7)
    // 若 lifeTime 尚未计算（如客户端或无世界场景），回退到 flightTime * 10 + 6 的旧简化行为
    //
    // TODO[firework-client-lifetime]: 此处客户端回退公式 flightTime*10+6 仅为防御性代码，
    // 与 MC 原版客户端行为存在偏差。MC 原版客户端通过实体同步的随机种子（由 entityId 派生）
    // 重新计算 lifeTime，保证双端爆炸时序一致。本项目当前 Entity::m_random 用
    // entityId^clock 初始化导致跨端不一致，且 createRandomFromEntity 不适用于 lifeTime
    // 计算（lifeTime 应在创建时一次性确定而非每 tick 重算）。
    // 当前架构下客户端不跑 FireworkRocketEntity::tick（爆炸由服务端 remove 数据包驱动），
    // 故此回退分支仅在测试或异常路径触发。若未来引入客户端独立 tick 路径，需改为通过
    // EntityDataManager 同步 lifeTime 字段或使用跨端确定性 RNG 重新计算。
    const i32 explodeThreshold = (m_lifeTime >= 0) ? m_lifeTime : (m_flightTime * 10 + 6);
    if (m_lifetime >= explodeThreshold) {
        _explode();
    }
}

void FireworkRocketEntity::_explode()
{
    // 如果从弩射出，对周围实体造成伤害
    if (m_shotFromCrossbow) {
        dealExplosionDamage();
    }

    // 生成爆炸粒子
    if (m_world != nullptr && m_world->isClientSide()) {
        mc::math::Random rng = createRandomFromEntity(*this);

        // 获取爆炸效果数量
        i32 explosionCount = getExplosionCount();

        if (explosionCount <= 0) {
            // 无爆炸效果时，生成简单的消散粒子（2-4 个 POOF 粒子）
            i32 poofCount = 2 + rng.nextInt(3);
            for (i32 i = 0; i < poofCount; ++i) {
                f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;
                f32 oy = (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;
                f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;

                m_world->addParticle(
                    particle::ParticleTypeId::Poof, Vector3(x() + ox, y() + oy, z() + oz), Vector3(0.0f, 0.0f, 0.0f));
            }
        } else {
            // 有爆炸效果时，生成烟花粒子
            // 生成爆炸闪光
            m_world->addParticle(particle::ParticleTypeId::Flash, Vector3(x(), y(), z()), Vector3(0.0f, 0.0f, 0.0f));

            // 生成主要爆炸粒子云
            i32 particleCount = 20 + rng.nextInt(20);
            for (i32 i = 0; i < particleCount; ++i) {
                // 球形分布
                f32 theta = rng.nextFloat() * math::TWO_PI;
                f32 phi = rng.nextFloat() * math::PI;
                f32 speed = 0.1f + rng.nextFloat() * 0.3f;

                f32 vx = std::sin(phi) * std::cos(theta) * speed;
                f32 vy = std::sin(phi) * std::sin(theta) * speed;
                f32 vz = std::cos(phi) * speed;

                f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;
                f32 oy = (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;
                f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;

                m_world->addParticle(
                    particle::ParticleTypeId::Firework, Vector3(x() + ox, y() + oy, z() + oz), Vector3(vx, vy, vz));
            }

            // 生成烟雾粒子
            for (i32 i = 0; i < 5 + rng.nextInt(5); ++i) {
                f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * 0.5f;
                f32 oy = (rng.nextFloat() * 2.0f - 1.0f) * 0.5f;
                f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * 0.5f;

                m_world->addParticle(
                    particle::ParticleTypeId::Poof, Vector3(x() + ox, y() + oy, z() + oz), Vector3(0.0f, 0.02f, 0.0f));
            }
        }
    }

    remove();
}

void FireworkRocketEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    Entity::addAdditionalSaveData(tag);

    using namespace serialization::nbt_keys;

    // 烟花物品（compound，由 ItemStack::toNbt 写入）
    if (!m_fireworkItem.isEmpty()) {
        nbt::tags::compound_tag itemTag;
        m_fireworkItem.toNbt(itemTag);
        tag.value.emplace(FIREWORKS_ITEM, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
    }

    // 已存在时间
    tag.put(LIFE, m_lifetime);

    // 总生命时间（创建时一次性确定的随机值）
    // 仅在已计算时写出，避免写出 -1 占位符
    if (m_lifeTime >= 0) {
        tag.put(LIFE_TIME, m_lifeTime);
    }

    // 是否从弩射出（i8 bool，与 MC 原版 ShotAtAngle 一致）
    tag.put(SHOT_AT_ANGLE, static_cast<i8>(m_shotFromCrossbow ? 1 : 0));
}

Result<void> FireworkRocketEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(Entity::readAdditionalSaveData(tag));

    using namespace serialization::nbt_keys;

    // 烟花物品（恢复 m_fireworkItem 与 m_flightTime）
    if (const nbt::tags::compound_tag* itemTag = serialization::nbt_helper::tryGetCompound(tag, FIREWORKS_ITEM)) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success()) {
            setFireworkItem(stackResult.value());
        }
    }

    // 已存在时间
    if (auto val = serialization::nbt_helper::tryGetInt(tag, LIFE)) {
        m_lifetime = *val;
    }

    // 总生命时间（覆盖 setFireworkItem 中的 -1 重置）
    if (auto val = serialization::nbt_helper::tryGetInt(tag, LIFE_TIME)) {
        m_lifeTime = *val;
    }

    // 是否从弩射出
    if (auto val = serialization::nbt_helper::tryGetByte(tag, SHOT_AT_ANGLE)) {
        m_shotFromCrossbow = (*val != 0);
    }

    return Result<void>::ok();
}

void FireworkRocketEntity::dealExplosionDamage()
{
    if (m_world == nullptr) {
        return;
    }

    // 获取爆炸效果数量（无爆炸效果时不造成伤害）
    i32 explosionCount = getExplosionCount();
    if (explosionCount <= 0) {
        return;
    }

    // 计算基础伤害：5 + 爆炸效果数量 * 2
    f32 baseDamage = 5.0f + static_cast<f32>(explosionCount * 2);

    // 爆炸半径 5 格
    constexpr f32 EXPLOSION_RADIUS = 5.0f;
    constexpr f64 EXPLOSION_RADIUS_SQ = 25.0; // 5.0 * 5.0

    // 获取爆炸范围内的所有 LivingEntity
    AxisAlignedBB searchBox = boundingBox().grow(EXPLOSION_RADIUS);
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(searchBox, this);

    // 获取发射者（用于伤害源）
    Entity* shooter = getShooter();
    bool isPlayer = (shooter != nullptr && dynamic_cast<Player*>(shooter) != nullptr);

    for (Entity* entity : entities) {
        // 只对 LivingEntity 造成伤害
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(entity);
        if (livingTarget == nullptr || !livingTarget->isAlive()) {
            continue;
        }

        // 计算距离
        f64 distanceSq = static_cast<f64>(livingTarget->distanceSqTo(*this));
        if (distanceSq > EXPLOSION_RADIUS_SQ) {
            continue;
        }

        // 视线检测（MC 1.16.5 射线追踪两条射线）
        if (!canSeeEntity(*livingTarget)) {
            continue;
        }

        // 计算距离衰减伤害
        f64 distance = std::sqrt(distanceSq);
        f32 damage = baseDamage * static_cast<f32>(std::sqrt((EXPLOSION_RADIUS - distance) / EXPLOSION_RADIUS));

        if (damage > 0.0f) {
            // 创建烟花伤害源
            auto damageSource = DamageSources::fireworks();

            // 对目标造成伤害
            livingTarget->hurt(damageSource, damage);
        }
    }
}

bool FireworkRocketEntity::canSeeEntity(const Entity& target) const
{
    // 参考 MC 1.16.5 FireworkRocketEntity.dealExplosionDamage()
    // 发射两条射线：脚部（y=0）和腰部（y=height*0.5）
    // 只要有一条射线未被方块阻挡，就可以造成伤害

    if (m_world == nullptr) {
        return false;
    }

    Vector3 rocketPos = position();
    Vector3 targetPos = target.position();

    // 两条射线的高度偏移
    constexpr f64 HEIGHT_OFFSETS[] = {0.0, 0.5};

    for (f64 heightOffset : HEIGHT_OFFSETS) {
        // 计算目标点位置
        Vector3 targetPoint(targetPos.x, targetPos.y + target.height() * heightOffset, targetPos.z);

        // 创建射线追踪上下文
        // 使用 COLLIDER 模式检测方块碰撞，忽略流体
        Ray ray(rocketPos, (targetPoint - rocketPos).normalized());
        f64 maxDistance = (targetPoint - rocketPos).length();

        RaycastContext context(ray, maxDistance);
        BlockRaycastResult result = raycastBlocks(context, *m_world);

        // 如果射线未被阻挡（MISS），则可以造成伤害
        if (result.isMiss()) {
            return true;
        }
    }

    // 两条射线都被阻挡
    return false;
}

} // namespace entity
} // namespace mc
