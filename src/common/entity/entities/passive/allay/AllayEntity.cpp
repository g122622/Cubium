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

#include "AllayEntity.hpp"
#include "common/entity/attribute/Attributes.hpp"

namespace mc {
namespace entity {

std::unique_ptr<Entity> AllayEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 临时 ID 0：实际 ID 由 EntityManager 分配。
    return std::make_unique<AllayEntity>(EntityInstanceId(0), registry);
}

AllayEntity::AllayEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 飞行生物不受重力影响（原版 Allay 通过 MoveControl + 飞行移动实现悬浮）。
    setNoGravity(true);
}

std::unique_ptr<AnimalEntity> AllayEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 原版悦灵不可繁殖（Animal 的繁殖路径对悦灵并不适用），因此没有幼体。
    return nullptr;
}

void AllayEntity::registerAttributes()
{
    AnimalEntity::registerAttributes();

    // 对齐原版 `Allay#createAttributes()`：MAX_HEALTH = 20，FLYING_SPEED = 0.1。
    attributes().setBaseValue(attribute::Attributes::MAX_HEALTH, 20.0);
    attributes().setBaseValue(attribute::Attributes::MOVEMENT_SPEED, 0.1);
}

} // namespace entity
} // namespace mc
