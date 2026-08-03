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

#include "CreeperEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <memory>
#include <optional>
#include <utility>

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

CreeperEntity::CreeperEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 苦力怕不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CreeperEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CreeperEntity>(EntityInstanceId(0));
}

std::optional<ResourceLocation> CreeperEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_CREEPER_HURT;
}

std::optional<ResourceLocation> CreeperEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_CREEPER_DEATH;
}

i32 CreeperEntity::getCreeperState() const
{
    // -1 = idle, 1 = fusing
    if (m_timeSinceIgnited > 0) {
        return 1;
    }
    return -1;
}

void CreeperEntity::setCreeperState(i32 state)
{
    if (state > 0) {
        ignite();
    }
}

void CreeperEntity::ignite()
{
    m_ignited = true;
}

bool CreeperEntity::ableToCauseSkullDrop() const
{
    // 只有高压苦力怕且还没掉过头颅才能导致头颅掉落
    return m_powered && m_droppedSkulls < 1;
}

void CreeperEntity::explode()
{
    // 只在服务端爆炸
    if (isDead()) return;

    IWorld* worldPtr = world();
    if (!worldPtr) {
        remove();
        return;
    }

    // 计算爆炸威力（高压翻倍）
    f32 radius = static_cast<f32>(m_explosionRadius);
    if (m_powered) {
        radius *= 2.0f;
    }

    // 苦力怕爆炸模式：检查 mobGriefing 游戏规则
    // 如果 mobGriefing 为 false，则使用 NONE（不破坏方块）
    world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy;
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        mode = world::explosion::ExplosionMode::None;
    }

    // 创建爆炸
    worldPtr->createExplosion(position(),
        radius,
        mode,
        false, // 不生成火焰
        this   // 爆炸源实体
    );

    // 移除实体
    remove();

    // 生成滞留药水云（如果有药水效果）
    _spawnLingeringCloud();
}

void CreeperEntity::_spawnLingeringCloud()
{
    // 当苦力怕有药水效果时，死亡后生成滞留药水云

    // 获取苦力怕当前所有药水效果
    const auto& effects = effectManager().getAllEffects();
    if (effects.empty()) {
        return; // 无效果则不生成
    }

    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 创建区域效果云实体
    auto cloud = std::make_unique<entity::AreaEffectCloudEntity>();
    cloud->setWorld(worldPtr);
    cloud->setPosition(x(), y(), z());

    // 苦力怕药水云参数：
    // - 初始半径: 2.5F
    // - radiusOnUse: -0.5F（每次应用效果后缩小）
    // - waitTime: 10 ticks（0.5秒）
    // - duration: 300 ticks（15秒，原默认600的一半）
    // - radiusPerTick: -radius/duration（线性衰减到0）

    cloud->setRadius(2.5f);
    cloud->setRadiusOnUse(-0.5f);
    cloud->setWaitTime(10);
    cloud->setDuration(300);
    cloud->setRadiusPerTick(-2.5f / 300.0f);

    // 添加所有效果到药水云
    for (const auto& effect : effects) {
        cloud->addEffect(effect);
    }

    // 设置拥有者为苦力怕
    cloud->setOwner(this);

    // 生成实体
    worldPtr->spawnEntity(std::move(cloud));
}

void CreeperEntity::tick()
{
    if (isAlive()) {
        m_lastActiveTime = m_timeSinceIgnited;

        // 如果被点燃，设置状态为膨胀
        if (hasIgnited()) {
            setCreeperState(1);
        }

        i32 state = getCreeperState();
        if (state > 0 && m_timeSinceIgnited == 0) {
            // 开始膨胀时播放音效
            playSound(SoundEvents::ENTITY_CREEPER_PRIMED, 1.0f, 0.5f);
        }

        m_timeSinceIgnited += state;

        if (m_timeSinceIgnited < 0) {
            m_timeSinceIgnited = 0;
        }

        // 达到点燃时间后爆炸
        if (m_timeSinceIgnited >= m_fuseTime) {
            m_timeSinceIgnited = m_fuseTime;
            explode();
            return;
        }
    }

    MonsterEntity::tick();
}

void CreeperEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // AI 目标优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: PanicGoal (父类已注册，但苦力怕不使用)
    // 2: CreeperSwellGoal - 膨胀爆炸
    // 3: AvoidEntityGoal<OcelotEntity> - 避开豹猫
    // 3: AvoidEntityGoal<CatEntity> - 避开猫
    // 4: MeleeAttackGoal - 近战攻击（实际不造成伤害，用于接近玩家）
    // 5: WaterAvoidingRandomWalkingGoal - 避水随机行走
    // 6: LookAtGoal<PlayerEntity> - 看向玩家
    // 6: LookRandomlyGoal - 随机看向
    //
    // 目标选择器：
    // 1: NearestAttackableTargetGoal<PlayerEntity> - 攻击玩家
    // 2: HurtByTargetGoal - 被攻击后反击

    // 优先级 2: 膨胀爆炸
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::CreeperSwellGoal>(this));

    // 优先级 3: 避开猫和豹猫
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(this,
            6.0f, // avoidDistance - 检测距离
            1.0,  // farSpeed - 远距离逃跑速度
            1.2,  // nearSpeed - 近距离逃跑速度（更快）
            [](const LivingEntity* entity) -> bool {
                if (!entity) return false;
                // 检查是否为猫或豹猫
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::CAT || type == entity::VanillaEntityTypeKeys::OCELOT;
            }));

    // 优先级 4: 近战攻击（用于接近玩家）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));

    // 优先级 5: 避水随机行走
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 0.8));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            // 只看向玩家
            return entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 优先级 1: 攻击最近的玩家
    // MC 原版: targetSelector.addGoal(1, NearestAttackableTargetGoal<Player>)
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight - 需要视线
            0,    // chance - 每tick都检查
            [](const LivingEntity* entity) -> bool {
                // 苦力怕只攻击玩家
                if (!entity) return false;
                return entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
            }));

    // 优先级 2: 被攻击后反击
    // MC 原版: targetSelector.addGoal(2, HurtByTargetGoal(this))
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this));
}

void CreeperEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 继承自 MonsterEntity: MAX_HEALTH = 20.0
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

std::optional<ResourceLocation> CreeperEntity::getAmbientSound() const
{
    // 苦力怕无环境音，对齐原版 Creeper（不 override → Mob 默认 null）。
    // sounds.json 中无 entity.creeper.ambient，仅有 primed/hurt/death。
    return std::nullopt;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void CreeperEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    MonsterEntity::addAdditionalSaveData(tag);

    // ExplosionRadius (i32) - 爆炸半径，默认为 3
    // 只有非默认值时才保存
    if (m_explosionRadius != DEFAULT_EXPLOSION_RADIUS) {
        tag.put(nbt_keys::EXPLOSION_RADIUS, m_explosionRadius);
    }

    // Fuse (i16) - 点燃时间，默认为 30
    // 只有非默认值时才保存
    if (m_fuseTime != DEFAULT_FUSE_TIME) {
        tag.put(nbt_keys::FUSE, static_cast<i16>(m_fuseTime));
    }

    // ignited (byte/bool) - 是否被打火石点燃
    tag.put(nbt_keys::IGNITED, static_cast<i8>(m_ignited ? 1 : 0));

    // powered (byte/bool) - 是否是高压苦力怕
    tag.put(nbt_keys::POWERED, static_cast<i8>(m_powered ? 1 : 0));
}

Result<void> CreeperEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现
    MC_TRY(MonsterEntity::readAdditionalSaveData(tag));

    // ExplosionRadius (i32) - 爆炸半径
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::EXPLOSION_RADIUS)) {
        m_explosionRadius = *val;
    }

    // Fuse (i16) - 点燃时间
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::FUSE)) {
        m_fuseTime = static_cast<i32>(*val);
    }

    // ignited (byte/bool) - 是否被打火石点燃
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::IGNITED)) {
        m_ignited = *val;
    }

    // powered (byte/bool) - 是否是高压苦力怕
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::POWERED)) {
        m_powered = *val;
    }

    return Result<void>::ok();
}

} // namespace mc
