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

#include "MuleEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp"
#include <memory>

namespace mc {

MuleEntity::MuleEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractChestedHorseEntity(id, registry)
{
    setJumpStrength(0.5f);

    // 补调 registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。详见 AbstractHorseEntity 构造注释。
    registerAttributes();
}

std::unique_ptr<Entity> MuleEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<MuleEntity>(0, registry);
}

void MuleEntity::registerGoals()
{
    // 骡完全使用 AbstractHorseEntity 的 AI 目标
    // 注意：骡不育，但 BreedGoal 和 FollowParentGoal 会通过 canBreed() 和 isChild() 检查自动跳过
    AbstractChestedHorseEntity::registerGoals();
}

void MuleEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    const f32 health = getHorseHealth();
    const f32 speed = getSpeed();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, health > 0 ? health : 20.0f);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, speed > 0 ? speed : 0.175f);
}

} // namespace mc
