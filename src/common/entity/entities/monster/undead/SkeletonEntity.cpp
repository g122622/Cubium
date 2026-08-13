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

#include "SkeletonEntity.hpp"
#include "../../../serialization/EntityNbtKeys.hpp"
#include "../../../serialization/NbtHelper.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

SkeletonEntity::SkeletonEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractSkeletonEntity(id, registry)
{
    registerGoals();
    registerAttributes();
    // 在 registerGoals() 之后设置战斗目标
    setCombatTask();

    // 默认主手持弓：骷髅使用弓远程攻击，setCombatTask 判定 shouldUseRanged 依赖主手持弓
    // （不持弓则退化 MeleeAttackGoal 近战）。GameTest 的 test.spawn 不走 finalizeSpawn/
    // populateDefaultEquipmentSlots，故构造期补弓确保 GameTest spawn 的骷髅也能远程攻击。
    // 自然生成路径由 populateDefaultEquipmentSlots 给弓（isEmpty 守卫避免重复）。
    if (getEquipment(EquipmentSlot::MainHand).isEmpty() && Items::BOW != nullptr) {
        setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::BOW, 1));
    }
}

std::unique_ptr<Entity> SkeletonEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SkeletonEntity>(EntityInstanceId(0), registry);
}

void SkeletonEntity::registerGoals()
{
    AbstractSkeletonEntity::registerGoals();
}

void SkeletonEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();
}

// ========== NBT 序列化 ==========

void SkeletonEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    AbstractSkeletonEntity::addAdditionalSaveData(tag);

    // StrayConversionTime — 流浪者转化倒计时
    if (m_strayConversionTime > 0) {
        tag.put(nbt_keys::STRAY_CONVERSION_TIME, m_strayConversionTime);
    }
}

Result<void> SkeletonEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现
    MC_TRY(AbstractSkeletonEntity::readAdditionalSaveData(tag));

    // StrayConversionTime
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::STRAY_CONVERSION_TIME)) {
        m_strayConversionTime = *val;
    }

    return Result<void>::ok();
}

} // namespace mc
