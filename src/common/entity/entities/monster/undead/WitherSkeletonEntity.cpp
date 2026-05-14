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

#include "WitherSkeletonEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

WitherSkeletonEntity::WitherSkeletonEntity(LegacyEntityType type, EntityId id)
    : AbstractSkeletonEntity(type, id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> WitherSkeletonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherSkeletonEntity>(LegacyEntityType::Unknown, 0);
}

void WitherSkeletonEntity::registerGoals()
{
    AbstractSkeletonEntity::registerGoals();

    // TODO: 切到 1.16.5 近战目标集，替换弓箭手行为。
}

void WitherSkeletonEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
}

} // namespace mc
