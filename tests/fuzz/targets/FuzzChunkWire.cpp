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

// Fuzz 目标：区块线格式的两条解析路径。
//
// 输入语义：第 1 字节为子路径选择子，其余为数据。
//   0：ChunkSerializer::deserializeChunk —— 项目内部紧凑二进制格式
//      （服务端 ↔ 集成服客户端经 LocalTransport 直传的那一份）；
//   1：ChunkSerializer::deserializeChunkSection —— 单个区块段；
//   2：vanilla 线格式 —— 先用 play Clientbound 包表解出 LevelChunkWithLight IR，
//      再交 VanillaChunkWire::readLevelChunkWithLightIR 翻译为 ChunkData。
//
// 覆盖要点（这三条路径的输入都直接来自对端）：
//   - palette 的 bits/长度/global id；
//   - 光照 nibble 数组长度（每段 2048 字节）与 4 个 BitSet + 2 个 List；
//   - 高度图 Long 数组长度；
//   - 方块实体 NBT（嵌套 NBT 解析）；
//   - 全局 block state id → 内部 id 的映射 miss 路径。

#include "support/FuzzSupport.hpp"

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"
#include "common/network/sync/ChunkSerializer.hpp"
#include "common/network/sync/VanillaChunkWire.hpp"
#include "common/registry/RegistryAccess.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::network;

namespace {

/// 子路径个数（见文件头注释）。
constexpr std::uint8_t kPathCount = 3;

/**
 * @brief 用 play Clientbound 包表解出 IR，再走 VanillaChunkWire 的 IR → ChunkData 翻译
 *
 * 这一层不碰 wire 字节（它的入参就是 IR 结构体），因此必须由 codec 先解出 IR 才能
 * 覆盖到它——这也是本路径与 fuzz_java_codec 的分工：后者只验证 codec 本身。
 */
void _fuzzVanillaChunkWire(const u8* data, usize size)
{
    const auto& tables = fuzz::tables();
    if (tables->playCb == nullptr) {
        return;
    }

    buffer::RegistryByteBuf buf(data, size, RegistryAccess::instance());
    auto decoded = tables->playCb->decode(buf);
    if (decoded.failed()) {
        return;
    }
    // playCb 的表类型即 ProtocolInfo<B, ir::PlayPacket>，故 decode 的返回值已是
    // ir::PlayPacket 变体本身，直接在其中取 LevelChunkWithLight 备选项。
    const auto* chunk = std::get_if<ir::play::LevelChunkWithLight>(&decoded.value());
    if (chunk == nullptr) {
        return; // 该 packet id 不是 level_chunk_with_light
    }
    (void)mc::world::chunk::VanillaChunkWire::readLevelChunkWithLightIR(*chunk);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size < 2) {
        return 0;
    }
    fuzz::initializeOnce();

    const auto* bytes = reinterpret_cast<const u8*>(data) + 1;
    const usize payloadSize = size - 1;

    switch (data[0] % kPathCount) {
        case 0:
            // 内部紧凑格式：坐标取自输入，使区块构造路径也随输入变化。
            (void)ChunkSerializer::deserializeChunk(static_cast<i32>(size), static_cast<i32>(size), bytes, payloadSize);
            break;
        case 1:
            (void)ChunkSerializer::deserializeChunkSection(bytes, payloadSize);
            break;
        default:
            _fuzzVanillaChunkWire(bytes, payloadSize);
            break;
    }

    return 0;
}
