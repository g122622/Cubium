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
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../boss/EnderDragonEntity.hpp"
#include "../player/Player.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <chrono>
#include <cmath>

namespace mc {
namespace entity {

namespace {

/**
 * @brief 应用瞬间效果到目标实体
 *
 * 参考 MC 1.16.5 EffectInstant.affectEntity()
 * 用于药水云、喷溅药水等场景中瞬间效果的应用。
 *
 * @param type 效果类型（必须是瞬间效果）
 * @param target 目标生物
 * @param amplifier 效果等级（0 = I, 1 = II）
 * @param multiplier 效果乘数（通常为 0.5 或 1.0）
 */
void applyInstantEffect(effect::EffectType type, LivingEntity& target, i32 amplifier, f32 multiplier)
{
    // MC 1.16.5: 基础值 4.0，每级增加 2.0
    f32 amount = (4.0f + static_cast<f32>(amplifier) * 2.0f) * multiplier;

    switch (type) {
        case effect::EffectType::InstantHealth:
            // 瞬间治疗：亡灵生物受到伤害，普通生物治疗
            if (target.getCreatureAttribute() == CreatureAttribute::Undead) {
                auto source = DamageSources::magic();
                target.hurt(source, amount);
            } else {
                target.heal(amount);
            }
            break;

        case effect::EffectType::InstantDamage:
            // 瞬间伤害：亡灵生物治疗，普通生物受到伤害
            if (target.getCreatureAttribute() == CreatureAttribute::Undead) {
                target.heal(amount);
            } else {
                auto source = DamageSources::magic();
                target.hurt(source, amount);
            }
            break;

        case effect::EffectType::Saturation:
            // 饱和效果：恢复饥饿值（仅对玩家有效）
            // TODO: 当玩家饥饿系统完善后，恢复饥饿值和饱和度
            // 目前通过治疗模拟效果
            if (target.getCreatureAttribute() != CreatureAttribute::Undead) {
                target.heal(amount * 0.5f);
            }
            break;

        default:
            // 非瞬间效果不做处理
            break;
    }
}

} // namespace

// ==================== EnderCrystalEntity ====================

EnderCrystalEntity::EnderCrystalEntity()
    : Entity(EntityId(0))
{}

f32 EnderCrystalEntity::width() const
{
    return 2.0f; // MC 1.16.5: 末地水晶宽度
}

f32 EnderCrystalEntity::height() const
{
    return 2.0f; // MC 1.16.5: 末地水晶高度
}

void EnderCrystalEntity::tick()
{
    Entity::tick();

    // MC 1.16.5: 递增内部旋转计数器（用于渲染动画）
    ++m_innerRotation;

    // 治愈末影龙冷却
    if (m_healCooldown > 0) {
        m_healCooldown--;
    }

    // MC 1.16.5: 服务端在末地且存在 DragonFightManager 时，在脚下放置火焰
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
    // MC 1.16.5: EnderCrystalEntity 治愈末影龙逻辑
    // 参考: EnderDragonEntity.updateDragonEnderCrystal() 双向关联

    // 检查冷却
    if (m_healCooldown > 0) {
        return;
    }

    // 获取世界
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // MC 1.16.5: 在 32 格范围内搜索末影龙
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
        if (entity->typeId() == entity::EntityTypeIdNumber::ENDER_DRAGON) {
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
        // MC 1.16.5: 治愈末影龙 1 点生命值
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
    // MC 1.16.5: 末地水晶爆炸
    // 参考: EnderCrystalEntity.attackEntityFrom() line 105
    // this.world.createExplosion((Entity)null, this.getPosX(), this.getPosY(), this.getPosZ(), 6.0F,
    // Explosion.Mode.DESTROY);
    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        // 爆炸半径 6.0，模式 DESTROY（破坏方块并掉落物品）
        worldPtr->createExplosion(m_position,
            6.0f, // MC 1.16.5: 末地水晶爆炸半径
            world::explosion::ExplosionMode::Destroy,
            false,  // 不生成火焰
            nullptr // 无爆炸源实体
        );
    }
    remove();
}

// ==================== LightningBoltEntity ====================

LightningBoltEntity::LightningBoltEntity()
    : Entity(EntityId(0))
{
    // MC 1.16.5: ignoreFrustumCheck = true
    // 闪电总是可见，即使不在视锥内
}

f32 LightningBoltEntity::width() const
{
    return 0.0f; // MC 1.16.5: 闪电没有碰撞箱
}

f32 LightningBoltEntity::height() const
{
    return 0.0f; // MC 1.16.5: 闪电没有碰撞箱
}

void LightningBoltEntity::initializeState()
{
    // MC 1.16.5 构造函数中的初始化：
    // lightningState = 2
    // boltVertex = rand.nextLong()
    // boltLivingTime = rand.nextInt(3) + 1

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

    // MC 1.16.5: 首次 tick 初始化状态
    if (!m_initialized) {
        initializeState();
    }

    // MC 1.16.5: lightningState == 2 时执行初始效果
    // 播放音效、点燃方块、造成伤害
    if (m_lightningState == 2) {
        // MC 1.16.5: 难度检查 - NORMAL 和 HARD 点燃更多火焰
        if (m_world != nullptr && !m_effectOnly && !m_world->isClientSide()) {
            Difficulty difficulty = m_world->difficulty();
            if (difficulty == Difficulty::Normal || difficulty == Difficulty::Hard) {
                igniteBlocks(4);
            } else {
                igniteBlocks(0);
            }
        }

        // MC 1.16.5: 播放雷声音效
        // 音量 10000（非常大的范围），音调 0.8-1.0
        if (m_world != nullptr) {
            // 使用 boltVertex 生成一致的随机音调
            f32 thunderPitch = 0.8f + static_cast<f32>(m_boltVertex % 100) / 100.0f * 0.2f;
            m_world->playSound(SoundEvents::WEATHER_THUNDER,
                sound::SoundCategory::Weather,
                m_position,
                10000.0f, // MC 1.16.5: 10000 音量（可传很远）
                thunderPitch);

            // MC 1.16.5: 播放雷击声音效（音量 2，音调 0.5-0.7）
            f32 impactPitch = 0.5f + static_cast<f32>((m_boltVertex >> 8) % 100) / 100.0f * 0.2f;
            m_world->playSound(
                SoundEvents::WEATHER_THUNDER, sound::SoundCategory::Weather, m_position, 2.0f, impactPitch);
        }

        // MC 1.16.5: 服务端造成伤害（非 effectOnly，非客户端）
        if (m_world != nullptr && !m_world->isClientSide() && !m_effectOnly) {
            damageEntities();
        }

        // MC 1.16.5: 客户端设置闪电闪烁效果
        // world.setTimeLightningFlash(2)
        if (m_world != nullptr && m_world->isClientSide()) {
            m_world->setTimeLightningFlash(2);
        }
    }

    // MC 1.16.5: 递减 lightningState
    --m_lightningState;

    // MC 1.16.5: lightningState < 0 时检查是否"复活"
    if (m_lightningState < 0) {
        if (m_boltLivingTime == 0) {
            // 所有视觉效果结束，移除实体
            remove();
        } else if (m_lightningState < -static_cast<i32>(m_boltVertex % 10)) {
            // MC 1.16.5: 随机间隔后"复活"
            // 闪电会多次闪烁，模拟真实闪电效果
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
            igniteBlocks(0);
        }
    }
}

void LightningBoltEntity::igniteBlocks(i32 extraIgnitions)
{
    // MC 1.16.5 igniteBlocks():
    // 检查游戏规则 doFireTick 和是否为客户端
    if (m_effectOnly || m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // MC 1.16.5: 检查游戏规则 doFireTick
    if (!m_world->doFireTick()) {
        return;
    }

    // 获取当前位置
    BlockPos blockPos(static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_position.y)),
        static_cast<i32>(std::floor(m_position.z)));

    // MC 1.16.5: 在当前位置放置火焰
    const BlockState* currentState = m_world->getBlockState(blockPos);
    if (currentState != nullptr && currentState->isAir()) {
        // MC 1.16.5: AbstractFireBlock.getFireForPlacement()
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

    // MC 1.16.5: 额外点燃周围方块
    if (extraIgnitions > 0) {
        math::Random rng(m_boltVertex);

        for (i32 i = 0; i < extraIgnitions; ++i) {
            // MC 1.16.5: pos.add(rand.nextInt(3) - 1, rand.nextInt(3) - 1, rand.nextInt(3) - 1)
            i32 dx = rng.nextInt(3) - 1;
            i32 dy = rng.nextInt(3) - 1;
            i32 dz = rng.nextInt(3) - 1;

            BlockPos firePos(blockPos.x + dx, blockPos.y + dy, blockPos.z + dz);

            const BlockState* stateAtPos = m_world->getBlockState(firePos);
            if (stateAtPos != nullptr && stateAtPos->isAir()) {
                // MC 1.16.5: AbstractFireBlock.getFireForPlacement()
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

void LightningBoltEntity::damageEntities()
{
    // MC 1.16.5: 获取 3x6x3 范围内的实体
    // AxisAlignedBB(pos.x - 3, pos.y - 3, pos.z - 3, pos.x + 3, pos.y + 6 + 3, pos.z + 3)
    if (m_world == nullptr || m_effectOnly) {
        return;
    }

    // 构建碰撞箱
    // MC 1.16.5: new AxisAlignedBB(posX - 3.0, posY - 3.0, posZ - 3.0, posX + 3.0, posY + 6.0 + 3.0, posZ + 3.0)
    AxisAlignedBB box(m_position.x - DAMAGE_RADIUS_XZ,
        m_position.y - DAMAGE_RADIUS_Y_OFFSET,
        m_position.z - DAMAGE_RADIUS_XZ,
        m_position.x + DAMAGE_RADIUS_XZ,
        m_position.y + DAMAGE_RADIUS_Y + DAMAGE_RADIUS_Y_OFFSET,
        m_position.z + DAMAGE_RADIUS_XZ);

    // 获取范围内的实体
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(box, this);

    // 收集被击中的实体用于引雷附魔进度触发
    std::vector<Entity*> victims;

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // MC 1.16.5: 调用 entity.func_241841_a() (onStruckByLightning)
        // 对于 LivingEntity，造成闪电伤害
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living != nullptr) {
            // 创建闪电伤害来源
            auto damageSource = DamageSources::lightningBolt(this);
            // MC 1.16.5: 闪电伤害为 5.0
            living->hurt(damageSource, 5.0f);
        }

        // MC 1.16.5: 调用实体的 onStruckByLightning() 方法
        // 用于处理特殊效果（如哞菇变色、苦力怕充能等）
        entity->onStruckByLightning();

        // 收集被击中的实体用于引雷附魔进度触发
        victims.push_back(entity);
    }

    // MC 1.16.5: 触发进度 CriteriaTriggers.CHANNELED_LIGHTNING
    // 如果有 caster（引雷附魔的玩家），通过 IWorld 发布事件
    if (m_caster != 0 && !victims.empty() && m_world != nullptr) {
        m_world->onChanneledLightning(m_caster, victims);
    }
}

// ==================== AreaEffectCloudEntity ====================

AreaEffectCloudEntity::AreaEffectCloudEntity()
    : Entity(EntityId(0))
{
    // MC 1.16.5: AreaEffectCloudEntity 无碰撞
    setNoClip(true);
}

std::unique_ptr<Entity> AreaEffectCloudEntity::create(IWorld* /*world*/)
{
    return std::make_unique<AreaEffectCloudEntity>();
}

f32 AreaEffectCloudEntity::width() const
{
    return m_radius * 2.0f; // MC 1.16.5: 实际宽度是半径的两倍
}

f32 AreaEffectCloudEntity::height() const
{
    return 0.5f; // MC 1.16.5: 药水云高度固定为 0.5
}

void AreaEffectCloudEntity::setRadius(f32 radius)
{
    m_radius = radius;
    m_initialRadius = radius;
    // MC 1.16.5: 宽度随半径变化，需要刷新碰撞箱
    refreshDimensions();
}

void AreaEffectCloudEntity::addEffect(const effect::EffectInstance& effect)
{
    // MC 1.16.5: 添加效果并更新颜色
    m_effects.push_back(effect);
    if (!m_colorSet) {
        updateColor();
    }
}

void AreaEffectCloudEntity::setOwner(LivingEntity* owner)
{
    m_owner = owner;
    // MC 1.16.5: 同时记录 ownerUniqueId
    if (owner != nullptr) {
        // 后续可添加 UUID 追踪
    }
}

void AreaEffectCloudEntity::tick()
{
    Entity::tick();

    m_ticksLived++;

    // MC 1.16.5: 检查生命周期结束
    if (m_ticksLived >= m_waitTime + m_duration) {
        remove();
        return;
    }

    // MC 1.16.5: 等待时间判断
    bool inWaitTime = m_ticksLived < m_waitTime;

    if (inWaitTime) {
        return; // 等待期间不执行效果
    }

    // MC 1.16.5: 半径按tick变化
    if (m_radiusPerTick != 0.0f) {
        m_radius += m_radiusPerTick;
        if (m_radius < 0.5f) {
            // 半径太小则移除
            remove();
            return;
        }
    }

    // MC 1.16.5: 每5个tick执行效果应用
    if (m_ticksLived % 5 == 0) {
        applyEffects();
    }
}

void AreaEffectCloudEntity::applyEffects()
{
    // MC 1.16.5: 服务端逻辑
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
    // MC 1.16.5: effectinstance.getDuration() / 4
    std::vector<effect::EffectInstance> effectsToApply;
    for (const auto& effect : m_effects) {
        effect::EffectInstance copy = effect;
        // 滞留药水效果持续时间 = 原持续时间 / 4
        // 注意：由于 EffectInstance 的 duration 是私有的，我们需要用 tick 来调整
        // 但根据 MC 源码，滞留药水创建时效果持续时间已设置好
        effectsToApply.push_back(copy);
    }

    // 获取范围内的生物
    AxisAlignedBB box(m_position.x - m_radius,
        m_position.y - 0.5f,
        m_position.z - m_radius,
        m_position.x + m_radius,
        m_position.y + 0.5f,
        m_position.z + m_radius);

    std::vector<Entity*> entities = m_world->getEntitiesInAABB(box, this);

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
        EntityId entityId = entity->id();
        if (m_reapplicationMap.find(entityId) != m_reapplicationMap.end()) {
            continue;
        }

        // MC 1.16.5: 检查实体是否可以被药水影响
        // canBeHitWithPotion() - 盔甲架返回 false，其他生物返回 true
        if (!living->canBeHitWithPotion()) {
            continue;
        }

        // 检查水平距离（只检查XZ平面）
        f32 dx = static_cast<f32>(entity->x() - m_position.x);
        f32 dz = static_cast<f32>(entity->z() - m_position.z);
        f32 distSq = dx * dx + dz * dz;

        if (distSq <= m_radius * m_radius) {
            // 在半径内，应用效果
            for (const auto& effect : effectsToApply) {
                // MC 1.16.5: 瞬间效果使用 affectEntity，持续效果使用 addPotionEffect
                if (effect::isInstantEffect(effect.type())) {
                    // 瞬间效果（如瞬间治疗、瞬间伤害、饱和）
                    // MC 1.16.5: affectEntity(this, owner, living, amplifier, 0.5)
                    // 乘数 0.5 表示药水云中的效果强度为原效果的一半
                    applyInstantEffect(effect.type(), *living, effect.amplifier(), 0.5f);
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

void AreaEffectCloudEntity::updateRadius()
{
    // 半径变化现在在 tick() 中处理
    // 这个方法保留用于其他地方可能需要的半径更新
}

void AreaEffectCloudEntity::updateColor()
{
    if (m_effects.empty()) {
        m_color = 0;
    } else {
        m_color = calculateEffectsColor(m_effects);
    }
}

u32 AreaEffectCloudEntity::calculateEffectsColor(const std::vector<effect::EffectInstance>& effects)
{
    // MC 1.16.5: PotionUtils.getPotionColorFromEffectList()
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
    return (0xFF << 24) | (static_cast<u32>(r * 255.0f) << 16)
        | (static_cast<u32>(g * 255.0f) << 8) | static_cast<u32>(b * 255.0f);
}

// 注意: ExperienceOrbEntity 已移动到独立的 orb/ 目录
// 文件位置: src/common/entity/entities/orb/ExperienceOrbEntity.cpp

// ==================== ArmorStandEntity ====================

ArmorStandEntity::ArmorStandEntity()
    : Entity(EntityId(0))
{}

f32 ArmorStandEntity::width() const
{
    return m_marker ? 0.0f : 0.5f; // MC 1.16.5: 标记模式无碰撞箱，否则 0.5
}

f32 ArmorStandEntity::height() const
{
    return m_marker ? 0.0f : 1.975f; // MC 1.16.5: 标记模式无碰撞箱，否则 1.975
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

} // namespace entity
} // namespace mc
