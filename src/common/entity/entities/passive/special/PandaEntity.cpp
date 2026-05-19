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
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/gamerule/GameRules.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/special/PandaGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

namespace mc {

PandaEntity::PandaEntity(EntityId id)
    : AnimalEntity(id)
{
    // 随机生成性格
    randomizePersonality();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> PandaEntity::create(IWorld* /*world*/)
{
    return std::make_unique<PandaEntity>(0);
}

void PandaEntity::randomizePersonality()
{
    math::Random rng = getRandom();

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
    // MC 1.16.5: 检查物品是否为竹子
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
    // MC 1.16.5: Gene.func_221101_b()
    // 根据主基因和隐藏基因计算表达的性格
    //
    // MC 1.16.5 基因表达规则：
    // 1. 如果主基因是显性的（Aggressive），直接返回主基因
    // 2. 如果主基因是隐性的（Lazy、Worried、Playful、Weak、Brown、Normal）：
    //    a. 如果主基因是 Lazy 且隐藏基因是 Aggressive，返回 Aggressive
    //    b. 否则返回主基因
    //
    // 注意：AggressiveLazy 性格实际上在 MC 1.16.5 中未被使用

    u8 mainGene = m_mainGene;
    u8 hiddenGene = m_hiddenGene;

    // 隐性基因列表（非 Aggressive 的基因都是隐性的）
    constexpr u8 AGGRESSIVE = static_cast<u8>(Personality::Aggressive);
    constexpr u8 LAZY = static_cast<u8>(Personality::Lazy);

    // 如果主基因是好斗，直接返回好斗（显性）
    if (mainGene == AGGRESSIVE) {
        return Personality::Aggressive;
    }

    // 如果隐藏基因是好斗且主基因是懒惰，返回好斗（MC 1.16.5 特殊规则）
    if (mainGene == LAZY && hiddenGene == AGGRESSIVE) {
        return Personality::Aggressive;
    }

    // 默认返回主基因
    return static_cast<Personality>(mainGene);
}

u8 PandaEntity::getOneOfGenesRandomly(math::Random& rng) const
{
    // MC 1.16.5: return this.rand.nextBoolean() ? this.getMainGene() : this.getHiddenGene();
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

    // MC 1.16.5: 1/32 概率发生变异
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
    // MC 1.16.5: PandaEntity.func_241840_a()
    auto baby = std::make_unique<PandaEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置
    baby->setPosition(x(), y(), z());

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
        updateRoll();
    } else {
        m_rollTimer = 0;
    }

    // 更新打喷嚏计时器
    if (m_sneezing && m_sneezeTimer > 0) {
        m_sneezeTimer--;

        // MC 1.16.5: 第1 tick播放预喷嚏音效
        if (m_sneezeTimer == 19) {
            playPreSneezeSound();
        }

        if (m_sneezeTimer <= 0) {
            m_sneezing = false;
            // 打喷嚏完成，执行效果
            onSneezeComplete();
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
    // MC 1.16.5: this.goalSelector.addGoal(12, new PandaEntity.RollGoal(this));
    m_goalSelector.addGoal(12, std::make_unique<entity::ai::goal::PandaRollGoal>(this));
}

void PandaEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 熊猫的基础属性
    // 参考 MC 1.16.5 熊猫属性
    f32 maxHealth = 20.0f;

    // 虚弱熊猫生命值只有10
    if (isWeak()) {
        maxHealth = 10.0f;
    }

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, maxHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.15); // 熊猫移动较慢

    // 好斗熊猫攻击力更高
    if (isAggressive()) {
        m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
    }
}

std::optional<ResourceLocation> PandaEntity::getAmbientSound() const
{
    // MC 1.16.5: 根据性格返回不同音效
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

void PandaEntity::onSneezeComplete()
{
    // MC 1.16.5: PandaEntity.onSneeze()

    if (m_world == nullptr) {
        return;
    }

    // 1. 播放喷嚏音效
    playSneezeSound();

    // 2. 生成喷嚏粒子
    // MC 1.16.5: 粒子位置在熊猫头部前方
    // 位置计算：根据朝向偏移
    f32 renderYawOffset = m_yaw; // 使用yaw作为朝向
    f32 yawRad = math::toRadians(renderYawOffset);
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    // 粒子位置：熊猫眼睛高度前方
    f32 particleX = static_cast<f32>(x()) - (width() + 1.0f) * 0.5f * sinYaw;
    f32 particleY = static_cast<f32>(y()) + eyeHeight() - 0.1f;
    f32 particleZ = static_cast<f32>(z()) + (width() + 1.0f) * 0.5f * cosYaw;

    // 使用熊猫当前运动速度作为粒子速度
    Vector3 vel = velocity();
    m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Sneeze,
        Vector3(particleX, particleY, particleZ),
        Vector3(vel.x, 0.0f, vel.z));

    // 3. 让周围10格内的成年熊猫跳跃
    // MC 1.16.5: 获取周围10格内的熊猫
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
    // MC 1.16.5: if (!this.world.isRemote && this.rand.nextInt(700) == 0 &&
    // this.world.getGameRules().getBoolean(GameRules.DO_MOB_LOOT))
    if (!m_world->isClientSide()) {
        const auto& gameRules = m_world->getGameRules();
        if (gameRules.getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT)) {
            math::Random rng = getRandom();
            if (rng.nextInt(700) == 0) {
                // 使用 ItemDropHelper 在熊猫位置掉落粘液球
                ItemStack slimeBall(*Items::SLIME_BALL, 1);
                ItemDropHelper::spawnItemAtEntity(this, slimeBall, 0.5f, rng);
            }
        }
    }
}

void PandaEntity::updateRoll()
{
    // MC 1.16.5: PandaEntity.func_213535_ey()
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
            // MC 1.16.5:
            // float f = this.rotationYaw * ((float)Math.PI / 180F);
            // float f1 = this.isChild() ? 0.1F : 0.2F;
            // this.rollDelta = new Vector3d(vec3d.x + (double)(-MathHelper.sin(f) * f1), 0.0D, vec3d.z +
            // (double)(MathHelper.cos(f) * f1)); this.setMotion(this.rollDelta.add(0.0D, 0.27D, 0.0D));

            f32 yawRad = math::toRadians(yaw());
            f32 speed = isChild() ? ROLL_SPEED_CHILD : ROLL_SPEED_ADULT;
            f32 sinYaw = std::sin(yawRad);
            f32 cosYaw = std::cos(yawRad);

            m_rollVelocity = Vector3(vel.x + (-sinYaw * speed), 0.0, vel.z + (cosYaw * speed));

            // 设置初始速度（包含跳跃）
            setVelocity(m_rollVelocity.x, ROLL_JUMP_VELOCITY, m_rollVelocity.z);
        } else if (m_rollTimer == 7 || m_rollTimer == 15 || m_rollTimer == 23) {
            // 第7、15、23帧：执行小跳
            // MC 1.16.5:
            // this.setMotion(0.0D, this.onGround ? 0.27D : vec3d.y, 0.0D);
            f32 jumpVel = onGround() ? ROLL_JUMP_VELOCITY : static_cast<f32>(vel.y);
            setVelocity(0.0, jumpVel, 0.0);
        } else {
            // 其他帧：维持水平移动
            // MC 1.16.5:
            // this.setMotion(this.rollDelta.x, vec3d.y, this.rollDelta.z);
            setVelocity(m_rollVelocity.x, vel.y, m_rollVelocity.z);
        }
    }
}

} // namespace mc
