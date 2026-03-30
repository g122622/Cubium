#include "AgeableEntity.hpp"

namespace mc {

AgeableEntity::AgeableEntity(LegacyEntityType type, EntityId id)
    : CreatureEntity(type, id)
{
}

void AgeableEntity::setGrowingAge(i32 age) {
    m_growingAge = age;

    // 更新尺寸
    if (isChild()) {
        // 幼体尺寸
        // TODO: 设置实体尺寸
    } else {
        // 成体尺寸
        // TODO: 设置实体尺寸
    }
}

void AgeableEntity::setChild(bool child) {
    if (child) {
        setGrowingAge(BABY_AGE);
    } else {
        setGrowingAge(MAX_AGE);
    }
}

void AgeableEntity::ageUp(i32 seconds) {
    i32 ticks = seconds * 20; // 秒转换为tick
    m_growingAge += ticks;

    if (m_growingAge >= MAX_AGE) {
        m_growingAge = MAX_AGE;
        onGrowUp();
    }
}

void AgeableEntity::addGrowingAge(i32 amount) {
    m_growingAge += amount;

    if (m_growingAge >= MAX_AGE && amount > 0) {
        m_growingAge = MAX_AGE;
        onGrowUp();
    }
}

bool AgeableEntity::canBreed() const {
    // 必须是成体且不在爱心状态
    return !isChild() && m_loveTimer <= 0;
}

void AgeableEntity::setInLove(u64 /*playerInLove*/) {
    if (canBreed()) {
        m_loveTimer = LOVE_TIMER_MAX;
    }
}

void AgeableEntity::tick() {
    CreatureEntity::tick();

    updateAge();
    updateLove();
}

void AgeableEntity::updateAge() {
    if (isChild()) {
        // 幼体成长
        i32 growth = static_cast<i32>(m_growthSpeed);

        // 处理强制成长
        if (m_forcedAgeTimer > 0) {
            --m_forcedAgeTimer;
            growth += m_forcedAge / LOVE_TIMER_MAX;
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
}

void AgeableEntity::updateLove() {
    if (m_loveTimer > 0) {
        --m_loveTimer;

        if (m_loveTimer == 0) {
            // 爱心状态结束
        }
    }
}

f32 AgeableEntity::getChildScale() const {
    if (isChild()) {
        return BABY_SCALE;
    }
    return 1.0f;
}

} // namespace mc
