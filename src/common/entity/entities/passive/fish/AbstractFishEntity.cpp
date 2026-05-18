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

#include "AbstractFishEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/FishSwimGoal.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../player/Player.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {

AbstractFishEntity::AbstractFishEntity(EntityId id)
    : WaterMobEntity(id)
{
    // 设置鱼类最大空气供应量（480 ticks = 24秒）
    setAir(maxAir());

    registerGoals();
    registerAttributes();
}

void AbstractFishEntity::tick()
{
    WaterMobEntity::tick();
    updateSwimming();
    updateFlopping();
}

void AbstractFishEntity::registerGoals()
{
    // MC 1.16.5 AbstractFishEntity.registerGoals()
    // 参考: net.minecraft.entity.passive.fish.AbstractFishEntity

    // 优先级 0: 恐慌逃跑（受到伤害或着火时）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.25));

    // 优先级 2: 避开玩家
    // MC 1.16.5: this.goalSelector.addGoal(2, new AvoidEntityGoal<>(this, PlayerEntity.class, 8.0F, 1.6D, 1.4D,
    // EntityPredicates.NOT_SPECTATING::test));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::AvoidEntityGoal>(
        this,
        8.0f,   // 避开距离：8格
        1.6,    // 近距离逃跑速度（更快）
        1.4,    // 远距离逃跑速度
        [](const LivingEntity* entity) -> bool {
            if (entity == nullptr || !entity->isAlive()) {
                return false;
            }
            // 只躲避玩家
            if (entity->typeId() != entity::EntityTypeIdNumber::PLAYER) {
                return false;
            }
            // 不躲避旁观者模式和创造模式玩家
            const Player* player = dynamic_cast<const Player*>(entity);
            if (player != nullptr && (player->isSpectator() || player->isCreative())) {
                return false;
            }
            return true;
        }));

    // 优先级 4: 鱼类游泳目标（继承自 RandomSwimmingGoal，检查 canRandomSwim()）
    // MC 1.16.5 AbstractFishEntity.SwimGoal 继承自 RandomSwimmingGoal(1.0D, 40)
    // shouldExecute() 检查 func_212800_dy() && super.shouldExecute()
    // 对于群游鱼类，只有没有群首时才会自主游泳
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FishSwimGoal>(this, 1.0, 40));
}

void AbstractFishEntity::registerAttributes()
{
    WaterMobEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void AbstractFishEntity::updateSwimming()
{
    if (isInWater()) {
        m_swimming = true;
        m_flopping = false;
        return;
    }

    m_swimming = false;
    m_flopping = true;
}

void AbstractFishEntity::updateFlopping()
{
    if (isInWater()) {
        m_flopTimer = 0;
        m_flopping = false;
        return;
    }

    ++m_flopTimer;

    // MC 1.16.5: 每 100 tick 执行一次扑腾（跳跃和声音）
    // 条件：不在水中 && 在地面 && 垂直碰撞
    // 参考: AbstractFishEntity.livingTick()
    if (m_flopTimer >= 100 && onGround()) {
        // 重置计时器
        m_flopTimer = 0;

        // MC 1.16.5: 计算随机跳跃速度
        // this.setMotion(this.getMotion().add(
        //     (double)((this.rand.nextFloat() * 2.0F - 1.0F) * 0.05F),  // X: -0.05 ~ +0.05
        //     (double)0.4F,                                               // Y: 0.4（向上）
        //     (double)((this.rand.nextFloat() * 2.0F - 1.0F) * 0.05F)    // Z: -0.05 ~ +0.05
        // ));
        math::Random rng = getRandom();
        f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * 0.05f;
        f32 dy = 0.4f;
        f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * 0.05f;

        addVelocity(dx, dy, dz);

        // 设置不在地面
        setOnGround(false);

        // MC 1.16.5: 播放扑腾声音
        // this.playSound(this.getFlopSound(), this.getSoundVolume(), this.getSoundPitch());
        auto flopSound = getFlopSound();
        if (flopSound.has_value()) {
            playSound(*flopSound, getSoundVolume(), getSoundPitch());
        }
    }
}

} // namespace mc
