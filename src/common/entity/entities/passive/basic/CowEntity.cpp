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

#include "CowEntity.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp" // 包含 LookRandomlyGoal
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <optional>

namespace mc {

std::unique_ptr<Entity> CowEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<CowEntity>(0, registry);
}

CowEntity::CowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 补调 registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），派生 override
    // 永不执行，须在派生类构造显式调用。Cow 的 registerAttributes 设 MAX_HEALTH=10 等。
    registerAttributes();
}

std::optional<ResourceLocation> CowEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> CowEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> CowEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> CowEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

void CowEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 牛播放固定的脚步声，忽略脚下方块类型
    auto sound = getStepSound();
    if (sound) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

bool CowEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 牛用小麦繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT;
}

bool CowEntity::canMateWith(const AnimalEntity& other) const
{
    // 检查是否是牛
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> CowEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建小牛
    auto baby = std::make_unique<CowEntity>(0, *registry);
    baby->setTypeId(entity::EntityTypeKeys::COW); // 工厂绕过补救：直接构造缺 typeId

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void CowEntity::registerGoals()
{
    // 调用父类方法
    AgeableEntity::registerGoals();

    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.25));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（小麦）
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr && item == Items::WHEAT;
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

void CowEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 牛的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

} // namespace mc
