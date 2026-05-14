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
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../../world/gamerule/GameRules.hpp"
#include "../../../../world/IWorld.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

namespace mc {

PandaEntity::PandaEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
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
    return std::make_unique<PandaEntity>(LegacyEntityType::Unknown, 0);
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
    // TODO: 检查是否是竹子
    // return itemStack.getItem() == Items::BAMBOO;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> PandaEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 创建小熊猫
    // auto baby = std::make_unique<PandaEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    //
    // // 遗传基因
    // PandaEntity* parent = dynamic_cast<PandaEntity*>(&partner);
    // if (parent) {
    //     // 主基因从父母随机遗传
    //     static std::random_device rd;
    //     static std::mt19937 gen(rd());
    //     std::uniform_int_distribution<int> dist(0, 1);
    //     baby->m_mainGene = dist(gen) == 0 ? m_mainGene : parent->m_mainGene;
    //     baby->m_hiddenGene = dist(gen) == 0 ? m_hiddenGene : parent->m_hiddenGene;
    //
    //     // 根据基因计算性格
    //     // 变异概率 1/32
    //     if (dist(gen) == 0 && dist(gen) == 0) {
    //         std::uniform_int_distribution<u8> geneDist(0, 5);
    //         baby->m_mainGene = geneDist(gen);
    //     }
    // }
    //
    // return baby;
    (void)partner;
    return nullptr;
}

void PandaEntity::tick()
{
    AnimalEntity::tick();

    // 更新各种状态计时器
    if (m_rolling && m_rollTimer > 0) {
        m_rollTimer--;
        if (m_rollTimer <= 0) {
            m_rolling = false;
        }
    }

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

    if (m_eating && m_eatTimer > 0) {
        m_eatTimer--;
        if (m_eatTimer <= 0) {
            m_eating = false;
        }
    }

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

    // TODO: 熊猫特有目标
    // - PandaRollGoal: 打滚（顽皮熊猫）
    // - PandaSneezeGoal: 打喷嚏（幼体）
    // - PandaLieGoal: 躺下（懒惰熊猫）
    // - PandaEatBambooGoal: 吃竹子
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
    m_world->addParticle(
        client::renderer::trident::particle::ParticleTypeId::Sneeze,
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
    // MC 1.16.5: if (!this.world.isRemote && this.rand.nextInt(700) == 0 && this.world.getGameRules().getBoolean(GameRules.DO_MOB_LOOT))
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

} // namespace mc
