#include "PandaEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../sound/SoundEvents.hpp"

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

std::unique_ptr<Entity> PandaEntity::create(IWorld* /*world*/) {
    return std::make_unique<PandaEntity>(LegacyEntityType::Unknown, 0);
}

void PandaEntity::randomizePersonality() {
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

bool PandaEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是竹子
    // return itemStack.getItem() == Items::BAMBOO;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> PandaEntity::spawnBaby(AnimalEntity& partner) {
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

void PandaEntity::tick() {
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
        if (m_sneezeTimer <= 0) {
            m_sneezing = false;
            // TODO: 生成粘液球
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

void PandaEntity::registerGoals() {
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

void PandaEntity::registerAttributes() {
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

std::optional<ResourceLocation> PandaEntity::getAmbientSound() const {
    // MC 1.16.5: 根据性格返回不同音效
    if (isAggressive()) {
        return SoundEvents::ENTITY_PANDA_AGGRESSIVE_AMBIENT;
    }
    if (isWorried()) {
        return SoundEvents::ENTITY_PANDA_WORRIED_AMBIENT;
    }
    return SoundEvents::ENTITY_PANDA_AMBIENT;
}

std::optional<ResourceLocation> PandaEntity::getHurtSound(DamageSource& /*source*/) const {
    return SoundEvents::ENTITY_PANDA_HURT;
}

std::optional<ResourceLocation> PandaEntity::getDeathSound() const {
    return SoundEvents::ENTITY_PANDA_DEATH;
}

void PandaEntity::playEatSound() {
    playSound(SoundEvents::ENTITY_PANDA_EAT, 1.0f, 1.0f);
}

void PandaEntity::playSneezeSound() {
    playSound(SoundEvents::ENTITY_PANDA_SNEEZE, 1.0f, 1.0f);
}

void PandaEntity::playBiteSound() {
    playSound(SoundEvents::ENTITY_PANDA_BITE, 1.0f, 1.0f);
}

} // namespace mc
