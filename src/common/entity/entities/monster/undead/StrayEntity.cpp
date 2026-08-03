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

#include "StrayEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include <memory>

namespace mc {

StrayEntity::StrayEntity(EntityInstanceId id)
    : AbstractSkeletonEntity(id)
{
    registerGoals();
    registerAttributes();
    // 在 registerGoals() 之后设置战斗目标
    // 流浪者使用远程攻击（继承父类的 setCombatTask）
    setCombatTask();
}

std::unique_ptr<Entity> StrayEntity::create(IWorld* /*world*/)
{
    return std::make_unique<StrayEntity>(EntityInstanceId(0));
}

void StrayEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();
}

} // namespace mc
