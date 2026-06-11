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

#include <cmath>

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

namespace mc {

RabbitEntity::RabbitEntity(EntityId id)
    : AnimalEntity(id)
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
    return std::make_unique<RabbitEntity>(0);
}

void RabbitEntity::setRandomRabbitType()
{
    math::Random rng = getRandom();

    // 杀手兔有极小概率生成（1/1000）
    if (rng.nextInt(0, 999) == 0) {
        m_rabbitType = RabbitType::Killer;
        return;
    }

    // 根据当前群系确定兔子类型
    m_rabbitType = getDefaultRabbitTypeForBiome();
}

RabbitEntity::RabbitType RabbitEntity::getDefaultRabbitTypeForBiome() const
{
    // 获取当前位置的群系
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return RabbitType::Brown;
    }

    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    const ChunkData* chunk = worldPtr->getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return RabbitType::Brown;
    }

    BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());

    // 雪地群系：生成白色/白色斑点兔子
    // 参考 MC BiomeTags.SPAWNS_WHITE_RABBITS
    if (biomeId == Biomes::SnowyPlains || biomeId == Biomes::SnowyMountains || biomeId == Biomes::IceSpikes ||
        biomeId == Biomes::FrozenOcean || biomeId == Biomes::DeepFrozenOcean || biomeId == Biomes::FrozenRiver ||
        biomeId == Biomes::SnowyBeach || biomeId == Biomes::SnowyTaiga || biomeId == Biomes::SnowyTaigaHills ||
        biomeId == Biomes::SnowyTaigaMountains || biomeId == Biomes::FrozenPeaks || biomeId == Biomes::JaggedPeaks ||
        biomeId == Biomes::SnowySlopes || biomeId == Biomes::Grove) {
        math::Random rng = getRandom();
        return rng.nextInt(100) < 80 ? RabbitType::White : RabbitType::WhiteSpotted;
    }

    // 沙漠群系：生成金色兔子
    // 参考 MC BiomeTags.SPAWNS_GOLD_RABBITS
    if (biomeId == Biomes::Desert || biomeId == Biomes::DesertHills || biomeId == Biomes::DesertLakes) {
        return RabbitType::Gold;
    }

    // 其他群系：棕色/椒盐色/黑色
    math::Random rng = getRandom();
    i32 i = rng.nextInt(100);
    if (i < 50) {
        return RabbitType::Brown;
    }
    if (i < 90) {
        return RabbitType::SaltAndPepper;
    }
    return RabbitType::Black;
}

bool RabbitEntity::isBreedingItem(const ItemStack& itemStack) const
{
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
    auto baby = std::make_unique<RabbitEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 类型继承逻辑：5% 概率根据群系随机生成类型，95% 从父母继承
    // 参考 MC 1.21.11 Rabbit.getBreedOffspring
    math::Random rng = getRandom();
    RabbitType babyType;

    if (rng.nextInt(20) == 0) {
        // 5% 概率：根据父母所在位置的群系生成类型
        // 注意：此时 baby 尚未设置 world，因此使用父级的位置和群系
        babyType = getDefaultRabbitTypeForBiome();
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
    }
    baby->setRabbitType(babyType);

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
    // 调用父类方法
    AgeableEntity::registerGoals();

    // 兔子有特殊的 AI 行为（逃跑更快）
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（兔子逃跑速度更快）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 2.2));

    // 优先级 2: 逃离玩家（8格，速度2.2）- 杀手兔不逃离
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

    // 优先级 2: 逃离狼（10格，速度2.2）- 杀手兔不逃离
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            10.0f, // avoidDistance - 检测狼的距离
            2.2,   // farSpeed
            2.2,   // nearSpeed
            [this](const LivingEntity* entity) -> bool {
                if (isKillerRabbit()) return false;
                return entity->typeId() == entity::EntityTypeIdNumber::WOLF;
            }));

    // 优先级 2: 逃离怪物（4格，速度2.2）- 杀手兔不逃离
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            4.0f, // avoidDistance - 检测怪物的距离
            2.2,  // farSpeed
            2.2,  // nearSpeed
            [this](const LivingEntity* entity) -> bool {
                if (isKillerRabbit()) return false;
                // 检查是否是敌对生物
                return dynamic_cast<const MonsterEntity*>(entity) != nullptr;
            }));

    // 优先级 3: 繁殖
    m_goalSelector.addGoal(3, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 4: 食物诱惑（胡萝卜、金胡萝卜、蒲公英）
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
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
