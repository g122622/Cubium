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

// Fuzz 目标：握手四阶段的服务端状态机（消息**序列**变异）
//
// 与其它 harness 的根本区别在于"变异的维度"：它们变异**单条报文的内容**，本目标变异
// **消息的发送顺序与组合**——这正是 AFLnet 那一类状态机 fuzz 的价值所在：
// 越阶段发包、跳过 Configuration、重复/缺失 terminal 包、在错误阶段发终态包等等，
// 都不是"某条报文畸形"能触发的，而是"报文顺序不对"才能触发的。
//
// 输入格式：
//   [0] flags：bit0 离线模式（否则在线模式）、bit1 启用压缩阈值 256（否则 -1 不压缩）
//   [1..] 消息序列：每条 = [u8 len][len 字节 payload]（len==0 表示空消息，跳过）
//         payload 即该阶段 Serverbound 包表的 packetID + 字段字节。
//
// 驱动方式与生产一致：解码出的 IR 包经 LocalTransport 投递给 ServerClientConnection，
// 再 pump 到 ServerHandshakeStateMachine —— 不直接调用状态机，以保证"入站派发链"本身
// 也在覆盖范围内。
//
// 当前阶段由 ProtocolSwapHandler 依据 terminal 包推进（与 Connection 的行为同一实现），
// 用于选择下一条消息的解码表。

#include "support/FuzzSupport.hpp"

#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/ProtocolSwapHandler.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"
#include "common/network/transport/LocalTransport.hpp"
#include "common/registry/RegistryAccess.hpp"
#include "server/network/base/ServerClientConnection.hpp"
#include "server/network/handshake/ServerHandshake.hpp"
#include "server/network/session/ServerNetwork.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

using namespace mc;
using namespace mc::network;
using mc::fuzz::FuzzBuf;

namespace {

/// 握手四阶段各自能收到的 Serverbound 报文的最大 payload（由输入长度字段限定为 u8）。
constexpr usize kMaxMessagePayload = 255;

/**
 * @brief 按指定阶段用 Serverbound 包表解码一条消息
 *
 * 解码失败（未知 packet id、字段越界等）返回 false——生产侧此时会丢弃该包并保留连接，
 * 本 harness 据此跳过该消息，保持与生产一致的语义。
 */
template <typename Variant>
bool _tryDecode(const protocol::ProtocolInfo<FuzzBuf, Variant>* info, const u8* data, usize size, ir::IrPacket& out)
{
    if (info == nullptr) {
        return false;
    }
    FuzzBuf buf(data, size, RegistryAccess::instance());
    auto decoded = info->decode(buf);
    if (decoded.failed()) {
        return false;
    }
    out.packet = std::move(decoded).value();
    return true;
}

/// 按阶段选取 Serverbound 表并解码（服务端只解对端发来的 Serverbound 包）。
bool _decodeServerbound(protocol::ConnectionProtocol phase, const u8* data, usize size, ir::IrPacket& out)
{
    out.phase = phase;
    const auto& t = fuzz::tables();
    switch (phase) {
        case protocol::ConnectionProtocol::Handshaking:
            return _tryDecode(t->handshakeSb.get(), data, size, out);
        case protocol::ConnectionProtocol::Status:
            return _tryDecode(t->statusSb.get(), data, size, out);
        case protocol::ConnectionProtocol::Login:
            return _tryDecode(t->loginSb.get(), data, size, out);
        case protocol::ConnectionProtocol::Configuration:
            return _tryDecode(t->configurationSb.get(), data, size, out);
        case protocol::ConnectionProtocol::Play:
            // Play 阶段的入站处理需要 MinecraftServer（ServerPlayHandler），不在本目标范围。
            return false;
    }
    return false;
}

/**
 * @brief 进程级常驻的 ServerNetwork
 *
 * 两条理由：
 *   1. 它的构造会构建**整套协议表**（10 张表、上百个 std::function 注册），每轮重做会把
 *      吞吐压低一个数量级；
 *   2. 它持有的 m_connections 只增不减，若每轮新建一个实例，烧掉的连接会随迭代累积。
 *
 * 因此只保留一条常驻实例，每轮在建连接后于迭代结束前用 removeConnection 摘除。
 */
server::net::ServerNetwork& _sharedServerNetwork()
{
    static server::net::ServerNetwork instance;
    return instance;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size < 2) {
        return 0;
    }
    fuzz::initializeOnce();
    fuzz::setCurrentInput(data, size);

    const u8 flags = data[0];
    const bool isOfflineMode = (flags & 0x01u) != 0;
    const bool useCompression = (flags & 0x02u) != 0;
    const i32 compressionThreshold = useCompression ? 256 : -1;

    // 与生产同形：常驻 ServerNetwork 建本地连接，状态机挂在服务端连接上。
    server::net::ServerNetwork& serverNetwork = _sharedServerNetwork();
    std::unique_ptr<transport::ILocalTransport> clientSide;
    server::net::ServerClientConnection* serverConn = serverNetwork.createLocalClientSide(&clientSide);
    if (serverConn == nullptr) {
        return 0;
    }

    // 状态机与驱动过程包在独立作用域里：必须保证**状态机先于连接析构**——连接的入站
    // 监听器捕获了状态机的引用，若连接先销毁而状态机后销毁尚可，反之则会让监听器持有
    // 悬垂引用。
    {
        server::net::ServerHandshakeStateMachine handshake(*serverConn, isOfflineMode, compressionThreshold);

        // 状态反馈：记录状态机是否走到"进入 Play"、以及走过的最大阶段。
        // 这些量本身不入断言，但驱动它们的代码路径已被插桩，配合 -use_value_profile=1
        // 即可让覆盖率反馈引导搜索走向新状态。
        bool playerReady = false;
        i32 maxPhaseReached = 0;
        handshake.onPlayerReady([&playerReady](const std::string&, const std::array<u8, 16>&) { playerReady = true; });

        serverConn->onPacket([&handshake, &maxPhaseReached](const ir::IrPacket& packet) {
            const auto phase = static_cast<i32>(packet.phase);
            if (phase > maxPhaseReached) {
                maxPhaseReached = phase;
            }
            (void)handshake.handleInbound(packet);
        });

        auto currentPhase = protocol::ConnectionProtocol::Handshaking;

        usize offset = 1;
        while (offset < size && !playerReady) {
            const usize declared = static_cast<usize>(data[offset]);
            ++offset;
            if (declared == 0) {
                continue;
            }
            const usize remaining = size - offset;
            const usize take = (declared < remaining) ? declared : remaining;
            if (take > kMaxMessagePayload) {
                break;
            }

            ir::IrPacket packet;
            if (_decodeServerbound(currentPhase, reinterpret_cast<const u8*>(data) + offset, take, packet)) {
                // terminal 包推进"当前阶段"，决定后续消息用哪张表解码（与 Connection 同一实现）。
                const auto swap = pipeline::ProtocolSwapHandler::check(packet, protocol::PacketFlow::Clientbound);
                if (swap.isTerminal) {
                    currentPhase = swap.nextPhase;
                }
                (void)clientSide->send(std::move(packet));
                serverConn->pumpLocal();
            }

            offset += take;
        }

        // 状态反馈量：仅供调试观察，值本身不影响解码逻辑。
        (void)maxPhaseReached;
        (void)playerReady;
    }

    // 摘除本轮的连接：ServerNetwork 只增不减，不摘会让连接随迭代无限累积。
    serverNetwork.removeConnection(serverConn->sessionId());

    return 0;
}
