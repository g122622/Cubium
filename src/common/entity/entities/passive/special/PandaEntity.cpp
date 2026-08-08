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

#include "PandaEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/special/PandaGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

PandaEntity::PandaEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 随机生成性格
    randomizePersonality();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> PandaEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PandaEntity>(0, registry);
}

void PandaEntity::randomizePersonality()
{
    math::Random& rng = getRandom();

    // 熊猫性格概率分布
    // 普通: 32%, 懒惰: 32%, 忧愁: 16%, 顽皮: 16%, 好斗: 1.6%, 虚弱: 0.08%, 棕色: 2.4%
    int value = rng.nextInt(0, 1249);

    if (value == 0) {
        // 虚弱（极稀有）
        m_personality = Personality::Weak;
        m_mainGene = 5;
    } else if (value <= 19) {
        // 好斗（稀有）
        m_personality = Personality::Aggressive;
        m_mainGene = 4;
    } else if (value <= 49) {
        // 棕色（较稀有）
        m_personality = Personality::Brown;
        m_mainGene = 6;
    } else if (value <= 249) {
        // 忧愁
        m_personality = Personality::Worried;
        m_mainGene = 2;
    } else if (value <= 449) {
        // 顽皮
        m_personality = Personality::Playful;
        m_mainGene = 3;
    } else if (value <= 849) {
        // 懒惰
        m_personality = Personality::Lazy;
        m_mainGene = 1;
    } else {
        // 普通
        m_personality = Personality::Normal;
        m_mainGene = 0;
    }

    // 隐藏基因随机
    m_hiddenGene = static_cast<u8>(rng.nextInt(0, 5));
}

bool PandaEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 检查物品是否为竹子
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // Items::BAMBOO 可能在初始化期间为 nullptr
    if (Items::BAMBOO == nullptr) {
        return false;
    }
    return item == Items::BAMBOO;
}

// ========== 基因系统实现 ==========

PandaEntity::Personality PandaEntity::calculateExpressedPersonality() const
{
    // 根据主基因和隐藏基因计算表达的性格
    // 基因表达规则：
    // 1. 如果主基因是显性的（Aggressive），直接返回主基因
    // 2. 如果主基因是隐性的（Lazy、Worried、Playful、Weak、Brown、Normal）：
    //    a. 如果主基因是 Lazy 且隐藏基因是 Aggressive，返回 Aggressive
    //    b. 否则返回主基因
    // 注意：AggressiveLazy 性格实际上未被使用

    u8 mainGene = m_mainGene;
    u8 hiddenGene = m_hiddenGene;

    // 隐性基因列表（非 Aggressive 的基因都是隐性的）
    constexpr u8 AGGRESSIVE = static_cast<u8>(Personality::Aggressive);
    constexpr u8 LAZY = static_cast<u8>(Personality::Lazy);

    // 如果主基因是好斗，直接返回好斗（显性）
    if (mainGene == AGGRESSIVE) {
        return Personality::Aggressive;
    }

    // 如果隐藏基因是好斗且主基因是懒惰，返回好斗（特殊规则）
    if (mainGene == LAZY && hiddenGene == AGGRESSIVE) {
        return Personality::Aggressive;
    }

    // 默认返回主基因
    return static_cast<Personality>(mainGene);
}

u8 PandaEntity::getOneOfGenesRandomly(math::Random& rng) const
{
    return rng.nextBoolean() ? m_mainGene : m_hiddenGene;
}

void PandaEntity::inheritGenesFromParents(PandaEntity* father, PandaEntity* mother)
{
    if (m_world == nullptr) {
        return;
    }

    math::Random& rng = m_world->getRandom();

    if (mother == nullptr) {
        // 只有父亲时，随机分配父亲的一个基因
        if (rng.nextBoolean()) {
            m_mainGene = father->getOneOfGenesRandomly(rng);
            m_hiddenGene = static_cast<u8>(rng.nextInt(0, 5)); // 随机隐藏基因
        } else {
            m_mainGene = static_cast<u8>(rng.nextInt(0, 5)); // 随机主基因
            m_hiddenGene = father->getOneOfGenesRandomly(rng);
        }
    } else {
        // 双亲都有时，随机从父母各取一个基因
        if (rng.nextBoolean()) {
            m_mainGene = father->getOneOfGenesRandomly(rng);
            m_hiddenGene = mother->getOneOfGenesRandomly(rng);
        } else {
            m_mainGene = mother->getOneOfGenesRandomly(rng);
            m_hiddenGene = father->getOneOfGenesRandomly(rng);
        }
    }

    // 1/32 概率发生变异
    if (rng.nextInt(32) == 0) {
        m_mainGene = static_cast<u8>(rng.nextInt(0, 5));
    }

    if (rng.nextInt(32) == 0) {
        m_hiddenGene = static_cast<u8>(rng.nextInt(0, 5));
    }

    // 更新表达性格
    updatePersonalityFromGenes();
}

void PandaEntity::updatePersonalityFromGenes()
{
    m_personality = calculateExpressedPersonality();
}

std::unique_ptr<AnimalEntity> PandaEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = m_world->entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto baby = std::make_unique<PandaEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置
    baby->setPosition(x(), y(), z());

    // 先把父方世界挂给幼体，inheritGenesFromParents 需 m_world->getRandom()；
    // 否则 m_world==nullptr 时会提前返回，幼体保留构造时 randomizePersonality 的随机基因，
    // 基因遗传失效（对齐 MC 1.21.11 Panda.getBreedOffspring：create 传入 ServerLevel 使子代有世界）。
    baby->setWorld(world());

    // 遗传基因
    PandaEntity* parent = dynamic_cast<PandaEntity*>(&partner);
    baby->inheritGenesFromParents(this, parent);

    return baby;
}

void PandaEntity::tick()
{
    AnimalEntity::tick();

    // 更新打滚物理
    if (m_rolling) {
        _updateRoll();
    } else {
        m_rollTimer = 0;
    }

    // 更新打喷嚏计时器
    if (m_sneezing && m_sneezeTimer > 0) {
        m_sneezeTimer--;

        // 第1 tick播放预喷嚏音效
        if (m_sneezeTimer == 19) {
            playPreSneezeSound();
        }

        if (m_sneezeTimer <= 0) {
            m_sneezing = false;
            // 打喷嚏完成，执行效果
            _onSneezeComplete();
        }
    }

    // 更新吃东西计时器
    if (m_eating && m_eatTimer > 0) {
        m_eatTimer--;
        if (m_eatTimer <= 0) {
            m_eating = false;
        }
    }

    // 更新躺着计时器
    if (m_lying && m_lyingTimer > 0) {
        m_lyingTimer--;
        if (m_lyingTimer <= 0) {
            m_lying = false;
        }
    }
}

void PandaEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 熊猫特有目标
    // 优先级 3: 食物诱惑（竹子）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isBambooPredicate));

    // 优先级 12: 打滚目标（顽皮熊猫或幼年熊猫）
    m_goalSelector.addGoal(12, std::make_unique<entity::ai::goal::PandaRollGoal>(this));
}

void PandaEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 熊猫的基础属性
    f32 maxHealth = 20.0f;

    // 虚弱熊猫生命值只有10
    if (isWeak()) {
        maxHealth = 10.0f;
    }

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, maxHealth);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.15); // 熊猫移动较慢

    // 好斗熊猫攻击力更高
    if (isAggressive()) {
        attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
    }
}

std::optional<ResourceLocation> PandaEntity::getAmbientSound() const
{
    // 根据性格返回不同音效
    if (isAggressive()) {
        return SoundEvents::ENTITY_PANDA_AGGRESSIVE_AMBIENT;
    }
    if (isWorried()) {
        return SoundEvents::ENTITY_PANDA_WORRIED_AMBIENT;
    }
    return SoundEvents::ENTITY_PANDA_AMBIENT;
}

std::optional<ResourceLocation> PandaEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_PANDA_HURT;
}

std::optional<ResourceLocation> PandaEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_PANDA_DEATH;
}

void PandaEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_PANDA_EAT, 1.0f, 1.0f);
}

void PandaEntity::playSneezeSound()
{
    playSound(SoundEvents::ENTITY_PANDA_SNEEZE, 1.0f, 1.0f);
}

void PandaEntity::playPreSneezeSound()
{
    playSound(SoundEvents::ENTITY_PANDA_PRE_SNEEZE, 1.0f, 1.0f);
}

void PandaEntity::playBiteSound()
{
    playSound(SoundEvents::ENTITY_PANDA_BITE, 1.0f, 1.0f);
}

void PandaEntity::_onSneezeComplete()
{
    if (m_world == nullptr) {
        return;
    }

    // 1. 播放喷嚏音效
    playSneezeSound();

    // 2. 生成喷嚏粒子
    // 粒子位置在熊猫头部前方
    f32 renderYawOffset = m_builtIn.rotation->m_rot.x; // 使用yaw作为朝向
    f32 yawRad = math::toRadians(renderYawOffset);
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    // 粒子位置：熊猫眼睛高度前方
    f32 particleX = static_cast<f32>(x()) - (width() + 1.0f) * 0.5f * sinYaw;
    f32 particleY = static_cast<f32>(y()) + eyeHeight() - 0.1f;
    f32 particleZ = static_cast<f32>(z()) + (width() + 1.0f) * 0.5f * cosYaw;

    // 使用熊猫当前运动速度作为粒子速度
    Vector3 vel = velocity();
    m_world->addParticle(
        particle::ParticleTypeId::Sneeze, Vector3(particleX, particleY, particleZ), Vector3(vel.x, 0.0f, vel.z));

    // 3. 让周围10格内的成年熊猫跳跃
    AxisAlignedBB searchBox = boundingBox().expand(10.0f, 10.0f, 10.0f);
    std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : nearbyEntities) {
        // 检查是否是熊猫
        auto* panda = dynamic_cast<PandaEntity*>(entity);
        if (panda != nullptr && !panda->isChild() && panda->onGround() && !panda->isInWater()) {
            // 成年熊猫跳起来
            panda->jump();
        }
    }

    // 4. 1/700概率掉落粘液球（需要游戏规则 doMobLoot）
    if (!m_world->isClientSide()) {
        const auto& gameRules = m_world->getGameRules();
        if (gameRules.getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT)) {
            math::Random& rng = getRandom();
            if (rng.nextInt(700) == 0) {
                // 使用 ItemDropHelper 在熊猫位置掉落粘液球
                ItemStack slimeBall(*Items::SLIME_BALL, 1);
                ItemDropHelper::spawnItemAtEntity(this, slimeBall, 0.5f, rng);
            }
        }
    }
}

void PandaEntity::_updateRoll()
{
    // 处理打滚物理
    m_rollTimer++;

    if (m_rollTimer > ROLL_DURATION) {
        // 打滚超过 32 ticks 后结束
        m_rolling = false;
        m_rollTimer = 0;
        return;
    }

    if (m_world != nullptr && !m_world->isClientSide()) {
        Vector3 vel = velocity();

        if (m_rollTimer == 1) {
            // 第1帧：初始化打滚方向和初速度
            f32 yawRad = math::toRadians(yaw());
            f32 speed = isChild() ? ROLL_SPEED_CHILD : ROLL_SPEED_ADULT;
            f32 sinYaw = std::sin(yawRad);
            f32 cosYaw = std::cos(yawRad);

            m_rollVelocity = Vector3(vel.x + (-sinYaw * speed), 0.0, vel.z + (cosYaw * speed));

            // 设置初始速度（包含跳跃）
            setVelocity(m_rollVelocity.x, ROLL_JUMP_VELOCITY, m_rollVelocity.z);
        } else if (m_rollTimer == 7 || m_rollTimer == 15 || m_rollTimer == 23) {
            // 第7、15、23帧：执行小跳
            f32 jumpVel = onGround() ? ROLL_JUMP_VELOCITY : static_cast<f32>(vel.y);
            setVelocity(0.0, jumpVel, 0.0);
        } else {
            // 其他帧：维持水平移动
            setVelocity(m_rollVelocity.x, vel.y, m_rollVelocity.z);
        }
    }
}

} // namespace mc
