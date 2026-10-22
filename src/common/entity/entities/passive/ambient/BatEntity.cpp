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

#include "common/entity/entities/passive/ambient/BatEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/BatGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/passive/ambient/AmbientEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>
#include <memory>

namespace mc {

BatEntity::BatEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AmbientEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 默认飞行
    m_flying = true;
}

std::unique_ptr<Entity> BatEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<BatEntity>(0, registry);
}

bool BatEntity::canRest() const
{
    // 检查上方是否有固体方块可以倒挂
    if (m_world == nullptr) {
        return false;
    }
    BlockPos above(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y + 1.0)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    const BlockState* state = m_world->getBlockState(above);
    if (state == nullptr) {
        return false;
    }
    return state->getBlock().isSolid(*state);
}

void BatEntity::tick()
{
    AmbientEntity::tick();

    // 飞行行为由 BatFloatGoal（随机目标点悬停）与 BatRestGoal（倒挂）驱动。
    //
    // 对齐 vanilla Bat.tick：
    //   休息中 → 速度清零并贴合到倒挂位置（头顶方块的底面）；
    //   飞行中 → Y 速度每 tick 保留 60%（垂直阻尼）。
    if (m_resting) {
        setVelocity(Vector3(0.0, 0.0, 0.0));
        const f32 hangY = static_cast<f32>(std::floor(y())) + 1.0f - height();
        setPosition(x(), hangY, z());
    } else {
        math::Vector3 vel = velocity();
        setVelocity(vel.x, vel.y * 0.6, vel.z);
    }
}

bool BatEntity::isFlapping() const
{
    // 对齐 vanilla Bat.isFlapping：!isResting() && tickCount % 10 == 0
    return !m_resting && ticksExisted() % 10 == 0;
}

std::optional<ResourceLocation> BatEntity::getAmbientSound() const
{
    // 对齐 vanilla Bat.getAmbientSound：休息时 3/4 概率不发声
    if (m_resting && getRandom().nextInt(4) != 0) {
        return std::nullopt;
    }
    return SoundEvents::ENTITY_BAT_AMBIENT;
}

bool BatEntity::hurt(DamageSource& source, f32 amount)
{
    // 对齐 vanilla Bat.hurtServer：受击先中断休息状态，再走基类受伤链路。
    if (isResting()) {
        setResting(false);
        setFlying(true);
    }
    return AmbientEntity::hurt(source, amount);
}

void BatEntity::registerGoals()
{
    // 蝙蝠使用独立的Goal类实现行为，遵循项目架构风格

    // 优先级 0: 随机飞行目标
    // 飞行时选择随机目标点，平滑转向飞行
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::BatFloatGoal>(this));

    // 优先级 1: 挂墙休息目标
    // 白天尝试挂墙休息，玩家靠近或失去支撑时飞走
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::BatRestGoal>(this));
}

void BatEntity::registerAttributes()
{
    AmbientEntity::registerAttributes();

    // 蝙蝠属性：低生命值，固定飞行速度
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, FLY_SPEED);
    attributes().setBaseValue(entity::attribute::Attributes::FLYING_SPEED, FLY_SPEED);
}

} // namespace mc
