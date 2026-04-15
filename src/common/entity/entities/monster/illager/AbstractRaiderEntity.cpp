#include "AbstractRaiderEntity.hpp"
#include "../../../../world/village/raid/Raid.hpp"
#include "../../../../world/IWorld.hpp"

namespace mc {

AbstractRaiderEntity::AbstractRaiderEntity(LegacyEntityType type, EntityId id)
    : PatrollerEntity(type, id)
{
}

void AbstractRaiderEntity::joinRaid(world::village::raid::Raid* raid, i32 wave) {
    m_raid = raid;
    m_wave = wave;
    m_canJoinRaid = false;
}

void AbstractRaiderEntity::leaveRaid() {
    m_raid = nullptr;
    m_wave = 0;
    m_canJoinRaid = true;
}

void AbstractRaiderEntity::startCelebrating() {
    m_celebrationTime = CELEBRATION_DURATION;
    setState(IllagerState::Celebrating);
}

void AbstractRaiderEntity::tick() {
    PatrollerEntity::tick();

    // 更新庆祝时间
    if (m_celebrationTime > 0) {
        m_celebrationTime--;
        if (m_celebrationTime <= 0) {
            setState(IllagerState::Neutral);
        }
    }

    // 检查袭击是否仍然有效
    if (m_raid != nullptr) {
        // 如果袭击已结束，离开袭击
        if (m_raid->status() != world::village::raid::RaidStatus::Ongoing) {
            leaveRaid();
        }
    }
}

void AbstractRaiderEntity::die(DamageSource& cause) {
    // 通知袭击掠夺者死亡
    if (m_raid != nullptr && m_world != nullptr) {
        m_raid->onRaiderDeath(id(), *m_world);
        leaveRaid();
    }

    // 调用父类die方法
    PatrollerEntity::die(cause);
}

} // namespace mc
