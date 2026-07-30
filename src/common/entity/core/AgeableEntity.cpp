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

#include "AgeableEntity.hpp"
#include "../serialization/EntityNbtKeys.hpp"
#include "../serialization/NbtHelper.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = CreatureEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达 CreatureEntity/MobEntity，子类首字段续接 MobEntity id15。
const entity::EntityClassInfo& AgeableEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AgeableEntity", &CreatureEntity::classInfo()};
    return s_classInfo;
}

AgeableEntity::AgeableEntity(EntityInstanceId id) noexcept
    : CreatureEntity(id)
{}

void AgeableEntity::setGrowingAge(i32 age)
{
    const bool wasChild = isChild();
    m_growingAge = age;
    if (wasChild != isChild()) {
        refreshDimensions();
    }
}

void AgeableEntity::setChild(bool child)
{
    setGrowingAge(child ? BABY_AGE : MAX_AGE);
}

void AgeableEntity::ageUp(i32 seconds)
{
    i32 ticks = seconds * 20; // 秒转换为tick
    const bool wasChild = isChild();
    m_growingAge += ticks;

    if (m_growingAge >= MAX_AGE) {
        m_growingAge = MAX_AGE;
        onGrowUp();
    }

    if (wasChild != isChild()) {
        refreshDimensions();
    }
}

void AgeableEntity::addGrowingAge(i32 amount)
{
    const bool wasChild = isChild();
    m_growingAge += amount;

    if (m_growingAge >= MAX_AGE && amount > 0) {
        m_growingAge = MAX_AGE;
        onGrowUp();
    }

    if (wasChild != isChild()) {
        refreshDimensions();
    }
}

bool AgeableEntity::canBreed() const
{
    // 必须是成体且不在爱心状态
    return !isChild() && m_loveTimer <= 0;
}

void AgeableEntity::setInLove(u64 /*playerInLove*/)
{
    if (canBreed()) {
        m_loveTimer = LOVE_TIMER_MAX;
    }
}

void AgeableEntity::tick()
{
    CreatureEntity::tick();

    updateAge();
    updateLove();
}

void AgeableEntity::updateAge()
{
    const bool wasChild = isChild();

    if (isChild()) {
        // 幼体成长
        i32 growth = static_cast<i32>(m_growthSpeed);

        // 处理强制成长
        if (m_forcedAgeTimer > 0) {
            --m_forcedAgeTimer;
            growth += m_forcedAge / LOVE_TIMER_MAX;

            // MC原版 AgeableMob.aiStep: 幼体被喂食加速成长时，每4tick显示开心村民粒子
            if (m_forcedAgeTimer % 4 == 0 && m_world != nullptr) {
                m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::VillagerHappy));
            }
        }

        m_growingAge += growth;

        if (m_growingAge >= MAX_AGE) {
            m_growingAge = MAX_AGE;
            onGrowUp();
        }
    } else {
        // 成体繁殖冷却
        if (m_growingAge > 0) {
            --m_growingAge;
        }
    }

    if (wasChild != isChild()) {
        refreshDimensions();
    }
}

void AgeableEntity::updateLove()
{
    if (m_loveTimer > 0) {
        --m_loveTimer;

        if (m_loveTimer == 0) {
            // 爱心状态结束
        }
    }
}

f32 AgeableEntity::getChildScale() const
{
    if (isChild()) {
        return BABY_SCALE;
    }
    return 1.0f;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void AgeableEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    MobEntity::addAdditionalSaveData(tag);

    // Age (i32) - 年龄（负数=幼体，0或正数=成体）
    tag.put(nbt_keys::AGE, m_growingAge);

    // ForcedAge (i32) - 强制成长值
    tag.put(nbt_keys::FORCED_AGE, m_forcedAge);

    // InLove (i32) - 爱心计时器
    tag.put(nbt_keys::IN_LOVE, m_loveTimer);
}

Result<void> AgeableEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现
    MC_TRY(MobEntity::readAdditionalSaveData(tag));

    // Age (i32) - 年龄
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::AGE)) {
        m_growingAge = *val;
        // 年龄变化可能影响尺寸
        refreshDimensions();
    }

    // ForcedAge (i32) - 强制成长值
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::FORCED_AGE)) {
        m_forcedAge = *val;
    }

    // InLove (i32) - 爱心计时器
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::IN_LOVE)) {
        m_loveTimer = *val;
    }

    return Result<void>::ok();
}

} // namespace mc
