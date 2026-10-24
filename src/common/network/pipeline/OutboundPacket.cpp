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

#include "common/network/pipeline/OutboundPacket.hpp"

#include <type_traits>

namespace mc::network::pipeline {

OutboundPacket::OutboundPacket(ir::IrPacket packet)
    : m_packet(std::move(packet))
{
    // shared_ptr 本身不保证不可变；冻结所有 IR 中持有的共享 NBT。
    if (auto* play = std::get_if<ir::PlayPacket>(&m_packet.packet)) {
        std::visit(
            [](auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, ir::play::BlockEntityData>) {
                    if (value.tag) {
                        value.tag = std::make_shared<nbt::CompoundTag>(*value.tag);
                    }
                } else if constexpr (std::is_same_v<T, ir::play::LevelChunkWithLight>) {
                    for (auto& entity : value.blockEntities) {
                        if (entity.tag) {
                            entity.tag = std::make_shared<nbt::CompoundTag>(*entity.tag);
                        }
                    }
                }
            },
            *play);
    }
}

Result<OutboundPacket::Bytes> OutboundPacket::prepare(
    const void* tables, protocol::PacketFlow flow, i32 threshold, const Encoder& encoder) const
{
    std::lock_guard lock(m_mutex);
    const auto key = std::make_tuple(tables, flow, threshold);
    if (auto found = m_frames.find(key); found != m_frames.end()) {
        return found->second;
    }
    auto result = encoder();
    if (!result.success()) {
        return result.error();
    }
    auto bytes = std::make_shared<const std::vector<u8>>(std::move(result.value()));
    m_frames.emplace(key, bytes);
    return bytes;
}

} // namespace mc::network::pipeline
