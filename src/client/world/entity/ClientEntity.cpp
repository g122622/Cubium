#include "ClientEntity.hpp"
#include "../../../common/network/packet/EntityMetadataSerializer.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client {

ClientEntity::ClientEntity(EntityId id, const String& typeId)
    : m_id(id)
    , m_typeId(typeId)
{
}

void ClientEntity::setPosition(f32 x, f32 y, f32 z) {
    // 每60次log一次位置更新
    // static u32 setPositionCounter = 0;
    // setPositionCounter++;
    // if (setPositionCounter % 120 == 0) {
    //     spdlog::info("[ClientEntity] Entity {} setPosition: ({:.2f}, {:.2f}, {:.2f}) -> ({:.2f}, {:.2f}, {:.2f})",
    //                  m_id, m_position.x, m_position.y, m_position.z, x, y, z);
    // }

    m_prevPosition = m_position;
    m_position = Vector3(x, y, z);
    m_targetPosition = m_position;
}

void ClientEntity::setTargetPosition(f32 x, f32 y, f32 z) {
    m_targetPosition = Vector3(x, y, z);
}

void ClientEntity::tickPosition() {
    m_prevPosition = m_position;
    m_position = m_targetPosition;
}

Vector3 ClientEntity::getInterpolatedPosition(f32 partialTick) const {
    return Vector3(
        m_prevPosition.x + (m_position.x - m_prevPosition.x) * partialTick,
        m_prevPosition.y + (m_position.y - m_prevPosition.y) * partialTick,
        m_prevPosition.z + (m_position.z - m_prevPosition.z) * partialTick
    );
}

void ClientEntity::setRotation(f32 yaw, f32 pitch) {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = yaw;
    m_pitch = pitch;
}

void ClientEntity::setHeadRotation(f32 headYaw) {
    m_prevHeadYaw = m_headYaw;
    m_headYaw = headYaw;
}

void ClientEntity::tickRotation() {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_prevHeadYaw = m_headYaw;
}

f32 ClientEntity::getInterpolatedYaw(f32 partialTick) const {
    return m_prevYaw + (m_yaw - m_prevYaw) * partialTick;
}

f32 ClientEntity::getInterpolatedPitch(f32 partialTick) const {
    return m_prevPitch + (m_pitch - m_prevPitch) * partialTick;
}

f32 ClientEntity::getInterpolatedHeadYaw(f32 partialTick) const {
    return m_prevHeadYaw + (m_headYaw - m_prevHeadYaw) * partialTick;
}

void ClientEntity::setVelocity(f32 x, f32 y, f32 z) {
    m_velocity = Vector3(x, y, z);
}

void ClientEntity::setMetadata(const std::vector<u8>& metadata) {
    m_metadata = metadata;
    if (!m_metadata.empty()) {
        (void)network::EntityMetadataSerializer::deserialize(m_metadata, m_dataManager);
        syncMetadataFromDataManager();
    }
}

void ClientEntity::updateAnimation(f32 distanceMoved) {
    // 更新 limbSwingAmount（移动强度）
    m_limbSwingAmount = distanceMoved;

    // 更新 limbSwing（摆动进度）
    // 摆动速度与移动距离成正比
    m_limbSwing += distanceMoved * 0.6f;

    // 保持 limbSwing 在合理范围内
    // 但不需要严格的 2π 限制，因为 sin/cos 可以处理任意值
    if (m_limbSwing > 6.283185307f * 100.0f) {
        m_limbSwing -= 6.283185307f * 100.0f;
    }
}

void ClientEntity::tick() {
    m_ticksExisted++;

    // 更新位置
    tickPosition();

    // 更新旋转
    tickRotation();
}

} // namespace mc::client
