#include "PufferfishEntity.hpp"
#include "../../../world/World.hpp"
#include "../../attribute/Attributes.hpp"

namespace mc {

PufferfishEntity::PufferfishEntity(LegacyEntityType type, EntityId id)
    : AbstractFishEntity(type, id)
{
}

std::unique_ptr<Entity> PufferfishEntity::create(IWorld* /*world*/) {
    return std::make_unique<PufferfishEntity>(LegacyEntityType::Unknown, 0);
}

f32 PufferfishEntity::getPuffSize() const {
    switch (m_puffState) {
        case PuffState::Deflated:
            return 0.35f;
        case PuffState::SemiPuffed:
            return 0.5f;
        case PuffState::FullyPuffed:
            return 0.7f;
        default:
            return 0.35f;
    }
}

void PufferfishEntity::tick() {
    AbstractFishEntity::tick();

    // 更新膨胀状态
    if (m_puffState != PuffState::Deflated) {
        m_puffTimer++;
        if (m_puffTimer >= PUFF_DURATION) {
            // 开始收缩
            m_deflateTimer++;
            if (m_deflateTimer >= DEFLATE_DELAY) {
                // 收缩一级
                if (m_puffState == PuffState::FullyPuffed) {
                    m_puffState = PuffState::SemiPuffed;
                } else if (m_puffState == PuffState::SemiPuffed) {
                    m_puffState = PuffState::Deflated;
                }
                m_deflateTimer = 0;
                m_puffTimer = 0;
            }
        }
    }

    // TODO: 检测附近玩家，如果靠近则膨胀
    // if (nearbyPlayer && m_puffState == PuffState::Deflated) {
    //     m_puffState = PuffState::SemiPuffed;
    //     m_puffTimer = 0;
    // }
}

void PufferfishEntity::registerAttributes() {
    // 调用父类方法
    AbstractFishEntity::registerAttributes();

    // 河豚的属性
    // 参考 MC 1.16.5 河豚属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
