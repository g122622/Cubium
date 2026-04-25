#include "ClientEntity.hpp"
#include "common/network/packet/EntityMetadataSerializer.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client {

ClientEntity::ClientEntity(EntityId id, const String& typeId)
    : m_id(id)
    , m_typeId(typeId)
{
}

void ClientEntity::setInterpolationSpeed(f32 speed) {
    m_interpolationSpeed = std::clamp(speed, 0.01f, 1.0f);
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
    // 保存当前位置作为上一帧位置（用于渲染插值）
    m_prevPosition = m_position;
    // 不在这里做平滑插值，平滑插值在 updateInterpolation 中每帧执行
}

void ClientEntity::updateInterpolation(f32 deltaTime) {
    if (m_smoothInterpolation) {
        // 平滑插值位置
        // 使用 deltaTime 归一化到 20 TPS 的等效插值速度
        // 例如：deltaTime=0.016s (60 FPS), interpolationSpeed=0.3
        // 实际插值 = 1 - pow(1 - 0.3, deltaTime * 20) ≈ 0.093
        // 这样无论帧率如何，插值速度都保持一致的感觉
        const f32 tickRate = 20.0f;
        const f32 alpha = 1.0f - std::pow(1.0f - m_interpolationSpeed, deltaTime * tickRate);

        Vector3 diff = m_targetPosition - m_position;
        m_position = m_position + diff * alpha;

        // 平滑插值旋转，需要处理角度环绕
        // Yaw
        f32 yawDiff = m_targetYaw - m_yaw;
        while (yawDiff > 180.0f) yawDiff -= 360.0f;
        while (yawDiff < -180.0f) yawDiff += 360.0f;
        m_yaw += yawDiff * alpha;
        while (m_yaw > 180.0f) m_yaw -= 360.0f;
        while (m_yaw < -180.0f) m_yaw += 360.0f;

        // Pitch (范围 -90 到 90)
        f32 pitchDiff = m_targetPitch - m_pitch;
        m_pitch += pitchDiff * alpha;
        m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

        // HeadYaw
        f32 headYawDiff = m_targetHeadYaw - m_headYaw;
        while (headYawDiff > 180.0f) headYawDiff -= 360.0f;
        while (headYawDiff < -180.0f) headYawDiff += 360.0f;
        m_headYaw += headYawDiff * alpha;
        while (m_headYaw > 180.0f) m_headYaw -= 360.0f;
        while (m_headYaw < -180.0f) m_headYaw += 360.0f;
    } else {
        // 禁用平滑插值时，直接跳到目标位置
        m_position = m_targetPosition;
        m_yaw = m_targetYaw;
        m_pitch = m_targetPitch;
        m_headYaw = m_targetHeadYaw;
    }
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
    m_targetYaw = yaw;
    m_targetPitch = pitch;
}

void ClientEntity::setTargetRotation(f32 yaw, f32 pitch) {
    m_targetYaw = yaw;
    m_targetPitch = pitch;
}

void ClientEntity::setHeadRotation(f32 headYaw) {
    m_prevHeadYaw = m_headYaw;
    m_headYaw = headYaw;
    m_targetHeadYaw = headYaw;
}

void ClientEntity::setTargetHeadRotation(f32 headYaw) {
    m_targetHeadYaw = headYaw;
}

void ClientEntity::tickRotation() {
    // 保存当前旋转作为上一帧旋转（用于渲染插值）
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_prevHeadYaw = m_headYaw;
    // 不在这里做平滑插值，平滑插值在 updateInterpolation 中每帧执行
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
    // 保存上一帧状态（用于渲染插值）
    m_prevLimbSwing = m_limbSwing;
    m_prevLimbSwingAmount = m_limbSwingAmount;

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

    // 更新位置和旋转的上一帧状态（用于渲染插值）
    tickPosition();
    tickRotation();
}

} // namespace mc::client
