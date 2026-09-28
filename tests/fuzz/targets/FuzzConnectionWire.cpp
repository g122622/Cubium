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

// Fuzz 目标：整条 Wire 入站流水线（Connection + 伪传输）
//
// 分层 harness（FuzzVarintFraming / FuzzCompression / FuzzCipher / FuzzJavaCodec）
// 各测一层，本目标测的是**层与层的组合**——这些边界单层 harness 覆盖不到：
//   - TCP 任意切分：长度前缀被切断、粘包、跨帧残留（m_plainIn 的语义）；
//   - 解密缓冲（m_encryptedIn）与明文缓冲（m_plainIn）的分离是否被破坏
//     （pipeline/README 第 4 条坑：二者混存会导致二次解密损坏数据）；
//   - 压缩层与帧层的相互约束（声明长度/阈值/残留字节）；
//   - terminal 包驱动的阶段切换（ProtocolSwapHandler）；
//   - 帧结构非法时清缓冲并断开连接、单包解码失败时留痕并跳过（本次新增的行为）。
//
// 输入格式（前 4 字节为控制头，其余为待投递字节流）：
//   [0] flags：bit0 启用加密、bit1 启用压缩、bit2 压缩阈值取 0（否则 256）、
//              bit3 本端流向取 Serverbound（否则 Clientbound）
//   [1] 初始阶段（data[1] % 5）
//   [2] 分块粒度 = 1 + data[2] % 64
//   [3] 密钥种子
//   [4..] 投递给传输层的字节流

#include "support/FakeTransport.hpp"
#include "support/FuzzSupport.hpp"

#include "common/network/crypto/Crypt.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>

using namespace mc;
using namespace mc::network;
using mc::fuzz::FuzzBuf;

namespace {

/// 控制头长度。
constexpr usize kHeaderSize = 4;

/// 遍历到叶子包，确保"某变体无人认领"不会让整条解码结果被优化掉。
void _consume(const ir::IrPacket& packet)
{
    std::visit([](const auto& phaseVariant) { std::visit([](const auto& leaf) { (void)leaf; }, phaseVariant); },
        packet.packet);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size <= kHeaderSize) {
        return 0;
    }
    fuzz::initializeOnce();

    const u8 flags = data[0];
    const bool useEncryption = (flags & 0x01u) != 0;
    const bool useCompression = (flags & 0x02u) != 0;
    const bool compressionZeroThreshold = (flags & 0x04u) != 0;
    const bool localIsServerbound = (flags & 0x08u) != 0;
    const auto initialPhase = static_cast<protocol::ConnectionProtocol>(data[1] % 5u);
    const usize chunkSize = 1 + (static_cast<usize>(data[2]) % 64);
    const u8 secretSeed = data[3];

    const auto* stream = reinterpret_cast<const u8*>(data) + kHeaderSize;
    const usize streamSize = size - kHeaderSize;

    auto transportOwner = std::make_unique<fuzz::FakeTransport>();
    fuzz::FakeTransport* transport = transportOwner.get();

    // 入站表取"对向"：本端为 Clientbound（服务端）时入站解码 Serverbound 包表。
    const auto flow = localIsServerbound ? protocol::PacketFlow::Serverbound : protocol::PacketFlow::Clientbound;
    pipeline::Connection<FuzzBuf> conn(std::move(transportOwner), fuzz::tables(), flow);

    conn.setInboundPhase(initialPhase);
    conn.setOutboundPhase(initialPhase);

    if (useCompression) {
        conn.setupCompression(compressionZeroThreshold ? 0 : 256);
    }
    if (useEncryption) {
        std::array<u8, crypto::kSharedSecretBytes> secret{};
        for (usize i = 0; i < secret.size(); ++i) {
            secret[i] = static_cast<u8>(secretSeed + static_cast<u8>(i * 31u));
        }
        (void)conn.setupEncryption(secret);
    }

    usize decodedCount = 0;
    conn.onPacket([&decodedCount](const ir::IrPacket& packet) {
        ++decodedCount;
        _consume(packet);
    });

    // 按输入决定的分块粒度投递。帧结构非法时 Connection 会断开连接，此处据此提前结束
    // （与真实场景一致：断开后不再有字节到达）。
    usize offset = 0;
    while (offset < streamSize && transport->isConnected()) {
        const usize remaining = streamSize - offset;
        const usize take = (remaining < chunkSize) ? remaining : chunkSize;
        transport->feed(stream + offset, take);
        offset += take;
    }

    return 0;
}
