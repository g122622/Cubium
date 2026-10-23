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

#include "PufferfishEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// DataParameter 定义
entity::DataParameter<i32> PufferfishEntity::DATA_PUFF_STATE_PARAM = entity::EntityDataManager::createKey<i32>();

const entity::EntityClassInfo& PufferfishEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"PufferfishEntity", &AbstractFishEntity::classInfo()};
    return s_classInfo;
}

PufferfishEntity::PufferfishEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractFishEntity(id, registry)
{
    // 显式调用 registerData() 注册 DATA_PUFF_STATE（C++ 基类构造期虚函数不派发，
    // AbstractFishEntity 构造函数已调，但本类有自身字段须由本类 override 注册）。
    registerData();

    // 补调 registerGoals / registerAttributes：AbstractFishEntity 构造调基类版（vtable 指向基类），
    // 派生 override 永不执行，须在派生类构造显式调用。Pufferfish 的 registerGoals 加专属 PuffGoal，
    // registerAttributes 设 MAX_HEALTH=3。
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> PufferfishEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PufferfishEntity>(0, registry);
}

f32 PufferfishEntity::getPuffSize() const
{
    // 返回碰撞箱缩放因子，基础尺寸为 0.7 x 0.7
    switch (m_puffState) {
        case PuffState::Deflated:
            return 0.5f; // 0.7 * 0.5 = 0.35
        case PuffState::SemiPuffed:
            return 0.7f; // 0.7 * 0.7 = 0.49
        case PuffState::FullyPuffed:
            return 1.0f; // 0.7 * 1.0 = 0.7
        default:
            return 0.5f;
    }
}

entity::EntitySize PufferfishEntity::getDimensions(EntityPose /*pose*/) const
{
    // 根据膨胀状态动态计算尺寸，基础尺寸 0.7 x 0.7
    f32 scale = getPuffSize();
    return entity::EntitySize::flexible(0.7f * scale, 0.7f * scale);
}

void PufferfishEntity::registerGoals()
{
    AbstractFishEntity::registerGoals();

    // 注册 PuffGoal，优先级 1，检测附近敌人并触发膨胀
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PuffGoal>(this));
}

void PufferfishEntity::tick()
{
    // 膨胀状态机对齐 vanilla Pufferfish.tick：先推进膨胀/收缩计时器（进出膨胀音效由
    // setPuffState 播放），再调用 super.tick（AbstractFishEntity::tick）。
    if (m_puffTimer > 0) {
        // 膨胀逻辑：当 puffTimer == 1 时，从状态 0 变为状态 1
        if (m_puffState == PuffState::Deflated && m_puffTimer == 1) {
            setPuffState(PuffState::SemiPuffed);
        }
        // 当 puffTimer > 40 且状态为 1 时，从状态 1 变为状态 2
        else if (m_puffTimer > PUFF_SEMI_THRESHOLD && m_puffState == PuffState::SemiPuffed) {
            setPuffState(PuffState::FullyPuffed);
        }

        ++m_puffTimer;
    } else if (m_puffState != PuffState::Deflated) {
        // 收缩逻辑
        ++m_deflateTimer;

        if (m_deflateTimer > DEFLATE_FULL_TO_SEMI && m_puffState == PuffState::FullyPuffed) {
            setPuffState(PuffState::SemiPuffed);
        } else if (m_deflateTimer > DEFLATE_SEMI_TO_DEFLATE && m_puffState == PuffState::SemiPuffed) {
            setPuffState(PuffState::Deflated);
        }
    }

    AbstractFishEntity::tick();
}

void PufferfishEntity::aiStep()
{
    // 对应 vanilla Pufferfish.aiStep：膨胀状态下每 tick 刺伤碰撞箱扩展 0.3 格内符合
    // Pufferfish.TARGETING_CONDITIONS 的生物（见 _attackNearbyEnemies）。
    AbstractFishEntity::aiStep();

    if (m_puffState != PuffState::Deflated) {
        _attackNearbyEnemies();
    }
}

void PufferfishEntity::startPuffTimer()
{
    // 设置 puffTimer = 1 并重置 deflateTimer = 0
    m_puffTimer = 1;
    m_deflateTimer = 0;
}

void PufferfishEntity::resetPuffTimer()
{
    // 重置膨胀计时器
    m_puffTimer = 0;
}

void PufferfishEntity::_attackNearbyEnemies()
{
    // 对齐 vanilla Pufferfish.aiStep：膨胀状态下遍历碰撞箱扩展 0.3 格内的 Mob，
    // 以 Pufferfish.TARGETING_CONDITIONS（forNonCombat + ignoreInvisibilityTesting +
    // ignoreLineOfSight + SCARY_MOB 选择器）过滤后刺伤。
    // 注意目标类型是 Mob（不含 Player）：玩家接触伤害由 playerTouch 单独处理。
    if (!isAlive() || !world()) {
        return;
    }

    const AxisAlignedBB searchBox = boundingBox().grow(0.3);
    const std::vector<Entity*> entities = world()->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // 目标类型为 Mob（vanilla getEntitiesOfClass(Mob.class, ...)）。
        auto* mob = dynamic_cast<MobEntity*>(entity);
        if (mob == nullptr) {
            continue;
        }

        if (!isScaryMob(*mob)) {
            continue;
        }

        // 伤害 = 1 + puffState，命中后附加 60*puffState tick 的中毒 I。
        EntityDamageSource damageSource = DamageSources::mobAttack(this);
        const i32 puffState = static_cast<i32>(m_puffState);
        if (mob->hurt(damageSource, static_cast<f32>(1 + puffState))) {
            mob->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison,
                60 * puffState,
                0,     // amplifier (0 = Poison I)
                false, // ambient
                true   // visible
                ));
            playSound(SoundEvents::ENTITY_PUFFER_FISH_STING, 1.0f, 1.0f);
        }
    }
}

bool PufferfishEntity::isScaryMob(const LivingEntity& entity)
{
    // 对齐 vanilla 1.21.11 Pufferfish.SCARY_MOB：
    //   e instanceof Player p && p.isCreative() ? false : !e.getType().is(NOT_SCARY_FOR_PUFFERFISH)
    // 膨胀触发（PuffGoal）与刺伤（aiStep）共用本判定，避免两处硬编码列表分叉。
    // 另：TargetingConditions.forNonCombat 在动作方非空时仍要求 canBeSeenByAnyone()
    // （= !isSpectator() && isAlive()），故旁观者不会触发膨胀。
    // 标签未初始化（极早期）时成员集为空会把水生生物误判为敌人，故保守返回"不惧怕"。
    if (entity.isSpectator()) {
        return false;
    }
    if (const auto* player = dynamic_cast<const Player*>(&entity); player != nullptr && player->isCreative()) {
        return false;
    }
    if (!EntityTypeTags::isInitialized()) {
        return false;
    }
    return !EntityTypeTags::NOT_SCARY_FOR_PUFFERFISH().contains(entity.getTypeId());
}

std::optional<ResourceLocation> PufferfishEntity::getAmbientSound() const
{
    // 对齐原版 Pufferfish（继承 AbstractFish，不 override → 默认不播放水中环境音）。
    // sounds.json 中无 entity.puffer_fish.ambient，仅在陆地播放扑腾音 FLOP。
    // 先前在水中返回 ENTITY_PUFFER_FISH_AMBIENT 会触发 "Sound event not found" 告警。
    if (!isInWater()) {
        return SoundEvents::ENTITY_PUFFER_FISH_FLOP;
    }
    return std::nullopt;
}

std::optional<ResourceLocation> PufferfishEntity::getFlopSound() const
{
    return SoundEvents::ENTITY_PUFFER_FISH_FLOP;
}

std::optional<ResourceLocation> PufferfishEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_PUFFER_FISH_DEATH;
}

std::optional<ResourceLocation> PufferfishEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_PUFFER_FISH_HURT;
}

void PufferfishEntity::registerAttributes()
{
    AbstractFishEntity::registerAttributes();

    // 河豚属性
    // 原版 Pufferfish 不覆盖移动速度（沿用 AbstractFish/LivingEntity 的 0.7）。
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
}

void PufferfishEntity::registerData()
{
    AbstractFishEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册膨胀状态同步参数
    m_dataManager.registerParam(DATA_PUFF_STATE_PARAM, 0);
}

PufferfishEntity::PuffState PufferfishEntity::getPuffState() const
{
    // 优先从 DataParameter 读取同步值
    if (m_dataManager.hasParam(DATA_PUFF_STATE_PARAM.id())) {
        const i32 value = m_dataManager.get<i32>(DATA_PUFF_STATE_PARAM);
        const i32 clamped = std::clamp(value, 0, 2);
        return static_cast<PuffState>(clamped);
    }
    return m_puffState;
}

void PufferfishEntity::setPuffState(PuffState state)
{
    if (state == m_puffState) {
        return;
    }

    PuffState oldState = m_puffState;
    m_puffState = state;

    // 通过 DataParameter 同步到客户端
    m_dataManager.set(DATA_PUFF_STATE_PARAM, static_cast<i32>(state));

    // 膨胀时播放 BLOW_UP 音效，收缩时播放 BLOW_OUT 音效
    if (static_cast<i32>(state) > static_cast<i32>(oldState)) {
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_UP, 1.0f, 1.0f);
    } else {
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_OUT, 1.0f, 1.0f);
    }

    // 刷新碰撞箱尺寸
    refreshDimensions();
}

ItemStack PufferfishEntity::getBucketItemStack() const
{
    return ItemStack(Items::PUFFERFISH_BUCKET, 1);
}

} // namespace mc
