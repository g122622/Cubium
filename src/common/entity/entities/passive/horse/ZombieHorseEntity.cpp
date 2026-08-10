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

#include "ZombieHorseEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

ZombieHorseEntity::ZombieHorseEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractHorseEntity(id, registry)
{
    // 僵尸马默认已驯服
    setTame(true);
    // 设置跳跃强度
    setJumpStrength(0.96f);

    // 补调 registerGoals / registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。registerGoals 累加语义，基类构造不再调用
    // 以避免重复。详见 AbstractHorseEntity 构造注释。
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> ZombieHorseEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ZombieHorseEntity>(0, registry);
}

bool ZombieHorseEntity::canBeRiddenBy(Player* player) const
{
    // 僵尸马不需要驯服即可骑乘
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }
    return true;
}

void ZombieHorseEntity::tick()
{
    AbstractHorseEntity::tick();

    // 僵尸马在阳光下燃烧（MC 原版中在 BURN_IN_DAYLIGHT 标签中）
    burnUndead();
}

void ZombieHorseEntity::registerGoals()
{
    AbstractHorseEntity::registerGoals();
    // 僵尸马没有额外 AI
}

void ZombieHorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();

    // 僵尸马的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2f);
}

} // namespace mc
