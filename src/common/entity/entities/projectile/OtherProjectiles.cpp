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
#include "common/entity/ecs/components/EvokerFangsComponent.hpp"
#include "common/entity/ecs/components/EyeOfEnderComponent.hpp"
#include "common/entity/ecs/components/FireworkRocketComponent.hpp"
#include "common/entity/ecs/components/FishingBobberComponent.hpp"
#include "common/entity/ecs/components/ShulkerBulletComponent.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
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
    setNoGravity(false);
    // C++ 虚函数在构造函数中不会派生到子类，因此 Entity 基类构造函数中
    // 调用的 registerData() 只会执行 Entity::registerData()。
    // 子类必须在此显式调用自身的 registerData() 以注册子类专属数据参数。
    registerData();
    // 批次6 子目标2 Step1：attach FishingBobberComponent（钓鱼浮标 13 字段）。
    // FishingBobberEntity 直接继承 Entity 不经 ProjectileEntity，独立 owner（m_angler），
    // 故不挂 ProjectileOwnerComponent，本组件独立承载 angler 引用。
    // Step4 将把 m_angler/m_caughtEntity/m_state 等 13 字段读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::FishingBobberComponent>(m_entityContext->entity());
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
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber != nullptr) {
        bobber->m_angler = dynamic_cast<Player*>(shooter);
    }
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

    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr) {
        return;
    }

    bobber->m_lifetime++;

    // 检查钓鱼者是否存在
    Player* angler = bobber->m_angler;
    if (angler == nullptr || !angler->isAlive()) {
        remove();
        return;
    }

    // 检查玩家是否还持有钓鱼竿
    // 如果玩家切换了物品或钓鱼浮标ID已被清除，则移除浮标
    if (!angler->isFishing() || angler->fishingBobber() != id()) {
        remove();
        return;
    }

    // 更新水面状态
    _updateWaterState();

    switch (bobber->m_state) {
        case FishingBobberState::Flying: {
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
                bobber->m_state = FishingBobberState::Bobbing;
                // 设置初始等待时间
                _setWaitTime();
                // 检测是否在开放水域
                bobber->m_inOpenWater = _checkOpenWater();
            }
            break;
        }

        case FishingBobberState::Bobbing:
            // 浮标浮在水面，执行钓鱼逻辑
            if (isInWater()) {
                bobber->m_outOfWaterTime = std::max(0, bobber->m_outOfWaterTime - 1);
                // 检查开放水域状态（进入水后延迟检查）
                if (bobber->m_outOfWaterTime < 10) {
                    bobber->m_inOpenWater = bobber->m_inOpenWater && _checkOpenWater();
                }
                _catchingFish();
            } else {
                bobber->m_outOfWaterTime = std::min(10, bobber->m_outOfWaterTime + 1);
            }
            _spawnFishingParticles();
            break;

        case FishingBobberState::Fishing:
            // 咬钩中，等待玩家收杆
            if (bobber->m_ticksCatchable > 0) {
                bobber->m_ticksCatchable--;
                bobber->m_fishAngle += 0.15f; // 鱼游动动画
                // 如果超时未收杆，重置状态
                if (bobber->m_ticksCatchable <= 0) {
                    bobber->m_state = FishingBobberState::Bobbing;
                    _setWaitTime();
                    // 对应 MC 1.21.11 FishingHook.catchingFish(): nibble 归零时
                    // 设置 DATA_BITING = false，客户端停止咬钩动画。
                    m_dataManager.set(DATA_BITING_PARAM, false);
                }
            } else {
                bobber->m_state = FishingBobberState::Bobbing;
                // 防御性：确保 DATA_BITING 已清除（虽然理论上不应进入此分支时仍为 true）
                m_dataManager.set(DATA_BITING_PARAM, false);
            }
            break;

        case FishingBobberState::Hooked:
            // 钩住实体
            if (bobber->m_caughtEntity != nullptr) {
                if (bobber->m_caughtEntity->isRemoved() || !bobber->m_caughtEntity->isAlive()) {
                    // 实体被移除或死亡，恢复飞行状态
                    // 先清除 m_caughtEntity 再调用 _syncCaughtEntityId()，
                    // 以确保客户端同步收到 DATA_HOOKED_ENTITY=0 的更新。
                    bobber->m_caughtEntity = nullptr;
                    _syncCaughtEntityId();
                    bobber->m_state = FishingBobberState::Flying;
                } else {
                    // 浮标跟随实体位置（设置位置到实体高度的 80% 处）
                    setPosition(bobber->m_caughtEntity->x(),
                        static_cast<f32>(bobber->m_caughtEntity->getY(0.8)),
                        bobber->m_caughtEntity->z());
                }
            } else {
                // 没有被钩住的实体，恢复飞行状态
                bobber->m_state = FishingBobberState::Flying;
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

    FishingWaterType prevType = FishingWaterType::Invalid;

    for (i32 dy = -1; dy <= 2; ++dy) {
        BlockPos from(bobberPos.x - 2, bobberPos.y + dy, bobberPos.z - 2);
        BlockPos to(bobberPos.x + 2, bobberPos.y + dy, bobberPos.z + 2);
        FishingWaterType layerType = _getOpenWaterTypeForArea(from, to);

        switch (layerType) {
            case FishingWaterType::Invalid:
                return false;
            case FishingWaterType::AboveWater:
                // 水上方块不能出现在最底层（前一层还是 Invalid 表示第一层就是 AboveWater）
                if (prevType == FishingWaterType::Invalid) {
                    return false;
                }
                break;
            case FishingWaterType::InsideWater:
                // 水内部不能出现在水上方块之后（不能从水面再回到水下）
                if (prevType == FishingWaterType::AboveWater) {
                    return false;
                }
                break;
        }

        prevType = layerType;
    }

    return true;
}

FishingWaterType FishingBobberEntity::_getOpenWaterTypeForBlock(const BlockPos& pos) const
{
    // 对应 MC Java FishingHook.getOpenWaterTypeFor
    const BlockState* blockState = m_world->getBlockState(pos);
    if (blockState == nullptr) {
        return FishingWaterType::Invalid;
    }

    // 空气 → AboveWater
    if (blockState->isAir()) {
        return FishingWaterType::AboveWater;
    }

    // 睡莲 → AboveWater
    if (blockState->is(block_registry::NaturalBlocks::LILY_PAD)) {
        return FishingWaterType::AboveWater;
    }

    // 非空气、非睡莲：检查是否为水源方块且碰撞箱为空
    const fluid::FluidState* fluidState = blockState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER()) &&
        fluidState->isSource() && blockState->getCollisionShape().isEmpty()) {
        return FishingWaterType::InsideWater;
    }

    return FishingWaterType::Invalid;
}

FishingWaterType FishingBobberEntity::_getOpenWaterTypeForArea(const BlockPos& from, const BlockPos& to) const
{
    // 对应 MC Java FishingHook.getOpenWaterTypeForArea
    // 区域内所有方块必须为同一 FishingWaterType，否则整个区域为 Invalid
    FishingWaterType result = FishingWaterType::Invalid;
    bool first = true;

    for (i32 x = from.x; x <= to.x; ++x) {
        for (i32 y = from.y; y <= to.y; ++y) {
            for (i32 z = from.z; z <= to.z; ++z) {
                FishingWaterType type = _getOpenWaterTypeForBlock(BlockPos(x, y, z));
                if (first) {
                    result = type;
                    first = false;
                } else if (type != result) {
                    return FishingWaterType::Invalid;
                }
            }
        }
    }

    return result;
}

void FishingBobberEntity::_catchingFish()
{
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr) {
        return;
    }

    // 阶段1：等待咬钩
    if (bobber->m_ticksCaughtDelay > 0) {
        bobber->m_ticksCaughtDelay--;

        // 接近咬钩时产生水花
        if (bobber->m_ticksCaughtDelay < 100 && bobber->m_ticksCaughtDelay % 10 == 0 && m_world) {
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
    if (bobber->m_ticksCatchableDelay > 0) {
        bobber->m_ticksCatchableDelay--;

        // 产生气泡和钓鱼粒子
        if (bobber->m_ticksCatchableDelay % 5 == 0 && m_world) {
            math::Random rng;
            f32 angle = bobber->m_fishAngle * math::DEG_TO_RAD;
            f32 sinAngle = std::sin(angle);
            f32 cosAngle = std::cos(angle);
            f32 d0 = x() + sinAngle * static_cast<f32>(bobber->m_ticksCatchableDelay) * 0.1f;
            f32 d1 = std::floor(y()) + 1.0f;
            f32 d2 = z() + cosAngle * static_cast<f32>(bobber->m_ticksCatchableDelay) * 0.1f;

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
        bobber->m_fishAngle += 0.1f;

        // 接近完成，进入可捕获状态
        if (bobber->m_ticksCatchableDelay <= 0) {
            bobber->m_ticksCatchable = math::Random().nextInt(MIN_CATCHABLE_TICKS, MAX_CATCHABLE_TICKS);
            bobber->m_state = FishingBobberState::Fishing;
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
    if (bobber->m_ticksCaughtDelay <= 0 && bobber->m_ticksCatchableDelay <= 0 && bobber->m_ticksCatchable <= 0) {
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
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr) {
        return;
    }
    // 设置咬钩等待时间
    // 基础时间: 100-600 ticks
    // 饵钓附魔: 每级减少 100 ticks
    math::Random rng;
    bobber->m_ticksCaughtDelay = rng.nextInt(MIN_WAIT_TICKS, MAX_WAIT_TICKS);
    bobber->m_ticksCaughtDelay -= bobber->m_speedBonus * LURE_REDUCTION;
    bobber->m_ticksCaughtDelay = std::max(20, bobber->m_ticksCaughtDelay); // 最小 1 秒

    // 鱼接近时间
    bobber->m_ticksCatchableDelay = 0;
    bobber->m_ticksCatchable = 0;
}

i32 FishingBobberEntity::_spawnCatchItems()
{
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr) {
        return 0;
    }
    Player* angler = bobber->m_angler;

    // 使用钓鱼掉落表生成物品
    if (!m_world || angler == nullptr) {
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
    f32 totalLuck = static_cast<f32>(bobber->m_luckBonus);
    totalLuck += static_cast<f32>(angler->getAttributeValue(entity::attribute::Attributes::LUCK, 0.0));

    // 获取随机数生成器
    math::Random& random = m_world->getRandom();

    // 构建掉落上下文
    auto context = loot::LootContextBuilder(*m_world)
                       .withRandom(random)
                       .withLuck(totalLuck)
                       .withParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(this))
                       .withParameter(loot::LootParams::KILLER_ENTITY, static_cast<Entity*>(angler))
                       .withOwnedValue(loot::LootParams::IS_IN_OPEN_WATER, bobber->m_inOpenWater)
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
    f64 dx = angler->x() - x();
    f64 dy = angler->y() + angler->eyeHeight() * 0.5 - y();
    f64 dz = angler->z() - z();
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
            vz,            // 朝玩家方向飞
            10,            // 拾取延迟 10 ticks
            angler->uuid() // 所有者 UUID，防止立即拾取自己的物品
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
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr) {
        remove();
        return 0;
    }
    // 收杆
    i32 damage = 0; // 钓鱼竿耐久消耗

    if (bobber->m_state == FishingBobberState::Fishing && bobber->m_ticksCatchable > 0) {
        // 成功钓到鱼
        damage = _spawnCatchItems();
        remove();
    } else if (bobber->m_state == FishingBobberState::Hooked) {
        // 钩住实体，拉过来
        if (bobber->m_caughtEntity != nullptr && bobber->m_caughtEntity->isAlive()) {
            _bringInHookedEntity();
            // 耐久消耗取决于实体类型：物品实体 3，其他实体 5
            if (dynamic_cast<ItemEntity*>(bobber->m_caughtEntity) != nullptr) {
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
    const auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (&target == (bobber != nullptr ? bobber->m_angler : nullptr)) {
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

    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr) {
        return;
    }

    // 记录被钩住的实体
    bobber->m_caughtEntity = result.hitEntity;

    // 同步实体ID（用于客户端）
    _syncCaughtEntityId();

    // 清零速度
    m_builtIn.velocity->m_velocity = Vector3(0.0, 0.0, 0.0);

    // 切换到钩住状态
    bobber->m_state = FishingBobberState::Hooked;
}

void FishingBobberEntity::_onBlockHit(const RayTraceResult& result)
{
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    // 命中方块时停止移动，进入漂浮状态
    m_builtIn.velocity->m_velocity = Vector3(0.0, 0.0, 0.0);

    // 如果在水上方块，设置 BOBBING 状态
    if (isInWater() && bobber != nullptr) {
        bobber->m_state = FishingBobberState::Bobbing;
        _setWaitTime();
        bobber->m_inOpenWater = _checkOpenWater();
    }
}

void FishingBobberEntity::_bringInHookedEntity()
{
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    if (bobber == nullptr || bobber->m_caughtEntity == nullptr || bobber->m_angler == nullptr) {
        return;
    }
    Player* angler = bobber->m_angler;
    Entity* caughtEntity = bobber->m_caughtEntity;

    // 计算从浮标指向钓鱼者的方向向量，缩放到 10% 的力
    Vector3d direction(angler->x() - x(), angler->y() - y(), angler->z() - z());
    direction = direction * 0.1;

    // 叠加到被钩实体的速度上
    caughtEntity->addVelocity(
        static_cast<f32>(direction.x), static_cast<f32>(direction.y), static_cast<f32>(direction.z));
}

void FishingBobberEntity::_syncCaughtEntityId()
{
    auto* bobber = tryGetComponent<ecs::FishingBobberComponent>();
    Entity* caughtEntity = (bobber != nullptr) ? bobber->m_caughtEntity : nullptr;
    // 存储时 +1，因为 0 表示"无实体"
    // 对应 MC 1.21.11 FishingHook.setHookedEntity():
    //   this.getEntityData().set(DATA_HOOKED_ENTITY, p_150158_ == null ? 0 : p_150158_.getId() + 1);
    i32 syncedId = (caughtEntity != nullptr) ? static_cast<i32>(caughtEntity->id()) + 1 : 0;
    if (bobber != nullptr) {
        bobber->m_caughtEntityId = syncedId;
    }

    // 通过 EntityDataManager 同步到客户端
    // EntityTracker 会在 tick() 中检测脏数据并广播 ir::play::SetEntityData
    m_dataManager.set(DATA_HOOKED_ENTITY_PARAM, syncedId);
}

// 批次6 子目标2 Step4：m_angler/m_caughtEntity/m_caughtEntityId/m_state/m_inOpenWater/
// m_luckBonus/m_speedBonus 迁入 ecs::FishingBobberComponent。
Player* FishingBobberEntity::getAngler() const
{
    const auto* c = tryGetComponent<ecs::FishingBobberComponent>();
    return (c != nullptr) ? c->m_angler : nullptr;
}

FishingBobberState FishingBobberEntity::state() const
{
    const auto* c = tryGetComponent<ecs::FishingBobberComponent>();
    return (c != nullptr) ? c->m_state : FishingBobberState::Flying;
}

Entity* FishingBobberEntity::getCaughtEntity() const
{
    const auto* c = tryGetComponent<ecs::FishingBobberComponent>();
    return (c != nullptr) ? c->m_caughtEntity : nullptr;
}

EntityInstanceId FishingBobberEntity::getCaughtEntityId() const
{
    const auto* c = tryGetComponent<ecs::FishingBobberComponent>();
    return (c != nullptr) ? c->m_caughtEntityId : EntityInstanceId{0};
}

void FishingBobberEntity::setFishingBonus(i32 luckBonus, i32 speedBonus)
{
    auto* c = tryGetComponent<ecs::FishingBobberComponent>();
    if (c != nullptr) {
        c->m_luckBonus = luckBonus;
        c->m_speedBonus = speedBonus;
    }
}

bool FishingBobberEntity::isInOpenWater() const
{
    const auto* c = tryGetComponent<ecs::FishingBobberComponent>();
    return (c != nullptr) ? c->m_inOpenWater : false;
}

// ============================================================================
// ShulkerBulletEntity
// ============================================================================

ShulkerBulletEntity::ShulkerBulletEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
{
    setNoGravity(true);
    m_noClip = true; // 穿墙
    // 批次6 子目标2 Step1：attach ShulkerBulletComponent（目标/方向/步数/增量 5 字段）。
    // Step4 已把 m_target/m_targetUuid/m_direction/m_flightSteps/m_targetDelta 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::ShulkerBulletComponent>(m_entityContext->entity());
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

    setTarget(target);

    auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    if (bullet != nullptr) {
        bullet->m_direction = Direction::Up;
    }
    _selectNextMoveDirection(axis);
}

std::unique_ptr<Entity> ShulkerBulletEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ShulkerBulletEntity>(0, registry);
}

void ShulkerBulletEntity::setTarget(Entity* target)
{
    auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    if (bullet == nullptr) {
        return;
    }
    bullet->m_target = target;
    if (target) {
        bullet->m_targetUuid = target->uuid();
    }
}

// 批次6 子目标2 Step4：m_target/m_targetUuid/m_direction/m_flightSteps/m_targetDelta
// 迁入 ecs::ShulkerBulletComponent。
Entity* ShulkerBulletEntity::target() const
{
    const auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    return (bullet != nullptr) ? bullet->m_target : nullptr;
}

Direction ShulkerBulletEntity::direction() const
{
    const auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    return (bullet != nullptr) ? bullet->m_direction : Direction::Up;
}

void ShulkerBulletEntity::_setDirection(Direction dir)
{
    auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    if (bullet != nullptr) {
        bullet->m_direction = dir;
    }
}

void ShulkerBulletEntity::tick()
{
    auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    const Entity* target = (bullet != nullptr) ? bullet->m_target : nullptr;

    // 服务端逻辑
    if (m_world != nullptr) {
        // 检查目标是否有效
        Player* playerTarget = dynamic_cast<Player*>(const_cast<Entity*>(target));
        if (target == nullptr || !target->isAlive() || (playerTarget != nullptr && playerTarget->isSpectator())) {
            // 目标无效，下落
            if (!hasNoGravity()) {
                m_builtIn.velocity->m_velocity.y -= 0.04;
            }
        } else {
            // 加速追踪
            bullet->m_targetDelta.x = std::clamp(bullet->m_targetDelta.x * ACCELERATION, -1.0, 1.0);
            bullet->m_targetDelta.y = std::clamp(bullet->m_targetDelta.y * ACCELERATION, -1.0, 1.0);
            bullet->m_targetDelta.z = std::clamp(bullet->m_targetDelta.z * ACCELERATION, -1.0, 1.0);

            // 向目标方向加速
            m_builtIn.velocity->m_velocity.x += (bullet->m_targetDelta.x - m_builtIn.velocity->m_velocity.x) * 0.2;
            m_builtIn.velocity->m_velocity.y += (bullet->m_targetDelta.y - m_builtIn.velocity->m_velocity.y) * 0.2;
            m_builtIn.velocity->m_velocity.z += (bullet->m_targetDelta.z - m_builtIn.velocity->m_velocity.z) * 0.2;
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
    if (m_world != nullptr && target != nullptr && target->isAlive() && bullet != nullptr) {
        // 更新飞行步数
        if (bullet->m_flightSteps > 0) {
            bullet->m_flightSteps--;

            // 步数用完时重新选择方向
            if (bullet->m_flightSteps == 0) {
                Axis excludeAxis =
                    (bullet->m_direction != Direction::None) ? Directions::getAxis(bullet->m_direction) : Axis::Y;
                _selectNextMoveDirection(excludeAxis);
            }
        }

        // 检查是否需要改变方向
        if (bullet->m_direction != Direction::None) {
            BlockPos currentPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
            Axis axis = Directions::getAxis(bullet->m_direction);

            // 检查前方是否有方块
            BlockPos nextPos(currentPos.x + Directions::xOffset(bullet->m_direction),
                currentPos.y + Directions::yOffset(bullet->m_direction),
                currentPos.z + Directions::zOffset(bullet->m_direction));

            const BlockState* nextState = m_world->getBlockState(nextPos);
            if (nextState != nullptr && nextState->blocksMovement()) {
                // 前方有方块，选择新方向
                _selectNextMoveDirection(axis);
            } else {
                // 检查是否与目标对齐
                BlockPos targetPos(static_cast<i32>(std::floor(target->x())),
                    static_cast<i32>(std::floor(target->y())),
                    static_cast<i32>(std::floor(target->z())));
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
    auto* bullet = tryGetComponent<ecs::ShulkerBulletComponent>();
    const Entity* target = (bullet != nullptr) ? bullet->m_target : nullptr;

    f64 targetOffsetY = 0.5;
    BlockPos targetPos;

    if (target == nullptr) {
        targetPos = BlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)) - 1,
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    } else {
        targetOffsetY = static_cast<f64>(target->height()) * 0.5;
        targetPos = BlockPos(static_cast<i32>(std::floor(target->x())),
            static_cast<i32>(std::floor(target->y() + targetOffsetY)),
            static_cast<i32>(std::floor(target->z())));
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

    if (bullet != nullptr) {
        if (dist == 0.0) {
            bullet->m_targetDelta = Vector3d(0.0, 0.0, 0.0);
        } else {
            bullet->m_targetDelta =
                Vector3d(dx / dist * BULLET_SPEED, dy / dist * BULLET_SPEED, dz / dist * BULLET_SPEED);
        }

        // 设置飞行步数
        if (m_world != nullptr) {
            math::Random& rng = m_world->getRandom();
            bullet->m_flightSteps = MIN_STEPS + rng.nextInt(MAX_STEPS_EXTRA) * 10;
        }
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
    // 批次6 子目标2 Step1：attach EvokerFangsComponent（owner/预热/生命 6 字段）。
    // EvokerFangsEntity 直接继承 Entity 不经 ProjectileEntity，独立 owner（唤魔者），
    // 故不挂 ProjectileOwnerComponent，本组件独立承载 owner。Step2 已把 m_owner/
    // m_ownerUuid/m_warmupDelay/m_sentAttackEvent/m_lifeTicks/m_clientSideAttackStarted
    // 读写改走组件（字段声明已删）。
    m_entityContext->enttRegistry().emplace<ecs::EvokerFangsComponent>(m_entityContext->entity());
}

std::unique_ptr<Entity> EvokerFangsEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EvokerFangsEntity>(0, registry);
}

void EvokerFangsEntity::setOwner(LivingEntity* owner)
{
    auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    MC_ASSERT_RELEASE(comp != nullptr);
    if (comp == nullptr) {
        return;
    }
    comp->m_owner = owner;
    if (owner != nullptr) {
        comp->m_ownerUuid = owner->uuid();
    } else {
        comp->m_ownerUuid.clear();
    }
}

LivingEntity* EvokerFangsEntity::getOwner()
{
    auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    if (comp == nullptr) {
        return nullptr;
    }
    // 如果缓存指针有效且实体未被移除，直接返回
    if (comp->m_owner != nullptr && comp->m_owner->isAlive()) {
        return comp->m_owner;
    }

    // 缓存失效，尝试通过 UUID 在世界中重新查找
    // 使用 IWorld::getEntityByUuid() 进行 O(1) 查找
    if (!comp->m_ownerUuid.empty() && m_world != nullptr) {
        Entity* entity = m_world->getEntityByUuid(comp->m_ownerUuid);
        if (entity != nullptr && entity->isAlive()) {
            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living != nullptr) {
                comp->m_owner = living;
                return comp->m_owner;
            }
        }
    }

    // UUID 查找也失败，清空缓存指针
    comp->m_owner = nullptr;
    return nullptr;
}

void EvokerFangsEntity::setOwnerUuid(const std::string& uuid)
{
    auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    MC_ASSERT_RELEASE(comp != nullptr);
    if (comp == nullptr) {
        return;
    }
    comp->m_ownerUuid = uuid;
    // 不设置 m_owner 指针，等到 getOwner() 被调用时再通过 UUID 懒加载查找
    comp->m_owner = nullptr;
}

LivingEntity* EvokerFangsEntity::owner() const
{
    const auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    return (comp != nullptr) ? comp->m_owner : nullptr;
}

const std::string& EvokerFangsEntity::ownerUuid() const
{
    static const std::string s_empty;
    const auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    return (comp != nullptr) ? comp->m_ownerUuid : s_empty;
}

void EvokerFangsEntity::setWarmupDelay(i32 delay)
{
    auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    MC_ASSERT_RELEASE(comp != nullptr);
    if (comp == nullptr) {
        return;
    }
    comp->m_warmupDelay = delay;
}

i32 EvokerFangsEntity::warmupDelay() const
{
    const auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    return (comp != nullptr) ? comp->m_warmupDelay : 0;
}

void EvokerFangsEntity::tick()
{
    Entity::tick();

    auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    if (comp == nullptr) {
        return;
    }

    // 服务端逻辑
    if (comp->m_warmupDelay > 0) {
        comp->m_warmupDelay--;
    } else {
        // warmupDelayTicks < 0 后开始攻击逻辑
        if (comp->m_warmupDelay == -8) {
            // 在 -8 tick 时对范围内实体造成伤害
            _damageEntities();
        }

        if (!comp->m_sentAttackEvent) {
            // 发送攻击事件给客户端（用于播放音效和粒子）
            comp->m_sentAttackEvent = true;
        }

        comp->m_lifeTicks--;
        if (comp->m_lifeTicks < 0) {
            remove();
        }
    }
}

f32 EvokerFangsEntity::getAnimationProgress(f32 partialTicks) const
{
    const auto* comp = tryGetComponent<ecs::EvokerFangsComponent>();
    if (comp == nullptr) {
        return 0.0f;
    }
    if (!comp->m_clientSideAttackStarted) {
        return 0.0f;
    } else {
        i32 i = comp->m_lifeTicks - 2;
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

void EvokerFangsEntity::addAdditionalSaveData(nbt::tags::compound_tag& /*tag*/) const
{
    // 批次6 子目标2 Step6：EvokerFangs 持久化（Warmup + Owner UUID）已搬至按组件注册的
    // 序列化器（ProjectileComponentSerialization.cpp 的 saveEvokerFangs/loadEvokerFangs），
    // 经 ComponentSerializerRegistry::saveAll/loadAll 调用。此 override 保留空壳避免子类
    // 回落到 Entity 基类后再被未来代码误加字段（与 FireworkRocket/Spear 同范式）。
}

Result<void> EvokerFangsEntity::readAdditionalSaveData(const nbt::tags::compound_tag& /*tag*/)
{
    // 持久化已搬注册表，此 override 空实现。
    return {};
}

// ============================================================================
// EyeOfEnderEntity
// ============================================================================

// ============================================================================
// EyeOfEnderEntity
// ============================================================================

// 静态数据参数定义（对应 MC 1.21.11 EyeOfEnder.defineSynchedData() 的 DATA_ITEM_STACK）。
// EyeOfEnderEntity 直接继承 Entity，真实 id 在 registerData() 内由 ClassRegisterGuard
// 沿继承链续接分配（Entity 8 字段后 → id8）。
entity::DataParameter<network::ir::play::ItemStackView> EyeOfEnderEntity::DATA_ITEM_STACK_PARAM =
    entity::EntityDataManager::createKey<network::ir::play::ItemStackView>();

const EntityClassInfo& EyeOfEnderEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"EyeOfEnderEntity", &Entity::classInfo()};
    return s_classInfo;
}

void EyeOfEnderEntity::registerData()
{
    // 先调基类注册基础参数（FLAGS/AIR/CUSTOM_NAME 等，id0..7）
    Entity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册末影之眼专属同步参数（id8，对应 vanilla EyeOfEnder.DATA_ITEM_STACK）。
    // TODO: 项目 EyeOfEnderEntity 当前无 item 字段，占位空 ItemStackView 保持 wire 位置对齐；
    // 补齐物品字段后镜像真实物品。
    m_dataManager.registerParam(DATA_ITEM_STACK_PARAM, network::ir::toItemStackView(ItemStack()));
}

EyeOfEnderEntity::EyeOfEnderEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    setNoGravity(false);
    // 批次6 子目标2 Step1：attach EyeOfEnderComponent（目标XZ/存活/碎裂 4 字段）。
    // EyeOfEnderEntity 直接继承 Entity 不经 ProjectileEntity，故不挂 ProjectileOwnerComponent。
    // Step4 将把 m_targetX/m_targetZ/m_lifetime/m_break 读写改走组件；Step5 补 DATA_ITEM_STACK。
    m_entityContext->enttRegistry().emplace<ecs::EyeOfEnderComponent>(m_entityContext->entity());
    // C++ 虚函数在构造函数中不会派生到子类，故 Entity 基类构造调用的 registerData()
    // 只执行 Entity::registerData()。此处显式调用注册末影之眼专属同步字段（id8）。
    registerData();
}

std::unique_ptr<Entity> EyeOfEnderEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EyeOfEnderEntity>(0, registry);
}

void EyeOfEnderEntity::tick()
{
    Entity::tick();

    auto* eye = tryGetComponent<ecs::EyeOfEnderComponent>();
    if (eye == nullptr) {
        return;
    }
    ++eye->m_lifetime;

    // 向目标移动
    if (eye->m_targetX != 0 || eye->m_targetZ != 0) {
        // 计算方向
        f32 dx = static_cast<f32>(eye->m_targetX) - m_builtIn.stateVector->m_pos.x;
        f32 dz = static_cast<f32>(eye->m_targetZ) - m_builtIn.stateVector->m_pos.z;
        f32 dist = std::sqrt(dx * dx + dz * dz);

        if (dist > 0.0f) {
            // 设置速度
            m_builtIn.velocity->m_velocity.x = dx / dist * 0.5f;
            m_builtIn.velocity->m_velocity.z = dz / dist * 0.5f;

            // Y轴波动
            m_builtIn.velocity->m_velocity.y = std::sin(eye->m_lifetime * 0.1f) * 0.1f;
        }
    }

    // 随机碎裂
    // 15% 概率碎裂
    // if (rand.nextInt(700) == 0) {
    //     eye->m_break = true;
    //     remove();
    // }

    // 超时移除
    if (eye->m_lifetime > 1200) { // 60秒
        remove();
    }
}

void EyeOfEnderEntity::moveTo(BlockCoord targetX, BlockCoord targetZ)
{
    auto* eye = tryGetComponent<ecs::EyeOfEnderComponent>();
    if (eye != nullptr) {
        eye->m_targetX = targetX;
        eye->m_targetZ = targetZ;
    }
}

// 批次6 子目标2 Step4：m_targetX/m_targetZ/m_break 迁入 ecs::EyeOfEnderComponent。
BlockCoord EyeOfEnderEntity::targetX() const
{
    const auto* c = tryGetComponent<ecs::EyeOfEnderComponent>();
    return (c != nullptr) ? c->m_targetX : 0;
}

BlockCoord EyeOfEnderEntity::targetZ() const
{
    const auto* c = tryGetComponent<ecs::EyeOfEnderComponent>();
    return (c != nullptr) ? c->m_targetZ : 0;
}

bool EyeOfEnderEntity::shouldBreak() const
{
    const auto* c = tryGetComponent<ecs::EyeOfEnderComponent>();
    return (c != nullptr) ? c->m_break : false;
}

// ============================================================================
// FireworkRocketEntity
// ============================================================================

// 静态数据参数定义（对应 MC 1.21.11 FireworkRocket.defineSynchedData()）。
// 真实 id 在 registerData() 内由 ClassRegisterGuard 沿继承链续接分配
// （Entity 8 字段后 → id8/9/10，ProjectileEntity 无同步字段不占 id）。
entity::DataParameter<network::ir::play::ItemStackView> FireworkRocketEntity::DATA_FIREWORKS_ITEM_PARAM =
    entity::EntityDataManager::createKey<network::ir::play::ItemStackView>();
entity::DataParameter<i32> FireworkRocketEntity::DATA_ATTACHED_TO_TARGET_PARAM =
    entity::EntityDataManager::createKey<i32>();
entity::DataParameter<bool> FireworkRocketEntity::DATA_SHOT_AT_ANGLE_PARAM =
    entity::EntityDataManager::createKey<bool>();

const EntityClassInfo& FireworkRocketEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"FireworkRocketEntity", &ProjectileEntity::classInfo()};
    return s_classInfo;
}

void FireworkRocketEntity::registerData()
{
    // 先调基类注册基础参数（FLAGS/AIR/CUSTOM_NAME 等，id0..7）
    ProjectileEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册烟花火箭专属同步参数（id8/9/10，对应 vanilla FireworkRocket）。
    m_dataManager.registerParam(DATA_FIREWORKS_ITEM_PARAM, network::ir::toItemStackView(ItemStack()));
    // DATA_ATTACHED_TO_TARGET：vanilla OptionalInt，absent=0/present=id+1。项目无附着目标机制，
    // 占位 -1 表 absent（TODO 待补 OptionalInt 序列化器后改精确编码）。
    m_dataManager.registerParam(DATA_ATTACHED_TO_TARGET_PARAM, static_cast<i32>(-1));
    m_dataManager.registerParam(DATA_SHOT_AT_ANGLE_PARAM, false);
}

FireworkRocketEntity::FireworkRocketEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
{
    // 批次6 子目标2 Step1：attach FireworkRocketComponent（烟花物品/飞行/生命 5 字段）。
    // Step4 将把 m_fireworkItem/m_flightTime/m_lifetime/m_lifeTime/m_shotFromCrossbow 读写
    // 改走组件；Step5 补 DATA_FIREWORKS_ITEM/DATA_ATTACHED_TO_TARGET/DATA_SHOT_AT_ANGLE 同步字段。
    m_entityContext->enttRegistry().emplace<ecs::FireworkRocketComponent>(m_entityContext->entity());
    // C++ 虚函数在构造函数中不会派生到子类，故 ProjectileEntity 构造调用的 registerData()
    // 不会派生到 FireworkRocketEntity::registerData()。此处显式调用注册烟花专属同步字段（id8/9/10）。
    registerData();
}

std::unique_ptr<Entity> FireworkRocketEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<FireworkRocketEntity>(0, registry);
}

void FireworkRocketEntity::setFireworkItem(const ItemStack& item)
{
    auto* fw = tryGetComponent<ecs::FireworkRocketComponent>();
    if (fw == nullptr || fw->m_fireworkItem == nullptr) {
        return;
    }
    *fw->m_fireworkItem = item;

    // 从物品 NBT 读取飞行时间
    const nlohmann::json* tag = item.getTag();
    if (tag != nullptr) {
        auto fireworksIt = tag->find("Fireworks");
        if (fireworksIt != tag->end() && fireworksIt->is_object()) {
            auto flightIt = fireworksIt->find("Flight");
            if (flightIt != fireworksIt->end() && flightIt->is_number()) {
                fw->m_flightTime = std::max(1, flightIt->get<i32>());
            }
        }
    }

    // 物品变更后，已计算的 lifeTime 失效，需重新懒初始化
    // （NBT 反序列化路径会在 setFireworkItem 之后显式 setLifeTime 覆盖此处的重置）
    fw->m_lifeTime = -1;

    // 批次6 子目标2 Step5：镜像同步 DATA_FIREWORKS_ITEM（对齐 vanilla FireworkRocket）。
    m_dataManager.set(DATA_FIREWORKS_ITEM_PARAM, network::ir::toItemStackView(item));
}

void FireworkRocketEntity::_ensureLifeTimeComputed()
{
    auto* fw = tryGetComponent<ecs::FireworkRocketComponent>();
    if (fw == nullptr || fw->m_lifeTime >= 0) {
        return; // 已计算或已从 NBT 恢复
    }

    // lifeTime = flightTime * 10 + rand.nextInt(6) + rand.nextInt(7)
    // 使用世界随机数生成器一次性确定，保证服务端确定性；
    // 客户端不跑 FireworkRocketEntity::tick，无需此值
    if (m_world != nullptr && !m_world->isClientSide()) {
        math::Random& rng = m_world->getRandom();
        fw->m_lifeTime = fw->m_flightTime * 10 + rng.nextInt(6) + rng.nextInt(7);
    }
}

i32 FireworkRocketEntity::getExplosionCount() const
{
    const auto* fw = tryGetComponent<ecs::FireworkRocketComponent>();
    if (fw == nullptr || fw->m_fireworkItem == nullptr) {
        return 0;
    }
    // 从物品 NBT 读取爆炸效果数量
    const nlohmann::json* tag = fw->m_fireworkItem->getTag();
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

    auto* fw = tryGetComponent<ecs::FireworkRocketComponent>();
    if (fw == nullptr) {
        return;
    }
    ++fw->m_lifetime;

    // 生成飞行粒子（每 2 tick 生成一次粒子，仅在客户端执行）
    if (m_world != nullptr && m_world->isClientSide() && fw->m_lifetime % 2 == 0) {
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
    const i32 explodeThreshold = (fw->m_lifeTime >= 0) ? fw->m_lifeTime : (fw->m_flightTime * 10 + 6);
    if (fw->m_lifetime >= explodeThreshold) {
        _explode();
    }
}

void FireworkRocketEntity::_explode()
{
    auto* fw = tryGetComponent<ecs::FireworkRocketComponent>();

    // 如果从弩射出，对周围实体造成伤害
    if (fw != nullptr && fw->m_shotFromCrossbow) {
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

void FireworkRocketEntity::addAdditionalSaveData(nbt::tags::compound_tag& /*tag*/) const
{
    // 批次6 子目标2 Step6：FireworkRocket 持久化（Life/LifeTime/FireworksItem/ShotAtAngle）已搬至
    // 按组件注册的序列化器（ProjectileComponentSerialization.cpp 的 saveFireworkRocket/loadFireworkRocket），
    // 经 ComponentSerializerRegistry::saveAll/loadAll 调用。此 override 保留空壳。
}

Result<void> FireworkRocketEntity::readAdditionalSaveData(const nbt::tags::compound_tag& /*tag*/)
{
    // 持久化已搬注册表，此 override 空实现。
    return {};
}

// 批次6 子目标2 Step4：FireworkRocket 5 字段经 ecs::FireworkRocketComponent 读写。
const ItemStack& FireworkRocketEntity::fireworkItem() const
{
    static const ItemStack s_empty;
    const auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    return (c != nullptr && c->m_fireworkItem != nullptr) ? *c->m_fireworkItem : s_empty;
}

bool FireworkRocketEntity::shotFromCrossbow() const
{
    const auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    return (c != nullptr) ? c->m_shotFromCrossbow : false;
}

void FireworkRocketEntity::setShotFromCrossbow(bool value)
{
    auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    if (c != nullptr) {
        c->m_shotFromCrossbow = value;
    }
    // 批次6 子目标2 Step5：镜像同步 DATA_SHOT_AT_ANGLE（对齐 vanilla FireworkRocket）。
    m_dataManager.set(DATA_SHOT_AT_ANGLE_PARAM, value);
}

i32 FireworkRocketEntity::flightTime() const
{
    const auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    return (c != nullptr) ? c->m_flightTime : 1;
}

void FireworkRocketEntity::setFlightTime(i32 time)
{
    auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    if (c != nullptr) {
        c->m_flightTime = time;
    }
}

i32 FireworkRocketEntity::lifeTime() const
{
    const auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    return (c != nullptr) ? c->m_lifeTime : -1;
}

void FireworkRocketEntity::setLifeTime(i32 time)
{
    auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    if (c != nullptr) {
        c->m_lifeTime = time;
    }
}

i32 FireworkRocketEntity::lifetime() const
{
    const auto* c = tryGetComponent<ecs::FireworkRocketComponent>();
    return (c != nullptr) ? c->m_lifetime : 0;
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
