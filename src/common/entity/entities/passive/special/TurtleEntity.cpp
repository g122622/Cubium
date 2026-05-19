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
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/Vector3.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/BlockTags.hpp"
#include "../../../../world/block/VanillaBlocks.hpp"
#include "../../../../world/block/blocks/mob/TurtleEggBlock.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/special/TurtleGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"
#include <cmath>

namespace mc {

TurtleEntity::TurtleEntity(EntityId id)
    : AnimalEntity(id)
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
    return std::make_unique<TurtleEntity>(0);
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
    // MC 1.16.5: 海龟仅接受海草作为繁殖物品
    // 参考: net.minecraft.entity.passive.TurtleEntity.isBreedingItem()
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::SEAGRASS;
}

bool TurtleEntity::canBreed() const
{
    // MC 1.16.5: 海龟只有在没有蛋的情况下才能繁殖
    // 参考: net.minecraft.entity.passive.TurtleEntity.canBreed()
    // return super.canBreed() && !this.hasEgg();
    return AnimalEntity::canBreed() && !m_hasEgg;
}

std::unique_ptr<AnimalEntity> TurtleEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // MC 1.16.5: TurtleEntity.createChild()
    // 创建小海龟，继承出生地记忆
    auto baby = std::make_unique<TurtleEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 关键：小海龟继承父母的出生地
    // 这样小海龟长大后也会回到这里产卵
    // 参考 MC 1.16.5: TurtleEggBlock 孵化时调用 onInitialSpawn 设置出生地为蛋的位置
    if (hasHomePos()) {
        baby->setHomePos(m_homePos);
    }

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
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
    BlockPos footPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

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
        Vector3(
            static_cast<f32>(footPos.x) + 0.5f, static_cast<f32>(footPos.y) + 0.5f, static_cast<f32>(footPos.z) + 0.5f),
        0.3f,
        pitch);
}

void TurtleEntity::registerGoals()
{
    // MC 1.16.5: TurtleEntity.registerGoals()
    // 参考: net.minecraft.entity.passive.TurtleEntity.registerGoals()

    // 优先级 0: 恐慌逃跑（最高优先级）
    // 海龟恐慌时优先寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::TurtlePanicGoal>(this, 1.2));

    // 优先级 1: 繁殖和产卵
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::TurtleMateGoal>(this, 1.0));
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::TurtleLayEggGoal>(this, 1.0));

    // 优先级 2: 海草诱惑
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::TurtleTemptGoal>(this, 1.1));

    // 优先级 3: 前往水中
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::TurtleGoToWaterGoal>(this, 1.0));

    // 优先级 4: 返回出生地
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::TurtleGoHomeGoal>(this, 1.0));

    // 优先级 5: 跟随父母（幼年海龟）
    // 由 AnimalEntity::registerGoals() 注册

    // 优先级 7: 旅行（在水中随机游泳）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::TurtleTravelGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    // MC 1.16.5: LookAtGoal(this, PlayerEntity.class, 8.0F)
    m_goalSelector.addGoal(
        8, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));

    // 优先级 9: 随机游荡（只在陆地上）
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::TurtleWanderGoal>(this, 1.0, 100));

    // 调用父类方法注册基础动物 AI（包括 SwimGoal、FollowParentGoal 等）
    AnimalEntity::registerGoals();
}

void TurtleEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 海龟的属性
    // 参考 MC 1.16.5 海龟属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    // MC 1.16.5: 海龟在陆地上移动较慢，通过 travel() 方法实现
    // 陆地速度 = max(AIMoveSpeed / 2.0, 0.06F)，约为水中速度的 24%
}

void TurtleEntity::travel(const Vector3& travelVec)
{
    // MC 1.16.5: TurtleEntity.travel() 参考
    // 原版在 MoveHelperController.updateSpeed() 中处理速度调整
    // 我们在 travel() 中根据环境调整速度

    // 获取基础移动速度
    f32 baseSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25));

    if (isInWater()) {
        // MC 1.16.5: 水中移动
        // 参考 TurtleEntity.travel() 和 MoveHelperController.updateSpeed()

        // 计算实际速度
        f32 swimSpeed = baseSpeed;

        // 检查是否远离出生地超过 16 格
        if (m_hasHomePos) {
            Vector3 homePosF(static_cast<f32>(m_homePos.x) + 0.5f,
                static_cast<f32>(m_homePos.y),
                static_cast<f32>(m_homePos.z) + 0.5f);
            f32 distanceSq = position().distanceSquared(homePosF);
            if (distanceSq > 256.0f) { // 16 * 16 = 256
                // 远离出生地时速度减半，最低 0.08F
                swimSpeed = std::max(swimSpeed * 0.5f, 0.08f);
            }
        }

        // 幼体在水中速度更低
        if (isChild()) {
            // MC 1.16.5: 幼体速度 = max(speed / 3.0, 0.06F)
            swimSpeed = std::max(swimSpeed / 3.0f, 0.06f);
        }

        setAIMoveSpeed(swimSpeed);

        // MC 1.16.5: 水中给予轻微上升动力
        // 参考 MoveHelperController.updateSpeed():
        // this.turtle.setMotion(this.turtle.getMotion().add(0.0D, 0.005D, 0.0D));
        Vector3 vel = velocity();
        vel.y += 0.005;
        setVelocity(vel);
    } else if (onGround()) {
        // MC 1.16.5: 陆地移动
        // 参考 MoveHelperController.updateSpeed():
        // this.turtle.setAIMoveSpeed(Math.max(f / 2.0F, 0.06F));
        f32 landSpeed = std::max(baseSpeed * 0.5f, 0.06f);
        setAIMoveSpeed(landSpeed);
    } else {
        // 空中（跳跃或下落）：保持当前 AI 速度
        // 不做额外调整
    }

    // 调用父类处理实际移动
    AnimalEntity::travel(travelVec);
}

} // namespace mc
