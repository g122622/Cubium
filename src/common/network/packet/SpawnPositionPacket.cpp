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

#include "SpawnPositionPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

SpawnPositionPacket::SpawnPositionPacket()
    : Packet(PacketType::SpawnPosition)
    , m_position(0, 0, 0)
    , m_angle(0.0f)
{}

SpawnPositionPacket::SpawnPositionPacket(const BlockPos& pos, f32 angle)
    : Packet(PacketType::SpawnPosition)
    , m_position(pos)
    , m_angle(angle)
{}

Result<std::vector<u8>> SpawnPositionPacket::serialize() const
{
    PacketSerializer ser;

    // 写入出生点坐标
    ser.writeI32(m_position.x);
    ser.writeI32(m_position.y);
    ser.writeI32(m_position.z);

    // 写入出生点偏航角
    ser.writeF32(m_angle);

    return ser.buffer();
}

Result<void> SpawnPositionPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deser(data, size);

    // 读取出生点坐标
    auto xResult = deser.readI32();
    if (xResult.failed()) {
        return xResult.error();
    }

    auto yResult = deser.readI32();
    if (yResult.failed()) {
        return yResult.error();
    }

    auto zResult = deser.readI32();
    if (zResult.failed()) {
        return zResult.error();
    }

    // 读取出生点偏航角
    auto angleResult = deser.readF32();
    if (angleResult.failed()) {
        // 协议要求 angle 字段，但为了向后兼容，允许缺失
        m_angle = 0.0f;
    } else {
        m_angle = angleResult.value();
    }

    m_position = BlockPos(xResult.value(), yResult.value(), zResult.value());

    return Result<void>::ok();
}

size_t SpawnPositionPacket::expectedSize() const
{
    // 12字节包头 + 12字节数据 (3个i32) + 4字节 (f32 angle) = 16字节数据
    return sizeof(PacketHeader) + sizeof(i32) * 3 + sizeof(f32);
}

} // namespace mc::network
