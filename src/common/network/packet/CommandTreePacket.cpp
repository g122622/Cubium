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

#include "CommandTreePacket.hpp"

#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

// ============================================================================
// CommandTreePacket 序列化/反序列化实现
// ============================================================================

Result<std::vector<u8>> CommandTreePacket::serialize() const
{
    // 计算 VarInt 编码所需的字节数
    // 字符串长度使用 VarInt 编码，需要计算实际编码字节数
    const size_t jsonSize = std::min(m_treeJson.size(), static_cast<size_t>(MAX_STRING_LENGTH));
    const size_t varIntSize = (jsonSize < 128) ? 1 : (jsonSize < 16384) ? 2 : 3;

    // 预分配缓冲区：VarInt 长度 + JSON 内容
    PacketSerializer serializer(varIntSize + jsonSize);
    serializer.writeString(m_treeJson);

    return serializer.buffer();
}

Result<void> CommandTreePacket::deserialize(const u8* data, size_t size)
{
    if (size == 0) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for command tree payload");
    }

    PacketDeserializer deserializer(data, size);

    auto jsonResult = deserializer.readString();
    if (jsonResult.failed()) {
        return jsonResult.error();
    }

    m_treeJson = jsonResult.value();
    return Result<void>::ok();
}

} // namespace mc::network
