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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SetCameraPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

SetCameraPacket::SetCameraPacket()
    : Packet(PacketType::SetCamera)
{}

SetCameraPacket::SetCameraPacket(u32 cameraEntityId)
    : Packet(PacketType::SetCamera)
    , m_cameraEntityId(cameraEntityId)
{}

Result<std::vector<u8>> SetCameraPacket::serialize() const
{
    PacketSerializer ser;
    ser.writeVarInt(static_cast<i32>(m_cameraEntityId));
    return ser.buffer();
}

Result<void> SetCameraPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deser(data, size);

    auto idResult = deser.readVarInt();
    if (idResult.failed()) {
        return idResult.error();
    }
    m_cameraEntityId = static_cast<u32>(idResult.value());

    return Result<void>::ok();
}

size_t SetCameraPacket::expectedSize() const
{
    // VarInt 最大5字节，但实体ID通常在1-4字节范围
    return 4;
}

} // namespace mc::network
