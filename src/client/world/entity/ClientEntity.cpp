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

#include "ClientEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/special/PolarBearEntity.hpp"
#include "common/network/packet/EntityMetadataSerializer.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <cmath>

namespace mc::client {

namespace {
constexpr u8 METADATA_END_MARKER = 0xFF;
constexpr u8 METADATA_TYPE_VAR_INT = 1;
constexpr u8 METADATA_TYPE_SLOT = 6;

i32 readVarIntRaw(const u8* data, size_t size, size_t& offset)
{
    i32 result = 0;
    i32 shift = 0;

    while (offset < size) {
        const u8 byte = data[offset++];
        result |= static_cast<i32>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            return result;
        }
        shift += 7;
    }

    return result;
}
} // namespace

ClientEntity::ClientEntity(EntityId id, const std::string& typeId)
    : m_id(id)
    , m_typeId(typeId)
{}

void ClientEntity::setInterpolationSpeed(f32 speed)
{
    m_interpolationSpeed = std::clamp(speed, 0.01f, 1.0f);
}

void ClientEntity::setPosition(f32 x, f32 y, f32 z)
{
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

void ClientEntity::setTargetPosition(f32 x, f32 y, f32 z)
{
    m_targetPosition = Vector3(x, y, z);
}

void ClientEntity::tickPosition()
{
    // 保存当前位置作为上一帧位置（用于渲染插值）
    m_prevPosition = m_position;
    // 不在这里做平滑插值，平滑插值在 updateInterpolation 中每帧执行
}

void ClientEntity::updateInterpolation(f32 deltaTime)
{
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
        while (yawDiff > 180.0f)
            yawDiff -= 360.0f;
        while (yawDiff < -180.0f)
            yawDiff += 360.0f;
        m_yaw += yawDiff * alpha;
        while (m_yaw > 180.0f)
            m_yaw -= 360.0f;
        while (m_yaw < -180.0f)
            m_yaw += 360.0f;

        // Pitch (范围 -90 到 90)
        f32 pitchDiff = m_targetPitch - m_pitch;
        m_pitch += pitchDiff * alpha;
        m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

        // HeadYaw
        f32 headYawDiff = m_targetHeadYaw - m_headYaw;
        while (headYawDiff > 180.0f)
            headYawDiff -= 360.0f;
        while (headYawDiff < -180.0f)
            headYawDiff += 360.0f;
        m_headYaw += headYawDiff * alpha;
        while (m_headYaw > 180.0f)
            m_headYaw -= 360.0f;
        while (m_headYaw < -180.0f)
            m_headYaw += 360.0f;
    } else {
        // 禁用平滑插值时，直接跳到目标位置
        m_position = m_targetPosition;
        m_yaw = m_targetYaw;
        m_pitch = m_targetPitch;
        m_headYaw = m_targetHeadYaw;
    }
}

Vector3 ClientEntity::getInterpolatedPosition(f32 partialTick) const
{
    return Vector3(m_prevPosition.x + (m_position.x - m_prevPosition.x) * partialTick,
        m_prevPosition.y + (m_position.y - m_prevPosition.y) * partialTick,
        m_prevPosition.z + (m_position.z - m_prevPosition.z) * partialTick);
}

void ClientEntity::setRotation(f32 yaw, f32 pitch)
{
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = yaw;
    m_pitch = pitch;
    m_targetYaw = yaw;
    m_targetPitch = pitch;
}

void ClientEntity::setTargetRotation(f32 yaw, f32 pitch)
{
    m_targetYaw = yaw;
    m_targetPitch = pitch;
}

void ClientEntity::setHeadRotation(f32 headYaw)
{
    m_prevHeadYaw = m_headYaw;
    m_headYaw = headYaw;
    m_targetHeadYaw = headYaw;
}

void ClientEntity::setTargetHeadRotation(f32 headYaw)
{
    m_targetHeadYaw = headYaw;
}

void ClientEntity::tickRotation()
{
    // 保存当前旋转作为上一帧旋转（用于渲染插值）
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_prevHeadYaw = m_headYaw;
    // 不在这里做平滑插值，平滑插值在 updateInterpolation 中每帧执行
}

f32 ClientEntity::getInterpolatedYaw(f32 partialTick) const
{
    return m_prevYaw + (m_yaw - m_prevYaw) * partialTick;
}

f32 ClientEntity::getInterpolatedPitch(f32 partialTick) const
{
    return m_prevPitch + (m_pitch - m_prevPitch) * partialTick;
}

f32 ClientEntity::getInterpolatedHeadYaw(f32 partialTick) const
{
    return m_prevHeadYaw + (m_headYaw - m_prevHeadYaw) * partialTick;
}

void ClientEntity::setVelocity(f32 x, f32 y, f32 z)
{
    m_velocity = Vector3(x, y, z);
}

void ClientEntity::setMetadata(const std::vector<u8>& metadata)
{
    MC_TRACE_EVENT("client.entity", "ClientEntity::setMetadata", "entityId", m_id, "size", metadata.size());

    m_metadata = metadata;
    if (!m_metadata.empty()) {
        (void)network::EntityMetadataSerializer::deserialize(m_metadata, m_dataManager);
        syncMetadataFromDataManager();
    }
}

void ClientEntity::syncMetadataFromDataManager()
{
    // 物品实体特殊处理
    if (m_typeId == entity::EntityTypes::ITEM || m_typeId == "minecraft:item") {
        if (m_dataManager.hasParam(ItemEntity::ITEM_COUNT_PARAM_ID)) {
            if (const auto* value = m_dataManager.getRaw(ItemEntity::ITEM_COUNT_PARAM_ID); value != nullptr) {
                const i32 count = value->get<i32>();
                if (m_itemStack != nullptr && m_itemStack->getCount() != count) {
                    ItemStack updated = *m_itemStack;
                    updated.setCount(count);
                    setItemStack(updated);
                }
            }
        }

        _syncItemEntityMetadataFromRawBytes();
        return;
    }

    // 北极熊站立状态同步
    if (m_typeId == "minecraft:polar_bear" || m_typeId == "polar_bear") {
        if (m_dataManager.hasParam(PolarBearEntity::getStandingParamId())) {
            if (const auto* value = m_dataManager.getRaw(PolarBearEntity::getStandingParamId()); value != nullptr) {
                const bool standing = value->get<bool>();
                setStanding(standing);
            }
        }
    }

    // 河豚膨胀状态同步
    if (m_typeId == "minecraft:pufferfish" || m_typeId == "pufferfish") {
        if (m_dataManager.hasParam(::mc::PufferfishEntity::getPuffStateParamId())) {
            if (const auto* value = m_dataManager.getRaw(::mc::PufferfishEntity::getPuffStateParamId());
                value != nullptr) {
                const i32 puffState = value->get<i32>();
                setPuffState(puffState);
            }
        }
    }
}

void ClientEntity::setItemStack(const ItemStack& stack)
{
    MC_TRACE_EVENT("client.entity",
        "ClientEntity::setItemStack",
        "entityId",
        m_id,
        "count",
        stack.getCount(),
        "isEmpty",
        stack.isEmpty());

    const bool changed = m_itemStack == nullptr || *m_itemStack != stack;
    m_itemStack = std::make_unique<ItemStack>(stack);
    if (changed) {
        _updateItemRenderStateVersion();
    }
}

void ClientEntity::clearItemStack()
{
    if (m_itemStack != nullptr) {
        m_itemStack.reset();
        _updateItemRenderStateVersion();
    }
}

void ClientEntity::updateAnimation(f32 distanceMoved)
{
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
    constexpr f32 LIMB_SWING_MAX = math::TWO_PI * 100.0f;
    if (m_limbSwing > LIMB_SWING_MAX) {
        m_limbSwing -= LIMB_SWING_MAX;
    }

    // 更新相机偏航角
    // cameraYaw 用于披风摆动强度计算
    m_cameraYaw += (distanceMoved - m_cameraYaw) * 0.4f;
}

void ClientEntity::triggerSwingAnimation(i32 hand)
{
    m_swingInProgress = true;
    m_swingTickCounter = 0;
    m_swingHand = hand;
    m_prevSwingProgress = m_swingProgress;
    m_swingProgress = 0.0f;
}

void ClientEntity::triggerHurtAnimation()
{
    m_hurtTime = 10;
}

void ClientEntity::triggerLeaveBedAnimation()
{
    m_sleeping = false;
}

void ClientEntity::updateElytraAngles(f32 targetX, f32 targetY, f32 targetZ)
{
    // 使用平滑插值更新鞘翅角度
    constexpr f32 ELYTRA_INTERPOLATION = 0.1f;
    m_rotateElytraX += (targetX - m_rotateElytraX) * ELYTRA_INTERPOLATION;
    m_rotateElytraY += (targetY - m_rotateElytraY) * ELYTRA_INTERPOLATION;
    m_rotateElytraZ += (targetZ - m_rotateElytraZ) * ELYTRA_INTERPOLATION;
}

void ClientEntity::tick()
{
    m_ticksExisted++;

    // 更新位置和旋转的上一帧状态（用于渲染插值）
    tickPosition();
    tickRotation();

    // 更新挥动动画
    if (m_swingInProgress) {
        ++m_swingTickCounter;
        m_prevSwingProgress = m_swingProgress;
        m_swingProgress = static_cast<f32>(m_swingTickCounter) / static_cast<f32>(DEFAULT_SWING_DURATION);

        if (m_swingTickCounter >= DEFAULT_SWING_DURATION) {
            m_swingInProgress = false;
            m_swingProgress = 0.0f;
            m_prevSwingProgress = 0.0f;
        }
    }

    // 更新受伤时间
    if (m_hurtTime > 0) {
        --m_hurtTime;
    }

    // 更新追踪位置系统
    // 用于披风摆动计算
    m_prevChasingPosX = m_chasingPosX;
    m_prevChasingPosY = m_chasingPosY;
    m_prevChasingPosZ = m_chasingPosZ;

    // 平滑追踪实际位置
    f64 dx = static_cast<f64>(m_position.x) - m_chasingPosX;
    f64 dy = static_cast<f64>(m_position.y) - m_chasingPosY;
    f64 dz = static_cast<f64>(m_position.z) - m_chasingPosZ;
    m_chasingPosX += dx * 0.25;
    m_chasingPosY += dy * 0.25;
    m_chasingPosZ += dz * 0.25;

    // 更新相机偏航角
    // 用于披风摆动强度计算
    m_prevCameraYaw = m_cameraYaw;
    // cameraYaw 基于移动距离，在 updateAnimation 中更新

    // 更新北极熊站立动画
    // 仅对北极熊实体有效
    updateStandingAnimation();

    // 更新吃草动画计时器
    if (m_eatAnimationTimer > 0) {
        --m_eatAnimationTimer;
    }
}

void ClientEntity::updateStandingAnimation()
{
    // 保存上一帧动画值
    m_clientSideStandAnimation0 = m_clientSideStandAnimation;

    // 根据站立状态更新动画
    if (m_isStanding) {
        // 站立时动画增加（最大 6.0）
        m_clientSideStandAnimation = std::clamp(m_clientSideStandAnimation + 1.0f, 0.0f, 6.0f);
    } else {
        // 非站立时动画减少（最小 0.0）
        m_clientSideStandAnimation = std::clamp(m_clientSideStandAnimation - 1.0f, 0.0f, 6.0f);
    }
}

bool ClientEntity::isFallFlying() const
{
    // 从元数据管理器读取 FLAGS_PARAM (id 0)
    if (m_dataManager.hasParam(0)) {
        const auto* value = m_dataManager.getRaw(0);
        if (value != nullptr) {
            // FLAGS_PARAM 类型是 i8
            i8 flags = value->get<i8>();
            return (flags & static_cast<i8>(EntityFlags::FallFlying)) != 0;
        }
    }
    return false;
}

bool ClientEntity::isAngry() const
{
    // 蜜蜂愤怒状态检测
    // 当愤怒时间 > 0 时，蜜蜂处于愤怒状态
    if (m_dataManager.hasParam(1)) {
        const auto* value = m_dataManager.getRaw(1);
        if (value != nullptr) {
            // ANGER_TIME 类型是 i32
            i32 angerTime = value->get<i32>();
            return angerTime > 0;
        }
    }
    return false;
}

void ClientEntity::_updateItemRenderStateVersion()
{
    ++m_itemRenderStateVersion;
}

void ClientEntity::_syncItemEntityMetadataFromRawBytes()
{
    if (m_metadata.empty()) {
        return;
    }

    size_t offset = 0;
    bool itemStackChanged = false;

    while (offset < m_metadata.size()) {
        const u8 index = m_metadata[offset++];
        if (index == METADATA_END_MARKER) {
            break;
        }

        if (offset >= m_metadata.size()) {
            break;
        }

        const u8 typeId = m_metadata[offset++];

        if (index == ItemEntity::ITEM_COUNT_PARAM_ID && typeId == METADATA_TYPE_VAR_INT) {
            const i32 count = readVarIntRaw(m_metadata.data(), m_metadata.size(), offset);
            if (offset <= m_metadata.size() && m_itemStack != nullptr && m_itemStack->getCount() != count) {
                ItemStack updated = *m_itemStack;
                updated.setCount(count);
                m_itemStack = std::make_unique<ItemStack>(updated);
                itemStackChanged = true;
            }
            continue;
        }

        if (index == 3 && typeId == METADATA_TYPE_SLOT) {
            ItemStack stack;
            if (_tryReadMetadataSlot(m_metadata.data(), m_metadata.size(), offset, stack)) {
                if (stack.isEmpty()) {
                    if (m_metadataItemStack.has_value()) {
                        m_metadataItemStack.reset();
                        itemStackChanged = true;
                    }
                } else if (!m_metadataItemStack.has_value() || m_metadataItemStack.value() != stack) {
                    m_metadataItemStack = stack;
                    itemStackChanged = true;
                }
            }
            continue;
        }

        if (!_tryReadMetadataEntry(typeId, m_metadata.data(), m_metadata.size(), offset)) {
            break;
        }
    }

    if (m_metadataItemStack.has_value()) {
        if (m_itemStack == nullptr || *m_itemStack != m_metadataItemStack.value()) {
            m_itemStack = std::make_unique<ItemStack>(m_metadataItemStack.value());
            itemStackChanged = true;
        }
    }

    if (itemStackChanged) {
        _updateItemRenderStateVersion();
    }
}

bool ClientEntity::_tryReadMetadataEntry(u8 typeId, const u8* data, size_t size, size_t& offset)
{
    switch (typeId) {
        case 0: // Byte
        case 7: // Boolean
            if (offset >= size) {
                return false;
            }
            ++offset;
            return true;
        case METADATA_TYPE_VAR_INT:
        case 17: // OptVarInt
        case 13: // OptBlockID
            (void)readVarIntRaw(data, size, offset);
            return offset <= size;
        case 2: // Float
            if (offset + sizeof(f32) > size) {
                return false;
            }
            offset += sizeof(f32);
            return true;
        case 3: // String
        case 4: // TextComponent
        case 5: // OptChat
        {
            const i32 length = readVarIntRaw(data, size, offset);
            if (length < 0 || offset + static_cast<size_t>(length) > size) {
                return false;
            }
            offset += static_cast<size_t>(length);
            return true;
        }
        case METADATA_TYPE_SLOT: {
            ItemStack ignored;
            return _tryReadMetadataSlot(data, size, offset, ignored);
        }
        case 8: // Rotation
            if (offset + sizeof(f32) * 3 > size) {
                return false;
            }
            offset += sizeof(f32) * 3;
            return true;
        case 9: // Position
            if (offset + sizeof(i64) > size) {
                return false;
            }
            offset += sizeof(i64);
            return true;
        case 10: // OptPosition
            if (offset >= size) {
                return false;
            }
            if (data[offset++] != 0) {
                if (offset + sizeof(i64) > size) {
                    return false;
                }
                offset += sizeof(i64);
            }
            return true;
        case 11: // Direction
        case 18: // Pose
            (void)readVarIntRaw(data, size, offset);
            return offset <= size;
        case 12: // OptUUID
            if (offset >= size) {
                return false;
            }
            if (data[offset++] != 0) {
                if (offset + 16 > size) {
                    return false;
                }
                offset += 16;
            }
            return true;
        case 14: // NBT
            if (offset >= size) {
                return false;
            }
            if (data[offset] == 0) {
                ++offset;
                return true;
            }
            return false;
        case 15: // Particle
        case 16: // VillagerData
            return false;
        default:
            return false;
    }
}

bool ClientEntity::_tryReadMetadataSlot(const u8* data, size_t size, size_t& offset, ItemStack& outStack) const
{
    if (offset >= size) {
        return false;
    }

    network::PacketDeserializer deser(data + offset, size - offset);
    auto stackResult = ItemStack::deserialize(deser);
    if (stackResult.failed()) {
        return false;
    }

    outStack = stackResult.value();
    offset = size - deser.remaining();
    return true;
}

} // namespace mc::client
