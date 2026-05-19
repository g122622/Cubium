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

#include "ServerDifficultyPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

ServerDifficultyPacket::ServerDifficultyPacket()
    : Packet(PacketType::ServerDifficulty)
{}

ServerDifficultyPacket::ServerDifficultyPacket(Difficulty difficulty, bool locked)
    : Packet(PacketType::ServerDifficulty)
    , m_difficulty(difficulty)
    , m_locked(locked)
{}

Result<std::vector<u8>> ServerDifficultyPacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    serializer.writeU8(static_cast<u8>(m_difficulty));
    serializer.writeBool(m_locked);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> ServerDifficultyPacket::deserialize(const u8* data, size_t size)
{
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidPacket, "ServerDifficultyPacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    // 跳过包头（由上层处理）
    // 这里直接读取数据

    auto difficultyResult = deserializer.readU8();
    if (difficultyResult.failed()) {
        return difficultyResult.error();
    }
    m_difficulty = static_cast<Difficulty>(difficultyResult.value());

    auto lockedResult = deserializer.readBool();
    if (lockedResult.failed()) {
        return lockedResult.error();
    }
    m_locked = lockedResult.value();

    return Result<void>::ok();
}

size_t ServerDifficultyPacket::expectedSize() const
{
    // difficulty (1 byte) + locked (1 byte)
    return 2;
}

} // namespace mc::network
