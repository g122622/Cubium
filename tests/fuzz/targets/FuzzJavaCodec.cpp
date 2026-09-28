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

// Fuzz 目标：Java 1.21.11 线协议五阶段全部包表的解码（packetID 分发 + payload codec）。
//
// 输入语义：第 1 字节选阶段（data[0] % 5），第 2 字节选流向（bit0），其余为
// packetID + payload（即压缩层解出的帧内容，与 Connection::_decodeAndDispatch 的入参一致）。
//
// 覆盖范围：backend/java/codecs/JavaPlayCodecs.hpp、JavaPlayCodecsExtended.hpp、
// JavaConfigurationCodecs.hpp、JavaCodecs.hpp（握手/登录/状态）以及其依赖的 NBT/Component/
// ItemStack/BlockState holder 解析——全部是 header-only，随 JavaProtocolTables.cpp 一起插桩。
//
// 说明：解码失败（未知 packet id、payload 越界、非法 NBT……）会走 Result 错误路径，
// 而生产代码在 Connection::_handleWireBytes 中**静默丢弃**该错误（见 docs/test/FUZZING.md
// 的已知缺陷记录）。此处同样不消费返回值，以保持与被测行为一致。

#include "support/FuzzSupport.hpp"

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"
#include "common/registry/RegistryAccess.hpp"

#include <cstddef>
#include <cstdint>

// 流量方向过滤（编译期，由 CMake 的 DEFINES 注入）：
//   0 = 只测 Serverbound（服务端解码不可信客户端输入 —— 真正的远程攻击面）
//   1 = 只测 Clientbound（客户端解码服务端输入）
//   2 = 两者都测，由输入第 2 字节决定（默认）
//
// 之所以需要单独的目标：Clientbound 方向已发现会触发 OOM 的缺陷，
// 而 libFuzzer 一遇 OOM 即中止整个运行，会把 Serverbound 方向的探索遮蔽掉。
#ifndef MC_FUZZ_FLOW_MODE
#define MC_FUZZ_FLOW_MODE 2
#endif

using namespace mc;
using namespace mc::network;
using mc::fuzz::FuzzBuf;

namespace {

/// 阶段取值个数（Handshaking/Status/Login/Configuration/Play）。
constexpr std::uint8_t kPhaseCount = 5;

/**
 * @brief 用某个 (阶段, 流向) 的包表解码一段字节
 *
 * @param info        包表；为空表示该 (阶段, 流向) 无表（Java 握手无 Clientbound 表）
 * @param payload     packetID + payload
 * @param payloadSize payload 字节数
 */
template <typename Variant>
void _decodeWith(const protocol::ProtocolInfo<FuzzBuf, Variant>* info, const u8* payload, usize payloadSize)
{
    if (info == nullptr) {
        return;
    }
    // RegistryByteBuf 绑定默认注册表单例；物品/方块/实体类型 holder 解码依赖它。
    FuzzBuf buf(payload, payloadSize, RegistryAccess::instance());
    auto result = info->decode(buf);
    (void)result;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // 至少需要 2 字节选择器 + 1 字节 payload。
    if (size < 3) {
        return 0;
    }

    const auto phase = static_cast<protocol::ConnectionProtocol>(data[0] % kPhaseCount);
#if MC_FUZZ_FLOW_MODE == 0
    const auto flow = protocol::PacketFlow::Serverbound;
#elif MC_FUZZ_FLOW_MODE == 1
    const auto flow = protocol::PacketFlow::Clientbound;
#else
    const auto flow = (data[1] & 1u) != 0u ? protocol::PacketFlow::Clientbound : protocol::PacketFlow::Serverbound;
#endif
    const u8* payload = reinterpret_cast<const u8*>(data) + 2;
    const usize payloadSize = size - 2;

    const auto& t = fuzz::tables();

    switch (phase) {
        case protocol::ConnectionProtocol::Handshaking:
            _decodeWith(
                protocol::isClientbound(flow) ? t->handshakeCb.get() : t->handshakeSb.get(), payload, payloadSize);
            break;
        case protocol::ConnectionProtocol::Status:
            _decodeWith(protocol::isClientbound(flow) ? t->statusCb.get() : t->statusSb.get(), payload, payloadSize);
            break;
        case protocol::ConnectionProtocol::Login:
            _decodeWith(protocol::isClientbound(flow) ? t->loginCb.get() : t->loginSb.get(), payload, payloadSize);
            break;
        case protocol::ConnectionProtocol::Configuration:
            _decodeWith(protocol::isClientbound(flow) ? t->configurationCb.get() : t->configurationSb.get(),
                payload,
                payloadSize);
            break;
        case protocol::ConnectionProtocol::Play:
            _decodeWith(protocol::isClientbound(flow) ? t->playCb.get() : t->playSb.get(), payload, payloadSize);
            break;
    }

    return 0;
}
