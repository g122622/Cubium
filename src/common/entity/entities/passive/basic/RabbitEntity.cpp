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

#include "RabbitEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/block/VanillaBlocks.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp" // 包含 LookRandomlyGoal
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../entities/monster/MonsterEntity.hpp"
#include "../../player/Player.hpp"

namespace mc {

RabbitEntity::RabbitEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 随机设置皮肤类型
    setRandomRabbitType();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> RabbitEntity::create(IWorld* /*world*/)
{
    return std::make_unique<RabbitEntity>(LegacyEntityType::Unknown, 0);
}

void RabbitEntity::setRandomRabbitType()
{
    math::Random rng = getRandom();

    // 杀手兔有极小概率生成（1/1000）
    if (rng.nextInt(0, 999) == 0) {
        m_rabbitType = RabbitType::Killer;
        return;
    }

    // 正常皮肤随机
    m_rabbitType = static_cast<RabbitType>(rng.nextInt(0, 5));
}

bool RabbitEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: RabbitEntity.isBreedingItem()
    // 兔子用胡萝卜、金胡萝卜、蒲公英繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;

    // 检查胡萝卜和金胡萝卜
    if (item == Items::CARROT || item == Items::GOLDEN_CARROT) {
        return true;
    }

    // 检查蒲公英（方块物品）
    // DANDELION 是方块，需要通过 BlockItemRegistry 获取对应的物品
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    if (block != nullptr && block == VanillaBlocks::DANDELION) {
        return true;
    }

    return false;
}

std::unique_ptr<AnimalEntity> RabbitEntity::spawnBaby(AnimalEntity& partner)
{
    // MC 1.16.5: RabbitEntity.createChild()
    auto baby = std::make_unique<RabbitEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // MC 1.16.5: 类型继承逻辑
    // 5% 概率随机生成类型（根据群系），95% 从父母继承
    math::Random rng = getRandom();
    RabbitType babyType;

    if (rng.nextInt(20) == 0) {
        // 5% 概率：随机类型（实际应该根据群系决定，这里简化处理）
        baby->setRandomRabbitType();
        babyType = baby->getRabbitType();
    } else {
        // 95% 概率：从父母继承
        // 50% 概率继承自己，50% 概率继承配偶
        if (rng.nextBoolean()) {
            babyType = m_rabbitType;
        } else {
            // 尝试从配偶获取类型
            RabbitEntity* partnerRabbit = dynamic_cast<RabbitEntity*>(&partner);
            if (partnerRabbit != nullptr) {
                babyType = partnerRabbit->getRabbitType();
            } else {
                babyType = m_rabbitType;
            }
        }
        baby->setRabbitType(babyType);
    }

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

void RabbitEntity::setJumping(bool jumping)
{
    LivingEntity::setJumping(jumping);

    if (!jumping) {
        return;
    }

    auto soundEvent = makeSoundEventId("jump");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random random = getRandom();
    playSound(*soundEvent, getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
}

sound::SoundCategory RabbitEntity::getSoundCategory() const
{
    return isKillerRabbit() ? sound::SoundCategory::Hostile : sound::SoundCategory::Neutral;
}

void RabbitEntity::playAttackSound(LivingEntity& /*target*/)
{
    if (!isKillerRabbit()) {
        return;
    }

    auto soundEvent = makeSoundEventId("attack");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random random = getRandom();
    playSound(*soundEvent, 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
}

void RabbitEntity::registerGoals()
{
    // 调用父类方法（AgeableEntity 会调用 AnimalEntity，现在 AnimalEntity 不注册任何目标）
    AgeableEntity::registerGoals();

    // MC 1.16.5 RabbitEntity.registerGoals()
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表
    // 兔子有特殊的 AI 行为（逃跑更快）

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（兔子逃跑速度更快）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 2.2));

    // MC 1.16.5: 兔子逃离玩家、狼和怪物（杀手兔不逃离）
    // 优先级 2: 逃离玩家（8格，速度2.2）
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            8.0f, // avoidDistance - 检测玩家的距离
            2.2,  // farSpeed - 远距离逃跑速度
            2.2,  // nearSpeed - 近距离逃跑速度
            [this](const LivingEntity* entity) -> bool {
                // 杀手兔不逃离
                if (isKillerRabbit()) return false;
                // 检查是否是玩家
                return dynamic_cast<const Player*>(entity) != nullptr;
            }));

    // 优先级 2: 逃离狼（10格，速度2.2）
    // 注意：狼是 WolfEntity，需要检查 LegacyEntityType::Wolf
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            10.0f, // avoidDistance - 检测狼的距离
            2.2,   // farSpeed
            2.2,   // nearSpeed
            [this](const LivingEntity* entity) -> bool {
                if (isKillerRabbit()) return false;
                return entity->legacyType() == LegacyEntityType::Wolf;
            }));

    // 优先级 2: 逃离怪物（4格，速度2.2）
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            4.0f, // avoidDistance - 检测怪物的距离
            2.2,  // farSpeed
            2.2,  // nearSpeed
            [this](const LivingEntity* entity) -> bool {
                if (isKillerRabbit()) return false;
                // 检查是否是敌对生物（MonsterEntity 的子类）
                // MC 1.16.5: MonsterEntity.class
                return dynamic_cast<const MonsterEntity*>(entity) != nullptr;
            }));

    // 优先级 3: 繁殖
    m_goalSelector.addGoal(3, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 4: 食物诱惑（胡萝卜、金胡萝卜、蒲公英）
    // MC 1.16.5: TemptGoal 使用 TemptGoal(this, 1.0D, Ingredient.fromItems(Items.CARROT, Items.GOLDEN_CARROT,
    // Blocks.DANDELION), false)
    m_goalSelector.addGoal(4,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                if (item == nullptr) return false;

                // 胡萝卜和金胡萝卜
                if (item == Items::CARROT || item == Items::GOLDEN_CARROT) {
                    return true;
                }

                // 蒲公英（方块物品）
                const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
                if (block != nullptr && block == VanillaBlocks::DANDELION) {
                    return true;
                }

                return false;
            },
            false)); // scaredByMovement = false

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 6: 随机漫步
    m_goalSelector.addGoal(6, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));
}

void RabbitEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 兔子的属性
    // 参考 MC 1.16.5 兔子属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
