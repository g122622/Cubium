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

#include "TurtleEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/BlockTags.hpp"
#include "../../../../world/block/VanillaBlocks.hpp"
#include "../../../../world/block/blocks/mob/TurtleEggBlock.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <cmath>

namespace mc {

TurtleEntity::TurtleEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // MC 1.16.5: TurtleEntity 构造函数中设置 stepHeight = 1.0F
    // 海龟可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> TurtleEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TurtleEntity>(LegacyEntityType::Unknown, 0);
}

void TurtleEntity::setHomePos(const BlockPos& pos)
{
    m_homePos = pos;
    m_hasHomePos = true;
}

bool TurtleEntity::isInWater() const
{
    // MC 1.16.5: 海龟在水中
    return Entity::isInWater();
}

bool TurtleEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // TODO: 检查是否是海草
    // return itemStack.getItem() == Items::SEAGRASS;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> TurtleEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 创建小海龟
    // auto baby = std::make_unique<TurtleEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // baby->setHomePos(m_homePos); // 继承出生地
    // return baby;
    (void)partner;
    return nullptr;
}

void TurtleEntity::tick()
{
    AnimalEntity::tick();

    // 更新产卵计时器
    if (m_layingEgg && m_layEggTimer > 0) {
        m_layEggTimer--;
        if (m_layEggTimer <= 0) {
            // 产卵完成
            m_layingEgg = false;
            m_hasEgg = false;

            // 在脚下生成海龟蛋方块
            layEgg();
        }
    }
}

void TurtleEntity::layEgg()
{
    // MC 1.16.5: TurtleEntity.layEgg()
    // 参考: net.minecraft.entity.passive.TurtleEntity.LayEggGoal.tick()

    if (world() == nullptr) {
        return;
    }

    // 获取海龟脚下位置
    BlockPos footPos(static_cast<i32>(std::floor(x())),
        static_cast<i32>(std::floor(y())),
        static_cast<i32>(std::floor(z())));

    // 检查脚下是否是沙子类方块
    const BlockState* belowState = world()->getBlockState(footPos.down());
    if (belowState == nullptr || !BlockTags::SAND().contains(*belowState)) {
        // 不是沙子，无法下蛋
        return;
    }

    // 检查目标位置是否为空气（沙子上方）
    const BlockState* currentPos = world()->getBlockState(footPos);
    if (currentPos == nullptr || !currentPos->isAir()) {
        // 位置被占用
        return;
    }

    // 随机生成 1-4 个蛋
    i32 eggCount = 1 + getRandom().nextInt(4);

    // 获取海龟蛋方块
    Block* turtleEggBlock = VanillaBlocks::TURTLE_EGG;
    if (turtleEggBlock == nullptr) {
        return;
    }

    // 创建海龟蛋方块状态
    // 注意：withEggs 返回值类型，需要保存后再取地址
    auto* turtleEgg = static_cast<blocks::TurtleEggBlock*>(turtleEggBlock);
    BlockState eggState = turtleEgg->withEggs(eggCount);

    // 放置海龟蛋方块
    // flags = 3: 通知客户端 + 通知邻居
    world()->setBlockState(footPos, &eggState, 3);

    // 播放下蛋音效
    // MC 1.16.5: worldIn.playSound((PlayerEntity)null, blockpos, SoundEvents.ENTITY_TURTLE_LAY_EGG,
    //          SoundCategory.BLOCKS, 0.3F, 0.9F + worldIn.rand.nextFloat() * 0.2F);
    f32 pitch = 0.9f + getRandom().nextFloat() * 0.2f;
    world()->playSound(SoundEvents::ENTITY_TURTLE_LAY_EGG,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(footPos.x) + 0.5f,
            static_cast<f32>(footPos.y) + 0.5f,
            static_cast<f32>(footPos.z) + 0.5f),
        0.3f,
        pitch);
}

void TurtleEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 海龟特有目标
    // 优先级 3: 食物诱惑（海草）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isSeagrassPredicate));

    // TODO: 海龟特有目标
    // - TurtleGoHomeGoal: 返回出生地
    // - TurtleLayEggGoal: 产卵
    // - TurtleTravelGoal: 前往海里
}

void TurtleEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 海龟的属性
    // 参考 MC 1.16.5 海龟属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    // 陆地上移动更慢
    // TODO: 在陆地上时减慢速度
}

} // namespace mc
