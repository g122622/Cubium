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

#include "ExplosionPacket.hpp"
#include <cmath>

namespace mc::network {

ExplosionPacket::ExplosionPacket()
    : Packet(PacketType::Explosion)
{}

ExplosionPacket::ExplosionPacket(const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback,
    u64 targetPlayerId)
    : Packet(PacketType::Explosion)
    , m_x(position.x)
    , m_y(position.y)
    , m_z(position.z)
    , m_strength(strength)
    , m_affectedBlocks(affectedBlocks)
{
    // 从玩家击退映射中获取当前玩家的击退向量
    setKnockbackForPlayer(playerKnockback, targetPlayerId);
}

void ExplosionPacket::setKnockbackForPlayer(const std::unordered_map<u64, Vector3>& playerKnockback, u64 playerId)
{
    auto it = playerKnockback.find(playerId);
    if (it != playerKnockback.end()) {
        m_motionX = it->second.x;
        m_motionY = it->second.y;
        m_motionZ = it->second.z;
    } else {
        m_motionX = 0.0f;
        m_motionY = 0.0f;
        m_motionZ = 0.0f;
    }
}

size_t ExplosionPacket::expectedSize() const
{
    // 基础大小：包头(12) + 3*f32(位置) + f32(威力) + VarInt(方块数) + 3*f32(击退)
    // 方块数据：每个方块 3 字节（相对坐标）
    // 保守估计：12 + 12 + 4 + 5 + (方块数 * 3) + 12 = 45 + 方块数 * 3
    return sizeof(PacketHeader) + 40 + m_affectedBlocks.size() * 3;
}

Result<std::vector<u8>> ExplosionPacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    // 写入位置 (f32)
    serializer.writeF32(m_x);
    serializer.writeF32(m_y);
    serializer.writeF32(m_z);

    // 写入威力 (f32)
    serializer.writeF32(m_strength);

    // 写入受影响方块数量 (VarInt)
    serializer.writeVarInt(static_cast<i32>(m_affectedBlocks.size()));

    // 计算爆炸位置的整数部分（作为相对坐标的基准）
    i32 baseX = static_cast<i32>(std::floor(m_x));
    i32 baseY = static_cast<i32>(std::floor(m_y));
    i32 baseZ = static_cast<i32>(std::floor(m_z));

    // 写入方块相对坐标
    // 每个方块使用 3 字节（有符号字节存储相对偏移）
    for (const auto& blockPos : m_affectedBlocks) {
        i32 deltaX = blockPos.x - baseX;
        i32 deltaY = blockPos.y - baseY;
        i32 deltaZ = blockPos.z - baseZ;

        // 检查范围（有符号字节范围 -128 到 127）
        // MC 1.16.5 中如果超出范围，方块会被忽略
        if (deltaX < -128 || deltaX > 127 || deltaY < -128 || deltaY > 127 || deltaZ < -128 || deltaZ > 127) {
            continue;
        }

        serializer.writeI8(static_cast<i8>(deltaX));
        serializer.writeI8(static_cast<i8>(deltaY));
        serializer.writeI8(static_cast<i8>(deltaZ));
    }

    // 写入击退速度 (f32)
    serializer.writeF32(m_motionX);
    serializer.writeF32(m_motionY);
    serializer.writeF32(m_motionZ);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> ExplosionPacket::deserialize(const u8* data, size_t size)
{
    if (size < sizeof(PacketHeader)) {
        return Error(ErrorCode::InvalidData, "ExplosionPacket: insufficient data for header");
    }

    PacketDeserializer deserializer(data, size);

    // 读取位置 (f32)
    auto xResult = deserializer.readF32();
    if (!xResult.success()) {
        return xResult.error();
    }
    m_x = xResult.value();

    auto yResult = deserializer.readF32();
    if (!yResult.success()) {
        return yResult.error();
    }
    m_y = yResult.value();

    auto zResult = deserializer.readF32();
    if (!zResult.success()) {
        return zResult.error();
    }
    m_z = zResult.value();

    // 读取威力 (f32)
    auto strengthResult = deserializer.readF32();
    if (!strengthResult.success()) {
        return strengthResult.error();
    }
    m_strength = strengthResult.value();

    // 读取受影响方块数量
    auto countResult = deserializer.readVarInt();
    if (!countResult.success()) {
        return countResult.error();
    }
    i32 blockCount = countResult.value();

    // 验证方块数量（防止恶意数据）
    constexpr i32 MAX_BLOCKS = 65536; // 合理上限
    if (blockCount < 0 || blockCount > MAX_BLOCKS) {
        return Error(ErrorCode::InvalidData, "ExplosionPacket: invalid block count");
    }

    // 计算基准坐标
    i32 baseX = static_cast<i32>(std::floor(m_x));
    i32 baseY = static_cast<i32>(std::floor(m_y));
    i32 baseZ = static_cast<i32>(std::floor(m_z));

    // 读取方块相对坐标
    m_affectedBlocks.clear();
    m_affectedBlocks.reserve(static_cast<size_t>(blockCount));

    for (i32 i = 0; i < blockCount; ++i) {
        auto deltaXResult = deserializer.readI8();
        if (!deltaXResult.success()) {
            return deltaXResult.error();
        }

        auto deltaYResult = deserializer.readI8();
        if (!deltaYResult.success()) {
            return deltaYResult.error();
        }

        auto deltaZResult = deserializer.readI8();
        if (!deltaZResult.success()) {
            return deltaZResult.error();
        }

        // 还原绝对坐标
        i32 blockX = baseX + static_cast<i32>(deltaXResult.value());
        i32 blockY = baseY + static_cast<i32>(deltaYResult.value());
        i32 blockZ = baseZ + static_cast<i32>(deltaZResult.value());

        m_affectedBlocks.emplace_back(blockX, blockY, blockZ);
    }

    // 读取击退速度 (f32)
    auto motionXResult = deserializer.readF32();
    if (!motionXResult.success()) {
        return motionXResult.error();
    }
    m_motionX = motionXResult.value();

    auto motionYResult = deserializer.readF32();
    if (!motionYResult.success()) {
        return motionYResult.error();
    }
    m_motionY = motionYResult.value();

    auto motionZResult = deserializer.readF32();
    if (!motionZResult.success()) {
        return motionZResult.error();
    }
    m_motionZ = motionZResult.value();

    return {};
}

} // namespace mc::network
