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

#include "EntityPackets.hpp"
#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

// ==================== SpawnEntityPacket ====================

Result<std::vector<u8>> SpawnEntityPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeBytes(m_uuid.data(), 16);
    serializer.writeString(m_entityTypeId);
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeI16(m_velocityX);
    serializer.writeI16(m_velocityY);
    serializer.writeI16(m_velocityZ);

    // 序列化 ItemStack 数据（如果存在）
    serializer.writeBool(m_hasItemStack);
    if (m_hasItemStack) {
        m_itemStack.serialize(serializer);
    }

    return serializer.buffer();
}

Result<void> SpawnEntityPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto result = deserializer.readU32();
    if (!result.success()) {
        return Error(result.error());
    }
    m_entityId = result.value();

    auto uuidResult = deserializer.readBytes(16);
    if (!uuidResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read UUID");
    }
    const auto& uuidBytes = uuidResult.value();
    std::copy(uuidBytes.begin(), uuidBytes.end(), m_uuid.begin());

    auto typeIdResult = deserializer.readString();
    if (!typeIdResult.success()) {
        return Error(typeIdResult.error());
    }
    m_entityTypeId = typeIdResult.value();

    auto xResult = deserializer.readF32();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto vxResult = deserializer.readI16();
    if (!vxResult.success()) return Error(vxResult.error());
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readI16();
    if (!vyResult.success()) return Error(vyResult.error());
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readI16();
    if (!vzResult.success()) return Error(vzResult.error());
    m_velocityZ = vzResult.value();

    // 读取 ItemStack 数据（如果存在）
    auto hasItemResult = deserializer.readBool();
    if (!hasItemResult.success()) return Error(hasItemResult.error());
    m_hasItemStack = hasItemResult.value();

    if (m_hasItemStack) {
        auto itemResult = ItemStack::deserialize(deserializer);
        if (!itemResult.success()) {
            return Error(itemResult.error());
        }
        m_itemStack = itemResult.value();
    }

    return {};
}

// ==================== SpawnMobPacket ====================

Result<std::vector<u8>> SpawnMobPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeBytes(m_uuid.data(), 16);
    serializer.writeString(m_entityTypeId);
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeF32(m_headYaw);
    serializer.writeI16(m_velocityX);
    serializer.writeI16(m_velocityY);
    serializer.writeI16(m_velocityZ);
    serializer.writeU32(static_cast<u32>(m_metadata.size()));
    serializer.writeBytes(m_metadata.data(), m_metadata.size());
    return serializer.buffer();
}

Result<void> SpawnMobPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto result = deserializer.readU32();
    if (!result.success()) {
        return Error(result.error());
    }
    m_entityId = result.value();

    auto uuidResult = deserializer.readBytes(16);
    if (!uuidResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read UUID");
    }
    const auto& uuidBytes = uuidResult.value();
    std::copy(uuidBytes.begin(), uuidBytes.end(), m_uuid.begin());

    auto typeIdResult = deserializer.readString();
    if (!typeIdResult.success()) {
        return Error(typeIdResult.error());
    }
    m_entityTypeId = typeIdResult.value();

    auto xResult = deserializer.readF32();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto headYawResult = deserializer.readF32();
    if (!headYawResult.success()) return Error(headYawResult.error());
    m_headYaw = headYawResult.value();

    auto vxResult = deserializer.readI16();
    if (!vxResult.success()) return Error(vxResult.error());
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readI16();
    if (!vyResult.success()) return Error(vyResult.error());
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readI16();
    if (!vzResult.success()) return Error(vzResult.error());
    m_velocityZ = vzResult.value();

    auto metaLenResult = deserializer.readU32();
    if (!metaLenResult.success()) return Error(metaLenResult.error());
    u32 metaLen = metaLenResult.value();

    auto metaDataResult = deserializer.readBytes(metaLen);
    if (!metaDataResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read metadata");
    }
    m_metadata = std::move(metaDataResult.value());

    return {};
}

// ==================== EntityMetadataPacket ====================

Result<std::vector<u8>> EntityMetadataPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeU32(static_cast<u32>(m_metadata.size()));
    serializer.writeBytes(m_metadata.data(), m_metadata.size());
    return serializer.buffer();
}

Result<void> EntityMetadataPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto lenResult = deserializer.readU32();
    if (!lenResult.success()) return Error(lenResult.error());
    u32 len = lenResult.value();

    auto metaDataResult = deserializer.readBytes(len);
    if (!metaDataResult.success()) {
        return Error(ErrorCode::InvalidPacket, "Failed to read metadata");
    }
    m_metadata = std::move(metaDataResult.value());
    return {};
}

// ==================== EntityVelocityPacket ====================

Result<std::vector<u8>> EntityVelocityPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeI16(m_velocityX);
    serializer.writeI16(m_velocityY);
    serializer.writeI16(m_velocityZ);
    return serializer.buffer();
}

Result<void> EntityVelocityPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto vxResult = deserializer.readI16();
    if (!vxResult.success()) return Error(vxResult.error());
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readI16();
    if (!vyResult.success()) return Error(vyResult.error());
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readI16();
    if (!vzResult.success()) return Error(vzResult.error());
    m_velocityZ = vzResult.value();

    return {};
}

// ==================== EntityTeleportPacket ====================

Result<std::vector<u8>> EntityTeleportPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeBool(m_onGround);
    return serializer.buffer();
}

Result<void> EntityTeleportPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto xResult = deserializer.readF32();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto groundResult = deserializer.readBool();
    if (!groundResult.success()) return Error(groundResult.error());
    m_onGround = groundResult.value();

    return {};
}

// ==================== EntityDestroyPacket ====================

Result<std::vector<u8>> EntityDestroyPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(static_cast<u32>(m_entityIds.size()));
    for (u32 id : m_entityIds) {
        serializer.writeU32(id);
    }
    return serializer.buffer();
}

Result<void> EntityDestroyPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto countResult = deserializer.readU32();
    if (!countResult.success()) return Error(countResult.error());
    u32 count = countResult.value();

    m_entityIds.clear();
    m_entityIds.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto idResult = deserializer.readU32();
        if (!idResult.success()) return Error(idResult.error());
        m_entityIds.push_back(idResult.value());
    }
    return {};
}

// ==================== EntityAnimationPacket ====================

Result<std::vector<u8>> EntityAnimationPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeU8(static_cast<u8>(m_animation));
    // 仅 TakeDamage 动画携带 hurtDir（ClientboundHurtAnimationPacket 的 yaw 字段），
    // 其它动画保持原两字段布局不变。
    if (m_animation == Animation::TakeDamage) {
        serializer.writeF32(m_hurtDir);
    }
    return serializer.buffer();
}

Result<void> EntityAnimationPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto animResult = deserializer.readU8();
    if (!animResult.success()) return Error(animResult.error());
    m_animation = static_cast<Animation>(animResult.value());

    if (m_animation == Animation::TakeDamage) {
        auto hurtDirResult = deserializer.readF32();
        if (!hurtDirResult.success()) return Error(hurtDirResult.error());
        m_hurtDir = hurtDirResult.value();
    }

    return {};
}

// ==================== EntityMovePacket ====================

Result<std::vector<u8>> EntityMovePacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeI16(m_deltaX);
    serializer.writeI16(m_deltaY);
    serializer.writeI16(m_deltaZ);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    serializer.writeBool(m_onGround);
    return serializer.buffer();
}

Result<void> EntityMovePacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto dxResult = deserializer.readI16();
    if (!dxResult.success()) return Error(dxResult.error());
    m_deltaX = dxResult.value();

    auto dyResult = deserializer.readI16();
    if (!dyResult.success()) return Error(dyResult.error());
    m_deltaY = dyResult.value();

    auto dzResult = deserializer.readI16();
    if (!dzResult.success()) return Error(dzResult.error());
    m_deltaZ = dzResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    auto groundResult = deserializer.readBool();
    if (!groundResult.success()) return Error(groundResult.error());
    m_onGround = groundResult.value();

    return {};
}

// ==================== EntityHeadLookPacket ====================

Result<std::vector<u8>> EntityHeadLookPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeF32(m_headYaw);
    return serializer.buffer();
}

Result<void> EntityHeadLookPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_headYaw = yawResult.value();

    return {};
}

// ==================== EntityStatusPacket ====================

Result<std::vector<u8>> EntityStatusPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_entityId);
    serializer.writeU8(static_cast<u8>(m_status));
    return serializer.buffer();
}

Result<void> EntityStatusPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);
    auto idResult = deserializer.readU32();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = idResult.value();

    auto statusResult = deserializer.readU8();
    if (!statusResult.success()) return Error(statusResult.error());
    m_status = static_cast<Status>(statusResult.value());

    return {};
}

// ==================== CollectItemPacket ====================

Result<std::vector<u8>> CollectItemPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(m_collectedEntityId);
    serializer.writeU32(m_collectorEntityId);
    serializer.writeI32(m_pickupItemCount);
    return serializer.buffer();
}

Result<void> CollectItemPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto collectedResult = deserializer.readU32();
    if (!collectedResult.success()) return Error(collectedResult.error());
    m_collectedEntityId = collectedResult.value();

    auto collectorResult = deserializer.readU32();
    if (!collectorResult.success()) return Error(collectorResult.error());
    m_collectorEntityId = collectorResult.value();

    auto countResult = deserializer.readI32();
    if (!countResult.success()) return Error(countResult.error());
    m_pickupItemCount = countResult.value();

    return {};
}

// ==================== PlayerInputPacket ====================

Result<std::vector<u8>> PlayerInputPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeF32(m_strafeSpeed);
    serializer.writeF32(m_forwardSpeed);

    // 将跳跃和潜行打包到一个字节中
    u8 flags = 0;
    if (m_jumping) flags |= 0x01;
    if (m_sneaking) flags |= 0x02;
    serializer.writeU8(flags);

    return serializer.buffer();
}

Result<void> PlayerInputPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto strafeResult = deserializer.readF32();
    if (!strafeResult.success()) return Error(strafeResult.error());
    m_strafeSpeed = strafeResult.value();

    auto forwardResult = deserializer.readF32();
    if (!forwardResult.success()) return Error(forwardResult.error());
    m_forwardSpeed = forwardResult.value();

    auto flagsResult = deserializer.readU8();
    if (!flagsResult.success()) return Error(flagsResult.error());
    u8 flags = flagsResult.value();

    m_jumping = (flags & 0x01) != 0;
    m_sneaking = (flags & 0x02) != 0;

    return {};
}

// ==================== SteerBoatPacket ====================

Result<std::vector<u8>> SteerBoatPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeBool(m_leftPaddle);
    serializer.writeBool(m_rightPaddle);
    return serializer.buffer();
}

Result<void> SteerBoatPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto leftResult = deserializer.readBool();
    if (!leftResult.success()) return Error(leftResult.error());
    m_leftPaddle = leftResult.value();

    auto rightResult = deserializer.readBool();
    if (!rightResult.success()) return Error(rightResult.error());
    m_rightPaddle = rightResult.value();

    return {};
}

// ==================== MoveVehiclePacket ====================

Result<std::vector<u8>> MoveVehiclePacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeF64(m_x);
    serializer.writeF64(m_y);
    serializer.writeF64(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    return serializer.buffer();
}

Result<void> MoveVehiclePacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto xResult = deserializer.readF64();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF64();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF64();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    return {};
}

// ==================== VehicleMovePacket ====================

Result<std::vector<u8>> VehicleMovePacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeF64(m_x);
    serializer.writeF64(m_y);
    serializer.writeF64(m_z);
    serializer.writeF32(m_yaw);
    serializer.writeF32(m_pitch);
    return serializer.buffer();
}

Result<void> VehicleMovePacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto xResult = deserializer.readF64();
    if (!xResult.success()) return Error(xResult.error());
    m_x = xResult.value();

    auto yResult = deserializer.readF64();
    if (!yResult.success()) return Error(yResult.error());
    m_y = yResult.value();

    auto zResult = deserializer.readF64();
    if (!zResult.success()) return Error(zResult.error());
    m_z = zResult.value();

    auto yawResult = deserializer.readF32();
    if (!yawResult.success()) return Error(yawResult.error());
    m_yaw = yawResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) return Error(pitchResult.error());
    m_pitch = pitchResult.value();

    return {};
}

// ==================== EntityActionPacket ====================

Result<std::vector<u8>> EntityActionPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeVarInt(static_cast<i32>(m_entityId));
    serializer.writeVarInt(static_cast<i32>(m_action));
    serializer.writeVarInt(m_auxData);
    return serializer.buffer();
}

Result<void> EntityActionPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto idResult = deserializer.readVarInt();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = static_cast<u32>(idResult.value());

    auto actionResult = deserializer.readVarInt();
    if (!actionResult.success()) return Error(actionResult.error());
    m_action = static_cast<EntityActionType>(actionResult.value());

    auto auxResult = deserializer.readVarInt();
    if (!auxResult.success()) return Error(auxResult.error());
    m_auxData = auxResult.value();

    // 验证动作类型
    if (static_cast<i32>(m_action) < 0 || static_cast<i32>(m_action) > 8) {
        return Error(ErrorCode::InvalidData, "Invalid entity action type");
    }

    return {};
}

// ==================== UseEntityPacket ====================

Result<std::vector<u8>> UseEntityPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeVarInt(static_cast<i32>(m_entityId));
    serializer.writeVarInt(static_cast<i32>(m_action));

    // INTERACT_AT 需要写入命中位置
    if (m_action == UseEntityAction::InteractAt) {
        serializer.writeF32(m_hitX);
        serializer.writeF32(m_hitY);
        serializer.writeF32(m_hitZ);
    }

    // INTERACT 和 INTERACT_AT 需要写入手
    if (m_action == UseEntityAction::Interact || m_action == UseEntityAction::InteractAt) {
        serializer.writeVarInt(static_cast<i32>(m_hand));
    }

    // 潜行标志
    serializer.writeBool(m_isSneaking);

    return serializer.buffer();
}

Result<void> UseEntityPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    // 读取实体ID
    auto idResult = deserializer.readVarInt();
    if (!idResult.success()) return Error(idResult.error());
    m_entityId = static_cast<u32>(idResult.value());

    // 读取动作类型
    auto actionResult = deserializer.readVarInt();
    if (!actionResult.success()) return Error(actionResult.error());
    m_action = static_cast<UseEntityAction>(actionResult.value());

    // 验证动作类型
    if (static_cast<i32>(m_action) < 0 || static_cast<i32>(m_action) > 2) {
        return Error(ErrorCode::InvalidData, "Invalid use entity action type");
    }

    // INTERACT_AT 需要读取命中位置
    if (m_action == UseEntityAction::InteractAt) {
        auto xResult = deserializer.readF32();
        if (!xResult.success()) return Error(xResult.error());
        m_hitX = xResult.value();

        auto yResult = deserializer.readF32();
        if (!yResult.success()) return Error(yResult.error());
        m_hitY = yResult.value();

        auto zResult = deserializer.readF32();
        if (!zResult.success()) return Error(zResult.error());
        m_hitZ = zResult.value();
    }

    // INTERACT 和 INTERACT_AT 需要读取手
    if (m_action == UseEntityAction::Interact || m_action == UseEntityAction::InteractAt) {
        auto handResult = deserializer.readVarInt();
        if (!handResult.success()) return Error(handResult.error());
        m_hand = static_cast<Hand>(handResult.value());

        // 验证手
        if (static_cast<i32>(m_hand) < 0 || static_cast<i32>(m_hand) > 1) {
            return Error(ErrorCode::InvalidData, "Invalid hand value");
        }
    }

    // 读取潜行标志
    auto sneakingResult = deserializer.readBool();
    if (!sneakingResult.success()) return Error(sneakingResult.error());
    m_isSneaking = sneakingResult.value();

    return {};
}

} // namespace mc::network
