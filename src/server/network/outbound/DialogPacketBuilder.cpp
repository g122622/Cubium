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

#include "server/network/outbound/PacketBuilders.hpp"

#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <nlohmann/json.hpp>

namespace mc::server::net {

Result<mc::network::ir::IrPacket> buildShowDialogIr(const nlohmann::json& definition)
{
    if (!definition.is_object() || !definition.contains("title") || !definition.contains("type") ||
        !definition["type"].is_string())
        return Error(ErrorCode::InvalidData, "Dialog requires type and title");
    auto tag = mc::nbt::jsonToNbtStrictInt(definition);
    const auto& compound = dynamic_cast<const mc::nbt::tags::compound_tag&>(*tag);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{
            mc::network::ir::play::ShowDialog{mc::network::buffer::nbt_io::serializeRootCompoundToBytes(compound)}}};
}

} // namespace mc::server::net
