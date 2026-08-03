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

#include "ChickenEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace mc {

std::unique_ptr<Entity> ChickenEntity::create(IWorld* /*world*/)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<ChickenEntity>(0);
}

ChickenEntity::ChickenEntity(EntityInstanceId id)
    : AnimalEntity(id)
{
    // 注册属性
    registerAttributes();
    // 注册 AI 目标
    registerGoals();
    // 初始化下蛋计时器
    resetEggTimer();
}

void ChickenEntity::resetEggTimer()
{
    math::Random rng(ticksExisted());
    // 下蛋时间：6000-12000 ticks = 5-10 分钟
    m_eggTimer = EGG_TIME_MIN + rng.nextInt(EGG_TIME_MAX - EGG_TIME_MIN);
}

std::optional<ResourceLocation> ChickenEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> ChickenEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> ChickenEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> ChickenEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

void ChickenEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 鸡播放固定的脚步声，忽略脚下方块类型
    auto sound = getStepSound();
    if (sound) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

bool ChickenEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 鸡用种子繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT_SEEDS || item == Items::PUMPKIN_SEEDS || item == Items::MELON_SEEDS ||
        item == Items::BEETROOT_SEEDS;
}

bool ChickenEntity::canMateWith(const AnimalEntity& other) const
{
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> ChickenEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建小鸡
    auto baby = std::make_unique<ChickenEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void ChickenEntity::registerGoals()
{
    // 调用父类方法（AgeableEntity 会调用 AnimalEntity，现在 AnimalEntity 不注册任何目标）
    AgeableEntity::registerGoals();

    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.4));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 种子诱惑
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr &&
                    (item == Items::WHEAT_SEEDS || item == Items::PUMPKIN_SEEDS || item == Items::MELON_SEEDS ||
                        item == Items::BEETROOT_SEEDS);
            },
            false)); // scaredByMovement = false

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void ChickenEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 鸡的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 4.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

void ChickenEntity::tick()
{
    // 保存上一帧翅膀角度
    m_prevWingRotation = m_wingRotation;

    AnimalEntity::tick();

    // 翅膀动画
    constexpr f32 WING_FLAP_SPEED = 1.0f;
    constexpr f32 WING_DAMPING = 0.9f;
    m_wingRotDelta += WING_FLAP_SPEED;
    m_wingRotation += m_wingRotDelta;
    m_wingRotDelta *= WING_DAMPING;

    // 下蛋逻辑（仅服务端、仅成体、非鸡骑士）
    if (!isChild() && !m_chickenJockey && m_eggTimer > 0) {
        --m_eggTimer;

        if (m_eggTimer <= 0 && world() != nullptr) {
            // 下蛋
            auto egg = std::make_unique<ItemEntity>(0, ItemStack(Items::EGG, 1), x(), y() + 0.2f, z());

            // 播放下蛋音效
            playSound(
                *makeSoundEventId("egg"), 1.0f, (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f + 1.0f);

            world()->spawnEntity(std::move(egg));
            resetEggTimer();
        }
    }
}

} // namespace mc
