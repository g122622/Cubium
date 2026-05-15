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

#include "BossInfoPacket.hpp"
#include "PacketSerializer.hpp"
#include "common/util/text/ITextComponent.hpp"
#include <nlohmann/json.hpp>

namespace mc::network {

// ============================================================================
// 构造函数
// ============================================================================

BossInfoPacket::BossInfoPacket()
    : Packet(PacketType::BossInfo)
{}

BossInfoPacket::BossInfoPacket(BossInfoAction action)
    : Packet(PacketType::BossInfo)
    , m_action(action)
{}

// ============================================================================
// 静态工厂方法
// ============================================================================

BossInfoPacket BossInfoPacket::add(u64 uuid,
    std::unique_ptr<text::ITextComponent> name,
    f32 percent,
    u8 color,
    u8 overlay,
    bool darkenSky,
    bool playEndBossMusic,
    bool createFog)
{
    BossInfoPacket packet(BossInfoAction::Add);
    packet.m_uuid = uuid;
    packet.m_nameJson = serializeName(*name);
    packet.m_percent = percent;
    packet.m_color = color;
    packet.m_overlay = overlay;
    packet.m_darkenSky = darkenSky;
    packet.m_playEndBossMusic = playEndBossMusic;
    packet.m_createFog = createFog;
    return packet;
}

BossInfoPacket BossInfoPacket::remove(u64 uuid)
{
    BossInfoPacket packet(BossInfoAction::Remove);
    packet.m_uuid = uuid;
    return packet;
}

BossInfoPacket BossInfoPacket::updatePercent(u64 uuid, f32 percent)
{
    BossInfoPacket packet(BossInfoAction::UpdatePercent);
    packet.m_uuid = uuid;
    packet.m_percent = percent;
    return packet;
}

BossInfoPacket BossInfoPacket::updateName(u64 uuid, std::unique_ptr<text::ITextComponent> name)
{
    BossInfoPacket packet(BossInfoAction::UpdateName);
    packet.m_uuid = uuid;
    packet.m_nameJson = serializeName(*name);
    return packet;
}

BossInfoPacket BossInfoPacket::updateStyle(u64 uuid, u8 color, u8 overlay)
{
    BossInfoPacket packet(BossInfoAction::UpdateStyle);
    packet.m_uuid = uuid;
    packet.m_color = color;
    packet.m_overlay = overlay;
    return packet;
}

BossInfoPacket BossInfoPacket::updateProperties(u64 uuid, bool darkenSky, bool playEndBossMusic, bool createFog)
{
    BossInfoPacket packet(BossInfoAction::UpdateProperties);
    packet.m_uuid = uuid;
    packet.m_darkenSky = darkenSky;
    packet.m_playEndBossMusic = playEndBossMusic;
    packet.m_createFog = createFog;
    return packet;
}

// ============================================================================
// 序列化
// ============================================================================

std::string BossInfoPacket::serializeName(const text::ITextComponent& name)
{
    nlohmann::json json = name.toJson();
    return json.dump();
}

Result<std::vector<u8>> BossInfoPacket::serialize() const
{
    PacketSerializer serializer;

    // 所有操作都包含 UUID 和操作类型
    serializer.writeU64(m_uuid);
    serializer.writeU8(static_cast<u8>(m_action));

    switch (m_action) {
        case BossInfoAction::Add: {
            // 名称 (JSON 字符串)
            serializer.writeString(m_nameJson);
            // 百分比
            serializer.writeF32(m_percent);
            // 颜色
            serializer.writeU8(m_color);
            // 样式
            serializer.writeU8(m_overlay);
            // 标志位
            u8 flags = 0;
            if (m_darkenSky) flags |= 0x01;
            if (m_playEndBossMusic) flags |= 0x02;
            if (m_createFog) flags |= 0x04;
            serializer.writeU8(flags);
            break;
        }

        case BossInfoAction::Remove:
            // 无额外数据
            break;

        case BossInfoAction::UpdatePercent:
            // 百分比
            serializer.writeF32(m_percent);
            break;

        case BossInfoAction::UpdateName:
            // 名称 (JSON 字符串)
            serializer.writeString(m_nameJson);
            break;

        case BossInfoAction::UpdateStyle:
            // 颜色
            serializer.writeU8(m_color);
            // 样式
            serializer.writeU8(m_overlay);
            break;

        case BossInfoAction::UpdateProperties: {
            // 标志位
            u8 propFlags = 0;
            if (m_darkenSky) propFlags |= 0x01;
            if (m_playEndBossMusic) propFlags |= 0x02;
            if (m_createFog) propFlags |= 0x04;
            serializer.writeU8(propFlags);
            break;
        }
    }

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> BossInfoPacket::deserialize(const u8* data, size_t size)
{
    // 最小数据: UUID(8) + Action(1) = 9 字节
    if (size < 9) {
        return Error(ErrorCode::InvalidPacket, "BossInfoPacket: data too short");
    }

    PacketDeserializer deserializer(data, size);

    // 读取 UUID
    auto uuidResult = deserializer.readU64();
    if (!uuidResult.success()) {
        return uuidResult.error();
    }
    m_uuid = uuidResult.value();

    // 读取操作类型
    auto actionResult = deserializer.readU8();
    if (!actionResult.success()) {
        return actionResult.error();
    }
    m_action = static_cast<BossInfoAction>(actionResult.value());

    switch (m_action) {
        case BossInfoAction::Add: {
            // 名称 (JSON 字符串)
            auto nameResult = deserializer.readString();
            if (!nameResult.success()) return nameResult.error();
            m_nameJson = nameResult.value();

            // 百分比
            auto percentResult = deserializer.readF32();
            if (!percentResult.success()) return percentResult.error();
            m_percent = percentResult.value();

            // 颜色
            auto colorResult = deserializer.readU8();
            if (!colorResult.success()) return colorResult.error();
            m_color = colorResult.value();

            // 样式
            auto overlayResult = deserializer.readU8();
            if (!overlayResult.success()) return overlayResult.error();
            m_overlay = overlayResult.value();

            // 标志位
            auto flagsResult = deserializer.readU8();
            if (!flagsResult.success()) return flagsResult.error();
            u8 flags = flagsResult.value();
            m_darkenSky = (flags & 0x01) != 0;
            m_playEndBossMusic = (flags & 0x02) != 0;
            m_createFog = (flags & 0x04) != 0;
            break;
        }

        case BossInfoAction::Remove:
            // 无额外数据
            break;

        case BossInfoAction::UpdatePercent: {
            auto percentResult = deserializer.readF32();
            if (!percentResult.success()) return percentResult.error();
            m_percent = percentResult.value();
            break;
        }

        case BossInfoAction::UpdateName: {
            auto nameResult = deserializer.readString();
            if (!nameResult.success()) return nameResult.error();
            m_nameJson = nameResult.value();
            break;
        }

        case BossInfoAction::UpdateStyle: {
            auto colorResult = deserializer.readU8();
            if (!colorResult.success()) return colorResult.error();
            m_color = colorResult.value();

            auto overlayResult = deserializer.readU8();
            if (!overlayResult.success()) return overlayResult.error();
            m_overlay = overlayResult.value();
            break;
        }

        case BossInfoAction::UpdateProperties: {
            auto flagsResult = deserializer.readU8();
            if (!flagsResult.success()) return flagsResult.error();
            u8 flags = flagsResult.value();
            m_darkenSky = (flags & 0x01) != 0;
            m_playEndBossMusic = (flags & 0x02) != 0;
            m_createFog = (flags & 0x04) != 0;
            break;
        }
    }

    return Result<void>();
}

size_t BossInfoPacket::expectedSize() const
{
    // 基础: UUID(8) + Action(1) = 9 字节
    size_t base = sizeof(PacketHeader) + 9;

    switch (m_action) {
        case BossInfoAction::Add:
            // 名称(变长) + 百分比(4) + 颜色(1) + 样式(1) + 标志位(1)
            return base + (m_nameJson.size() + 5) + 4 + 1 + 1 + 1;

        case BossInfoAction::Remove:
            return base;

        case BossInfoAction::UpdatePercent:
            return base + 4; // 百分比(4)

        case BossInfoAction::UpdateName:
            return base + (m_nameJson.size() + 5); // 名称(变长)

        case BossInfoAction::UpdateStyle:
            return base + 1 + 1; // 颜色(1) + 样式(1)

        case BossInfoAction::UpdateProperties:
            return base + 1; // 标志位(1)
    }

    return base;
}

} // namespace mc::network
