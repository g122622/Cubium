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

#include "EffectEntities.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/items/special/FlintAndSteelItem.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/UuidUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/explosion/ExplosionImmunityContext.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../damage/tag/DamageTypeTags.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "../../serialization/EntityNbtKeys.hpp"
#include "../boss/EnderDragonEntity.hpp"
#include "../player/Player.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace entity {

namespace {

/**
 * @brief 应用瞬间效果到目标实体
 *
 * 用于药水云、喷溅药水等场景中瞬间效果的应用。
 * 瞬间效果（治疗、伤害、饱和）使用 affectEntity 模式而非 addEffect。
 *
 * @param type 效果类型（必须是瞬间效果）
 * @param source 效果来源实体（AreaEffectCloud 自身）
 * @param caster 效果施放者（拥有者，如投掷药水的玩家）
 * @param target 目标生物
 * @param amplifier 效果等级（0 = I, 1 = II）
 * @param multiplier 效果乘数（通常为 0.5 或 1.0）
 */
void applyInstantEffect(
    effect::EffectType type, Entity& source, LivingEntity* caster, LivingEntity& target, i32 amplifier, f32 multiplier)
{
    // 基础值 4.0，每级增加 2.0
    f32 amount = (4.0f + static_cast<f32>(amplifier) * 2.0f) * multiplier;

    switch (type) {
        case effect::EffectType::InstantHealth:
            // 瞬间治疗：亡灵生物受到伤害，普通生物治疗
            if (target.getCreatureAttribute() == CreatureAttribute::Undead) {
                // 伤害归属于 caster（如果存在），使击杀归因正确
                if (caster != nullptr) {
                    auto dmgSource = DamageSources::indirectMagic(&source, caster);
                    target.hurt(dmgSource, amount);
                } else {
                    auto dmgSource = DamageSources::magic();
                    target.hurt(dmgSource, amount);
                }
            } else {
                target.heal(amount);
            }
            break;

        case effect::EffectType::InstantDamage:
            // 瞬间伤害：亡灵生物治疗，普通生物受到伤害
            if (target.getCreatureAttribute() == CreatureAttribute::Undead) {
                target.heal(amount);
            } else {
                if (caster != nullptr) {
                    auto dmgSource = DamageSources::indirectMagic(&source, caster);
                    target.hurt(dmgSource, amount);
                } else {
                    auto dmgSource = DamageSources::magic();
                    target.hurt(dmgSource, amount);
                }
            }
            break;

        case effect::EffectType::Saturation:
            // 饱和效果：恢复饥饿值和饱和度（仅对玩家有效）
            if (auto* player = dynamic_cast<Player*>(&target)) {
                i32 nutrition = amplifier + 1;
                player->foodStats().addStats(nutrition, 1.0f);
            }
            break;

        default:
            // 非瞬间效果不做处理
            break;
    }
}

} // namespace

// ==================== EnderCrystalEntity ====================

EnderCrystalEntity::EnderCrystalEntity(ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
{}

std::unique_ptr<Entity> EnderCrystalEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EnderCrystalEntity>(registry);
}

f32 EnderCrystalEntity::width() const
{
    return 2.0f;
}

f32 EnderCrystalEntity::height() const
{
    return 2.0f;
}

void EnderCrystalEntity::tick()
{
    Entity::tick();

    // 递增内部旋转计数器（用于渲染动画）
    ++m_innerRotation;

    // 治愈末影龙冷却
    if (m_healCooldown > 0) {
        m_healCooldown--;
    }

    // 服务端在末地且存在 DragonFightManager 时，在脚下放置火焰
    // 注意：末地水晶不生成粒子，光束效果由渲染器处理
}

bool EnderCrystalEntity::hasBeamTarget() const
{
    return m_beamTarget.x != 0 || m_beamTarget.y != 0 || m_beamTarget.z != 0;
}

void EnderCrystalEntity::setBeamTarget(BlockPos pos)
{
    m_beamTarget = pos;
}

void EnderCrystalEntity::healDragon()
{
    // 检查冷却
    if (m_healCooldown > 0) {
        return;
    }

    // 获取世界
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 在 32 格范围内搜索末影龙
    constexpr f32 HEAL_RANGE = 32.0f;
    constexpr f32 HEAL_RANGE_SQ = HEAL_RANGE * HEAL_RANGE;

    // 获取水晶位置
    Vector3 crystalPos(x(), y(), z());

    // 获取范围内的实体
    std::vector<Entity*> entities = worldPtr->getEntitiesInRange(crystalPos, HEAL_RANGE, this);

    EnderDragonEntity* nearestDragon = nullptr;
    f32 nearestDistSq = HEAL_RANGE_SQ;

    // 搜索最近的末影龙
    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // 检查是否为末影龙
        if (entity->entityType() == entity::VanillaEntityTypeKeys::ENDER_DRAGON) {
            f32 dx = static_cast<f32>(entity->x() - x());
            f32 dy = static_cast<f32>(entity->y() - y());
            f32 dz = static_cast<f32>(entity->z() - z());
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestDragon = static_cast<EnderDragonEntity*>(entity);
            }
        }
    }

    // 如果找到末影龙
    if (nearestDragon != nullptr && nearestDragon->isAlive()) {
        // 治愈末影龙 1 点生命值
        nearestDragon->heal(1.0f);

        // 设置冷却时间
        m_healCooldown = HEAL_COOLDOWN;

        // 设置光束指向末影龙（用于渲染）
        setBeamTarget(BlockPos(static_cast<BlockCoord>(nearestDragon->x()),
            static_cast<BlockCoord>(nearestDragon->y()),
            static_cast<BlockCoord>(nearestDragon->z())));

        // 设置龙的最近水晶引用（建立双向关联）
        nearestDragon->setClosestEnderCrystal(this);
    }
}

void EnderCrystalEntity::explode()
{
    // 末地水晶爆炸，爆炸半径 6.0，模式 DESTROY（破坏方块并掉落物品）
    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        worldPtr->createExplosion(m_builtIn.stateVector->m_pos,
            6.0f, // 爆炸半径
            world::explosion::ExplosionMode::Destroy,
            false,  // 不生成火焰
            nullptr // 无爆炸源实体
        );
    }
    remove();
}

bool EnderCrystalEntity::hurt(DamageSource& source, f32 /*amount*/)
{
    // 对应 MC 1.21.11 EndCrystal.hurtServer()
    // 1. 无敌伤害直接拒绝
    if (isInvulnerableTo(source)) {
        return false;
    }
    // 2. 末影龙造成的伤害无效（避免龙误伤自己的水晶）
    Entity* attacker = source.getEntity();
    if (attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::ENDER_DRAGON) {
        return false;
    }
    // 3. 仅在尚未移除时执行破坏流程（防止递归爆炸重复触发）
    if (isRemoved()) {
        return true;
    }
    discard();
    // 4. 若伤害来源不是爆炸，则触发一次破坏性爆炸（避免被爆炸炸毁时再次爆炸）
    if (!source.isExplosion()) {
        IWorld* worldPtr = world();
        if (worldPtr != nullptr) {
            // 使用当前水晶作为爆炸源，伤害来源作为造成者（用于击杀归因）
            // MC: level.explode(this, damagesource, null, x, y, z, 6.0F, false, BLOCK)
            worldPtr->createExplosion(m_builtIn.stateVector->m_pos,
                EXPLOSION_RADIUS,
                world::explosion::ExplosionMode::Break,
                false, // 不生成火焰
                this);
        }
    }
    // 5. 通知末影龙战斗系统（中止重生 / 更新水晶计数 / 触发龙息等）
    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        EndDragonFight* fight = worldPtr->dragonFight();
        if (fight != nullptr) {
            fight->onCrystalDestroyed(*worldPtr, this, source);
        }
    }
    return true;
}

// ==================== LightningBoltEntity ====================

LightningBoltEntity::LightningBoltEntity(ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
{
    // 闪电总是可见，即使不在视锥内
}

std::unique_ptr<Entity> LightningBoltEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<LightningBoltEntity>(registry);
}

f32 LightningBoltEntity::width() const
{
    return 0.0f; // 闪电没有碰撞箱
}

f32 LightningBoltEntity::height() const
{
    return 0.0f; // 闪电没有碰撞箱
}

void LightningBoltEntity::_initializeState()
{
    // 初始化闪电状态
    m_lightningState = 2;

    // 使用世界种子或随机数生成 boltVertex
    if (m_world != nullptr) {
        math::Random rng(static_cast<u64>(m_world->currentTick()) ^ m_world->seed());
        m_boltVertex = rng.nextLong();
        m_boltLivingTime = rng.nextInt(1, 3); // 1-3
    } else {
        // 无世界时使用确定性种子（基于时间）
        math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
        m_boltVertex = rng.nextLong();
        m_boltLivingTime = rng.nextInt(1, 3);
    }

    m_initialized = true;
}

void LightningBoltEntity::tick()
{
    Entity::tick();

    // 首次 tick 初始化状态
    if (!m_initialized) {
        _initializeState();
    }

    // lightningState == 2 时执行初始效果：播放音效、点燃方块、造成伤害
    if (m_lightningState == 2) {
        // 难度检查 - NORMAL 和 HARD 点燃更多火焰
        if (m_world != nullptr && !m_effectOnly && !m_world->isClientSide()) {
            Difficulty difficulty = m_world->difficulty();
            if (difficulty == Difficulty::Normal || difficulty == Difficulty::Hard) {
                _igniteBlocks(4);
            } else {
                _igniteBlocks(0);
            }
        }

        // 播放雷声音效，音量 10000（非常大的范围），音调 0.8-1.0
        if (m_world != nullptr) {
            // 使用 boltVertex 生成一致的随机音调
            f32 thunderPitch = 0.8f + static_cast<f32>(m_boltVertex % 100) / 100.0f * 0.2f;
            m_world->playSound(SoundEvents::WEATHER_THUNDER,
                sound::SoundCategory::Weather,
                m_builtIn.stateVector->m_pos,
                10000.0f, // 音量（可传很远）
                thunderPitch);

            // 播放雷击声音效（音量 2，音调 0.5-0.7）
            f32 impactPitch = 0.5f + static_cast<f32>((m_boltVertex >> 8) % 100) / 100.0f * 0.2f;
            m_world->playSound(SoundEvents::WEATHER_THUNDER,
                sound::SoundCategory::Weather,
                m_builtIn.stateVector->m_pos,
                2.0f,
                impactPitch);
        }

        // 服务端造成伤害（非 effectOnly，非客户端）
        if (m_world != nullptr && !m_world->isClientSide() && !m_effectOnly) {
            _damageEntities();
        }

        // 客户端设置闪电闪烁效果
        if (m_world != nullptr && m_world->isClientSide()) {
            m_world->setTimeLightningFlash(2);
        }
    }

    // 递减 lightningState
    --m_lightningState;

    // lightningState < 0 时检查是否"复活"
    if (m_lightningState < 0) {
        if (m_boltLivingTime == 0) {
            // 所有视觉效果结束，移除实体
            remove();
        } else if (m_lightningState < -static_cast<i32>(m_boltVertex % 10)) {
            // 随机间隔后"复活"，闪电会多次闪烁，模拟真实闪电效果
            --m_boltLivingTime;
            m_lightningState = 1;

            // 生成新的随机种子用于渲染
            if (m_world != nullptr) {
                math::Random rng(static_cast<u64>(m_world->currentTick()) ^ m_boltVertex);
                m_boltVertex = rng.nextLong();
            } else {
                math::Random rng(
                    static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()) ^ m_boltVertex);
                m_boltVertex = rng.nextLong();
            }

            // "复活"时再次尝试点燃（不额外点燃）
            _igniteBlocks(0);
        }
    }
}

void LightningBoltEntity::_igniteBlocks(i32 extraIgnitions)
{
    // 检查游戏规则 doFireTick 和是否为客户端
    if (m_effectOnly || m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    if (!m_world->doFireTick()) {
        return;
    }

    // 获取当前位置
    BlockPos blockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));

    // 在当前位置放置火焰
    const BlockState* currentState = m_world->getBlockState(blockPos);
    if (currentState != nullptr && currentState->isAir()) {
        // 检查下方方块决定火焰类型（灵魂火或普通火）
        Block* fireBlock = item::tool::FlintAndSteelItem::getFireForPlacement(*m_world, blockPos);
        if (fireBlock != nullptr) {
            const BlockState& fireState = fireBlock->defaultState();
            // 检查火焰是否可以放置在当前位置
            IBlockReader& blockReader = static_cast<IBlockReader&>(*m_world);
            if (fireBlock->isValidPosition(fireState, blockReader, blockPos)) {
                m_world->setBlockState(blockPos, &fireState);
            }
        }
    }

    // 额外点燃周围方块
    if (extraIgnitions > 0) {
        math::Random rng(m_boltVertex);

        for (i32 i = 0; i < extraIgnitions; ++i) {
            i32 dx = rng.nextInt(3) - 1;
            i32 dy = rng.nextInt(3) - 1;
            i32 dz = rng.nextInt(3) - 1;

            BlockPos firePos(blockPos.x + dx, blockPos.y + dy, blockPos.z + dz);

            const BlockState* stateAtPos = m_world->getBlockState(firePos);
            if (stateAtPos != nullptr && stateAtPos->isAir()) {
                Block* fireBlock = item::tool::FlintAndSteelItem::getFireForPlacement(*m_world, firePos);
                if (fireBlock != nullptr) {
                    const BlockState& fireState = fireBlock->defaultState();
                    IBlockReader& blockReader = static_cast<IBlockReader&>(*m_world);
                    if (fireBlock->isValidPosition(fireState, blockReader, firePos)) {
                        m_world->setBlockState(firePos, &fireState);
                    }
                }
            }
        }
    }
}

void LightningBoltEntity::_damageEntities()
{
    // 获取 3x6x3 范围内的实体
    if (m_world == nullptr || m_effectOnly) {
        return;
    }

    // 构建碰撞箱
    AxisAlignedBB box(m_builtIn.stateVector->m_pos.x - DAMAGE_RADIUS_XZ,
        m_builtIn.stateVector->m_pos.y - DAMAGE_RADIUS_Y_OFFSET,
        m_builtIn.stateVector->m_pos.z - DAMAGE_RADIUS_XZ,
        m_builtIn.stateVector->m_pos.x + DAMAGE_RADIUS_XZ,
        m_builtIn.stateVector->m_pos.y + DAMAGE_RADIUS_Y + DAMAGE_RADIUS_Y_OFFSET,
        m_builtIn.stateVector->m_pos.z + DAMAGE_RADIUS_XZ);

    // 获取范围内的实体
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(box, this);

    // 收集被击中的实体用于引雷附魔进度触发
    std::vector<Entity*> victims;

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // 对于 LivingEntity，造成闪电伤害
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living != nullptr) {
            // 创建闪电伤害来源
            auto damageSource = DamageSources::lightningBolt(this);
            // 闪电伤害为 5.0
            living->hurt(damageSource, 5.0f);
        }

        // 调用实体的 onStruckByLightning() 方法
        // 用于处理特殊效果（如哞菇变色、苦力怕充能等）
        entity->onStruckByLightning();

        // 收集被击中的实体用于引雷附魔进度触发
        victims.push_back(entity);
    }

    // 触发进度 CriteriaTriggers.CHANNELED_LIGHTNING
    // 如果有 caster（引雷附魔的玩家），通过 IWorld 发布事件
    if (m_caster != 0 && !victims.empty() && m_world != nullptr) {
        m_world->onChanneledLightning(m_caster, victims);
    }
}

// ==================== AreaEffectCloudEntity ====================

AreaEffectCloudEntity::AreaEffectCloudEntity(ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
{
    // 药水云无碰撞
    setNoClip(true);
}

std::unique_ptr<Entity> AreaEffectCloudEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<AreaEffectCloudEntity>(registry);
}

f32 AreaEffectCloudEntity::width() const
{
    return m_radius * 2.0f; // 实际宽度是半径的两倍
}

f32 AreaEffectCloudEntity::height() const
{
    return 0.5f; // 药水云高度固定为 0.5
}

void AreaEffectCloudEntity::setRadius(f32 radius)
{
    m_radius = radius;
    m_initialRadius = radius;
    // 宽度随半径变化，需要刷新碰撞箱
    refreshDimensions();
}

void AreaEffectCloudEntity::addEffect(const effect::EffectInstance& effect)
{
    // 添加效果并更新颜色
    m_effects.push_back(effect);
    if (!m_colorSet) {
        _updateColor();
    }
}

void AreaEffectCloudEntity::setOwner(LivingEntity* owner)
{
    m_owner = owner;
    if (owner != nullptr) {
        m_ownerUuid = owner->uuid();
    } else {
        m_ownerUuid.clear();
    }
}

LivingEntity* AreaEffectCloudEntity::getOwner()
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

void AreaEffectCloudEntity::setOwnerUuid(const std::string& uuid)
{
    m_ownerUuid = uuid;
    // 不设置 m_owner 指针，等到 getOwner() 被调用时再通过 UUID 懒加载查找
    m_owner = nullptr;
}

void AreaEffectCloudEntity::tick()
{
    Entity::tick();

    m_ticksLived++;

    // 检查生命周期结束
    if (m_ticksLived >= m_waitTime + m_duration) {
        remove();
        return;
    }

    // 等待时间判断
    bool inWaitTime = m_ticksLived < m_waitTime;

    if (inWaitTime) {
        return; // 等待期间不执行效果
    }

    // 半径按tick变化
    if (m_radiusPerTick != 0.0f) {
        m_radius += m_radiusPerTick;
        if (m_radius < 0.5f) {
            // 半径太小则移除
            remove();
            return;
        }
    }

    // 每5个tick执行效果应用
    if (m_ticksLived % 5 == 0) {
        _applyEffects();
    }
}

void AreaEffectCloudEntity::_applyEffects()
{
    // 服务端逻辑
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 没有效果则跳过
    if (m_effects.empty()) {
        return;
    }

    // 清理过期的重应用延迟映射
    auto it = m_reapplicationMap.begin();
    while (it != m_reapplicationMap.end()) {
        if (m_ticksLived >= it->second) {
            it = m_reapplicationMap.erase(it);
        } else {
            ++it;
        }
    }

    // 构建效果列表（持续时间除以4）
    // 滞留药水效果持续时间 = 原持续时间 / 4
    // 注意：由于 EffectInstance 的 duration 是私有的，我们需要用 tick 来调整
    // 但根据实现，滞留药水创建时效果持续时间已设置好
    std::vector<effect::EffectInstance> effectsToApply;
    for (const auto& effect : m_effects) {
        effect::EffectInstance copy = effect;
        effectsToApply.push_back(copy);
    }

    // 获取范围内的生物
    AxisAlignedBB box(m_builtIn.stateVector->m_pos.x - m_radius,
        m_builtIn.stateVector->m_pos.y - 0.5f,
        m_builtIn.stateVector->m_pos.z - m_radius,
        m_builtIn.stateVector->m_pos.x + m_radius,
        m_builtIn.stateVector->m_pos.y + 0.5f,
        m_builtIn.stateVector->m_pos.z + m_radius);

    std::vector<Entity*> entities = m_world->getEntitiesInAABB(box, this);

    // 获取 owner（用于构建正确的伤害来源）
    LivingEntity* owner = getOwner();

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // 只对 LivingEntity 生效
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living == nullptr) {
            continue;
        }

        // 检查是否在重应用延迟中
        EntityInstanceId entityId = entity->id();
        if (m_reapplicationMap.find(entityId) != m_reapplicationMap.end()) {
            continue;
        }

        // 检查实体是否可以被药水影响
        // canBeHitWithPotion() - 盔甲架返回 false，其他生物返回 true
        if (!living->canBeHitWithPotion()) {
            continue;
        }

        // 检查水平距离（只检查XZ平面）
        f32 dx = static_cast<f32>(entity->x() - m_builtIn.stateVector->m_pos.x);
        f32 dz = static_cast<f32>(entity->z() - m_builtIn.stateVector->m_pos.z);
        f32 distSq = dx * dx + dz * dz;

        if (distSq <= m_radius * m_radius) {
            // 在半径内，应用效果
            for (const auto& effect : effectsToApply) {
                // 瞬间效果使用 affectEntity，持续效果使用 addPotionEffect
                if (effect::isInstantEffect(effect.type())) {
                    // 瞬间效果（如瞬间治疗、瞬间伤害、饱和）
                    // 乘数 0.5 表示药水云中的效果强度为原效果的一半
                    applyInstantEffect(effect.type(), *this, owner, *living, effect.amplifier(), 0.5f);
                } else {
                    // 持续效果
                    living->addEffect(effect);
                }
            }

            // 记录重应用延迟
            m_reapplicationMap[entityId] = m_ticksLived + m_reapplicationDelay;

            // 应用后半径变化
            if (m_radiusOnUse != 0.0f) {
                m_radius += m_radiusOnUse;
                if (m_radius < 0.5f) {
                    remove();
                    return;
                }
            }

            // 应用后持续时间变化
            if (m_durationOnUse != 0) {
                m_duration += m_durationOnUse;
                if (m_duration <= 0) {
                    remove();
                    return;
                }
            }
        }
    }
}

void AreaEffectCloudEntity::_updateRadius()
{
    // 半径变化现在在 tick() 中处理
    // 这个方法保留用于其他地方可能需要的半径更新
}

void AreaEffectCloudEntity::_updateColor()
{
    if (m_effects.empty()) {
        m_color = 0;
    } else {
        m_color = _calculateEffectsColor(m_effects);
    }
}

u32 AreaEffectCloudEntity::_calculateEffectsColor(const std::vector<effect::EffectInstance>& effects)
{
    // 混合所有效果的颜色
    if (effects.empty()) {
        return 0;
    }

    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    i32 count = 0;

    for (const auto& effect : effects) {
        u32 effectColor = effect::getEffectColor(effect.type());
        if (effectColor != 0) {
            f32 cr = static_cast<f32>((effectColor >> 16) & 0xFF) / 255.0f;
            f32 cg = static_cast<f32>((effectColor >> 8) & 0xFF) / 255.0f;
            f32 cb = static_cast<f32>(effectColor & 0xFF) / 255.0f;
            r += cr;
            g += cg;
            b += cb;
            ++count;
        }
    }

    if (count == 0) {
        return 0;
    }

    r = r / static_cast<f32>(count);
    g = g / static_cast<f32>(count);
    b = b / static_cast<f32>(count);

    // 返回 ARGB 格式
    return (0xFF << 24) | (static_cast<u32>(r * 255.0f) << 16) | (static_cast<u32>(g * 255.0f) << 8) |
        static_cast<u32>(b * 255.0f);
}

void AreaEffectCloudEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 调用基类序列化
    Entity::addAdditionalSaveData(tag);

    using namespace serialization::nbt_keys;

    // 生命周期
    tag.put(CLOUD_AGE, m_ticksLived);
    tag.put(CLOUD_DURATION, m_duration);
    tag.put(CLOUD_WAIT_TIME, m_waitTime);
    tag.put(CLOUD_REAPPLICATION_DELAY, m_reapplicationDelay);
    tag.put(CLOUD_DURATION_ON_USE, m_durationOnUse);

    // 半径
    tag.put(CLOUD_RADIUS_ON_USE, m_radiusOnUse);
    tag.put(CLOUD_RADIUS_PER_TICK, m_radiusPerTick);
    tag.put(CLOUD_RADIUS, m_radius);

    // 拥有者 UUID
    // 采用 UUIDMost/UUIDLeast 格式存储，与 Entity 基类的 UUID 格式一致
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
            tag.put("OwnerUUIDMost", most);
            tag.put("OwnerUUIDLeast", least);
        }
    }

    // 粒子类型
    tag.put(CLOUD_PARTICLE, static_cast<i32>(m_particleType));

    // 颜色
    tag.put(CLOUD_COLOR, static_cast<i32>(m_color));

    // 效果列表
    if (!m_effects.empty()) {
        auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
        for (const auto& effect : m_effects) {
            nbt::tags::compound_tag effectTag;
            effect.toNbt(effectTag);
            effectsList->value.push_back(std::move(effectTag));
        }
        tag.value.emplace(CLOUD_EFFECTS, std::move(effectsList));
    }
}

Result<void> AreaEffectCloudEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 调用基类反序列化
    MC_TRY(Entity::readAdditionalSaveData(tag));

    using namespace serialization::nbt_keys;

    // 生命周期
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_AGE)) {
        m_ticksLived = *val;
    }
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_DURATION)) {
        m_duration = *val;
    }
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_WAIT_TIME)) {
        m_waitTime = *val;
    }
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_REAPPLICATION_DELAY)) {
        m_reapplicationDelay = *val;
    }
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_DURATION_ON_USE)) {
        m_durationOnUse = *val;
    }

    // 半径
    if (auto val = serialization::nbt_helper::tryGetFloat(tag, CLOUD_RADIUS_ON_USE)) {
        m_radiusOnUse = *val;
    }
    if (auto val = serialization::nbt_helper::tryGetFloat(tag, CLOUD_RADIUS_PER_TICK)) {
        m_radiusPerTick = *val;
    }
    if (auto val = serialization::nbt_helper::tryGetFloat(tag, CLOUD_RADIUS)) {
        m_radius = *val;
        m_initialRadius = m_radius;
    }

    // 拥有者 UUID
    // 读取 OwnerUUIDMost/OwnerUUIDLeast，转换为 UUID 字符串
    auto mostVal = serialization::nbt_helper::tryGetLong(tag, "OwnerUUIDMost");
    auto leastVal = serialization::nbt_helper::tryGetLong(tag, "OwnerUUIDLeast");
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

    // 粒子类型
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_PARTICLE)) {
        m_particleType = static_cast<u32>(*val);
    }

    // 颜色
    if (auto val = serialization::nbt_helper::tryGetInt(tag, CLOUD_COLOR)) {
        m_color = static_cast<u32>(*val);
        m_colorSet = (m_color != 0); // 如果从 NBT 读取了颜色，则标记为已设置
    }

    // 效果列表
    auto effectsIt = tag.value.find(CLOUD_EFFECTS);
    if (effectsIt != tag.value.end() && effectsIt->second->id() == nbt::TagId::List) {
        const auto& effectsList = dynamic_cast<const nbt::tags::compound_list_tag&>(*effectsIt->second);
        m_effects.clear();
        for (const auto& effectTag : effectsList.value) {
            m_effects.push_back(effect::EffectInstance::fromNbt(effectTag));
        }
    }

    // 刷新碰撞箱
    refreshDimensions();

    return Result<void>::ok();
}

// 注意: ExperienceOrbEntity 已移动到独立的 orb/ 目录
// 文件位置: src/common/entity/entities/orb/ExperienceOrbEntity.cpp

// ==================== ArmorStandEntity ====================

ArmorStandEntity::ArmorStandEntity(ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
{}

std::unique_ptr<Entity> ArmorStandEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ArmorStandEntity>(registry);
}

f32 ArmorStandEntity::width() const
{
    return m_marker ? 0.0f : 0.5f; // 标记模式无碰撞箱，否则 0.5
}

f32 ArmorStandEntity::height() const
{
    return m_marker ? 0.0f : 1.975f; // 标记模式无碰撞箱，否则 1.975
}

void ArmorStandEntity::tick()
{
    Entity::tick();

    // 如果不是标记模式，应用重力
    if (!m_marker && m_hasGravity) {
        Vector3 vel = velocity();
        vel.y -= 0.04f; // 重力
        move(vel.x, vel.y, vel.z);

        // 减速
        vel.x *= 0.98f;
        vel.y *= 0.98f;
        vel.z *= 0.98f;
        setVelocity(vel);
    }
}

bool ArmorStandEntity::ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const
{
    // 仅当爆炸影响方块类实体时才受影响；不可见盔甲架忽略爆炸。
    // TODO: vanilla 用 Entity.isInvisible()（共享 flags），Cubium Entity 暂无通用 isInvisible，
    //       此处用 ArmorStand 自身 isInvisible() 近似，未来 Entity 引入通用 invisible 后切换。
    return ctx.shouldAffectBlocklikeEntities ? isInvisible() : true;
}

void ArmorStandEntity::setHeadRotation(f32 x, f32 y, f32 z)
{
    m_head = {x, y, z};
}

void ArmorStandEntity::setBodyRotation(f32 x, f32 y, f32 z)
{
    m_body = {x, y, z};
}

void ArmorStandEntity::setLeftArmRotation(f32 x, f32 y, f32 z)
{
    m_leftArm = {x, y, z};
}

void ArmorStandEntity::setRightArmRotation(f32 x, f32 y, f32 z)
{
    m_rightArm = {x, y, z};
}

void ArmorStandEntity::setLeftLegRotation(f32 x, f32 y, f32 z)
{
    m_leftLeg = {x, y, z};
}

void ArmorStandEntity::setRightLegRotation(f32 x, f32 y, f32 z)
{
    m_rightLeg = {x, y, z};
}

void ArmorStandEntity::causeDamage(f32 damage)
{
    // 对齐 vanilla ArmorStand.causeDamage：直接 setHealth(health - damage)，health<=0 调 kill。
    // 盔甲架不经过护甲/附魔减伤链路。Cubium 用自带 m_health 模拟，扣至 0 及以下销毁。
    m_health -= damage;
    if (m_health <= 0.0f) {
        m_health = 0.0f;
        remove();
    }
}

bool ArmorStandEntity::hurt(DamageSource& source, f32 amount)
{
    // 对齐 vanilla ArmorStand.hurtServer:266-318（ArmorStand.java）。分支链按伤害源标签判定：
    //   1. isRemoved → 拒绝
    //   2. !mobGriefing && source.entity instanceof Mob → 拒绝
    //   3. BYPASSES_INVULNERABILITY → kill
    //   4. isInvulnerableTo || invisible || marker → 拒绝
    //   5. IS_EXPLOSION → brokenByAnything + kill
    //   6. IGNITES_ARMOR_STANDS：isOnFire → causeDamage(0.15) else igniteForSeconds(5)
    //   7. BURNS_ARMOR_STANDS && health>0.5 → causeDamage(4.0)
    //   8. CAN_BREAK/ALWAYS_KILLS：mayBuild 守卫、creativePlayer→kill、5tick 节流/brokenByPlayer→kill
    //
    // 架构差异：vanilla ArmorStand 继承 LivingEntity（maxHealth=2、有 causeDamage/kill/brokenByAnybody
    // 装备掉落体系）。Cubium ArmorStandEntity 继承 Entity 无 health/装备体系，此处用自带 m_health
    // 模拟 causeDamage（扣血）与 kill（销毁）。装备掉落（brokenByAnything/brokenByPlayer）、mobGriefing
    // Mob 守卫、creativePlayer、5tick 节流等依赖未实现体系的分支加 TODO 简化或守卫。
    MC_UNUSED(amount);

    if (isRemoved()) {
        return false;
    }

    // 2. mobGriefing 守卫：伤害来源是 Mob 且 mobGriefing=false 时拒绝（vanilla :269）。
    // TODO: 完整对齐需 dynamic_cast<MobEntity>(source.getEntity())，当前简化为无条件放行（测试不涉及 mob 攻击盔甲架）。

    // 3. BYPASSES_INVULNERABILITY → kill（销毁，对齐 vanilla :271-273）。
    if (source.is(DamageTypeTags::BYPASSES_INVULNERABILITY())) {
        remove();
        return false;
    }

    // 4. isInvulnerableTo || invisible || marker → 拒绝（对齐 vanilla :274）。
    if (isInvulnerableTo(source) || m_invisible || m_marker) {
        return false;
    }

    // 5. IS_EXPLOSION → brokenByAnything + kill（对齐 vanilla :276-279）。
    // TODO: brokenByAnything 应掉落盔甲架穿戴的装备（依赖未实现的装备体系），当前仅销毁。
    if (source.is(DamageTypeTags::IS_EXPLOSION())) {
        remove();
        return false;
    }

    // 6. IGNITES_ARMOR_STANDS：着火则受 0.15 伤害，未着火则点燃 5 秒（对齐 vanilla :280-287）。
    if (source.is(DamageTypeTags::IGNITES_ARMOR_STANDS())) {
        if (isOnFire()) {
            causeDamage(0.15f);
        } else {
            igniteForSeconds(5.0f);
        }
        return false;
    }

    // 7. BURNS_ARMOR_STANDS（on_fire）&& health>0.5 → causeDamage(4.0)（对齐 vanilla :288-290）。
    if (source.is(DamageTypeTags::BURNS_ARMOR_STANDS()) && m_health > 0.5f) {
        causeDamage(4.0f);
        return false;
    }

    // 8. CAN_BREAK/ALWAYS_KILLS 近战/箭破坏（对齐 vanilla :292-316）。
    const bool canBreak = source.is(DamageTypeTags::CAN_BREAK_ARMOR_STAND());
    const bool alwaysKills = source.is(DamageTypeTags::ALWAYS_KILLS_ARMOR_STANDS());
    if (!canBreak && !alwaysKills) {
        return false;
    }

    // mayBuild 守卫：来源玩家无建造权限则拒绝（vanilla :296）。
    Entity* attacker = source.getEntity();
    Player* player = dynamic_cast<Player*>(attacker);
    if (player != nullptr && !player->mayBuild()) {
        return false;
    }

    // creativePlayer：创造模式直接破坏销毁，不进入节流（vanilla :298-302）。
    // 注：vanilla isCreativePlayer() = getEntity() instanceof Player && abilities.instabuild。
    // Cubium DamageSource 无 isCreativePlayer()，用 source.getEntity() dynamic_cast<Player> + isCreative() 近似。
    if (player != nullptr && player->isCreative()) {
        // TODO: playBrokenSound + showBreakingParticles（纯客户端效果，依赖未实现的音效/粒子体系）。
        remove();
        return true;
    }

    // 5 tick 节流：距上次受击 >5 tick 才记录（广播受击事件），否则 brokenByPlayer 销毁（vanilla :304-313）。
    // TODO: vanilla 用 level.getGameTime() - lastHit > 5，Cubium 用 ticksExisted() 近似（无独立 gameTime 缓存）。
    //       ALWAYS_KILLS 标签跳过节流直接销毁（vanilla :305 !flag1）。
    const u64 currentTick = m_world != nullptr ? m_world->getGameTime() : ticksExisted();
    if (!alwaysKills && currentTick - static_cast<u64>(m_lastHit) > 5ULL) {
        m_lastHit = static_cast<i64>(currentTick);
        // TODO: 广播 EntityEvent(32) 受击音效 + gameEvent(ENTITY_DAMAGE)（依赖未实现的事件广播体系）。
        return true;
    }

    // brokenByPlayer + showBreakingParticles + kill（vanilla :310-312）。
    // TODO: brokenByPlayer 应掉落盔甲架穿戴的装备（依赖未实现的装备体系），当前仅销毁。
    remove();
    return true;
}

} // namespace entity
} // namespace mc
