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

#include "GiantEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>
#include <memory>

namespace mc {

GiantEntity::GiantEntity(EntityId id)
    : MonsterEntity(id)
{
    // 巨人体型巨大 - 通过 width()/height() 设置
}

std::unique_ptr<Entity> GiantEntity::create(IWorld* /*world*/)
{
    return std::make_unique<GiantEntity>(EntityId(0));
}

void GiantEntity::tick()
{
    MonsterEntity::tick();

    // 巨人没有特殊的tick逻辑
    // 原版巨人没有AI
}

void GiantEntity::registerGoals()
{
    // 原版巨人没有AI目标
    // 不调用 MonsterEntity::registerGoals()
}

void GiantEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 巨人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 50.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0f);
}

std::optional<ResourceLocation> GiantEntity::getAmbientSound() const
{
    // 巨人无环境音，对齐原版 Giant（不 override → Mob 默认 null）。
    // sounds.json 中无 entity.giant.ambient。
    return std::nullopt;
}

f32 GiantEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // MC Giant.getWalkTargetValue: 返回 brightness - 0.5f（不取反）
    // 巨人是唯一偏好明亮区域的 Monster 子类
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }
    f32 brightness = worldPtr->getBrightness(
        BlockPos(static_cast<i32>(std::floor(x)), static_cast<i32>(std::floor(y)), static_cast<i32>(std::floor(z))));
    return brightness - 0.5f;
}

} // namespace mc
