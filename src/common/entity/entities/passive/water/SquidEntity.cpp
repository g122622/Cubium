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

#include "SquidEntity.hpp"

#include "common/entity/ai/goal/goals/special/SquidGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

SquidEntity::SquidEntity(EntityInstanceId id)
    : WaterMobEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SquidEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SquidEntity>(0);
}

void SquidEntity::sprayInk()
{
    if (m_sprayingInk) {
        return;
    }
    m_sprayingInk = true;
    m_sprayTimer = SPRAY_INK_DURATION;

    // 播放喷墨音效
    auto squirtSound = getSquirtSound();
    if (squirtSound.has_value()) {
        playSound(*squirtSound, 1.0f, 1.0f);
    }

    // 在鱿鱼位置生成墨汁粒子
    if (world() != nullptr && world()->isClientSide()) {
        using namespace mc::particle;
        math::Random& random = world()->getRandom();

        // 生成多个墨汁粒子形成云状效果
        for (i32 i = 0; i < 30; ++i) {
            // 粒子位置：在鱿鱼周围随机分布
            f32 px = static_cast<f32>(x()) + (random.nextFloat() - 0.5f) * width() * 2.0f;
            f32 py = static_cast<f32>(y()) + random.nextFloat() * height();
            f32 pz = static_cast<f32>(z()) + (random.nextFloat() - 0.5f) * width() * 2.0f;

            // 粒子速度：向外扩散
            f32 vx = (random.nextFloat() - 0.5f) * 0.5f;
            f32 vy = random.nextFloat() * 0.1f;
            f32 vz = (random.nextFloat() - 0.5f) * 0.5f;

            world()->addParticle(getInkParticle(), Vector3(px, py, pz), Vector3(vx, vy, vz));
        }
    }
}

bool SquidEntity::hurt(DamageSource& source, f32 amount)
{
    // 调用父类 hurt 处理实际伤害；仅当成功受伤且有复仇目标时才喷墨逃跑
    if (WaterMobEntity::hurt(source, amount) && getLastHurtBy() != nullptr) {
        sprayInk();
        return true;
    }
    return false;
}

void SquidEntity::tick()
{
    WaterMobEntity::tick();

    // 更新喷墨计时器
    if (m_sprayingInk && m_sprayTimer > 0) {
        m_sprayTimer--;
        if (m_sprayTimer <= 0) {
            m_sprayingInk = false;
        }
    }

    // 更新游泳行为
    if (isInWater()) {
        m_swimTimer++;
        m_changeDirectionTimer++;

        // 随机改变方向
        if (m_changeDirectionTimer >= 100) {
            math::Random& rng = getRandom();
            m_targetSwimAngle = rng.nextFloat(0.0f, 360.0f);
            m_changeDirectionTimer = 0;
        }

        // 平滑转向
        f32 angleDiff = m_targetSwimAngle - m_swimAngle;
        while (angleDiff > 180.0f)
            angleDiff -= 360.0f;
        while (angleDiff < -180.0f)
            angleDiff += 360.0f;
        m_swimAngle += angleDiff * 0.1f;

        // 游泳推进
        if (m_swimTimer >= SWIM_DURATION) {
            m_swimming = false;
            m_swimTimer = 0;
        }
    } else {
        // 在陆地上扑腾
        m_swimming = false;
    }
}

void SquidEntity::setMovementVector(f32 x, f32 y, f32 z)
{
    m_randomMotionVecX = x;
    m_randomMotionVecY = y;
    m_randomMotionVecZ = z;
}

bool SquidEntity::hasMovementVector() const
{
    return m_randomMotionVecX != 0.0f || m_randomMotionVecY != 0.0f || m_randomMotionVecZ != 0.0f;
}

void SquidEntity::registerGoals()
{
    // 优先级 0: 随机游泳（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SquidMoveRandomGoal>(this));

    // 优先级 1: 逃跑目标（受攻击时逃跑）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SquidFleeGoal>(this));
}

void SquidEntity::registerAttributes()
{
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // 鱿鱼的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
