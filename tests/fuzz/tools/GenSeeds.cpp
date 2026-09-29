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

// 种子生成器：为 fuzz_java_codec / _sb / _cb 产出结构化起始语料。
//
// 做法：遍历 5 阶段 × 2 流向共 10 张包表的**每个已登记包**，用默认构造的字段值经
// 项目自身的 codec 编码成合法 wire 字节，再前置 2 字节选择子（阶段、流向）写成种子文件。
//
// 为什么不采用「由 tests/e2e/bot 的 bot-trace-*.jsonl 反编码」：
//   - trace 里的 params 是**解码后的字段 JSON**（且是 prismarine/nmp 的包名，与官方名
//     存在别名差异），要反编码就得为每个包手写"JSON 字段 → IR 字段"的映射，覆盖度受
//     限于 bot 实际发过的包；
//   - 本做法直接复用**项目自身的编码器**，天然与解码侧对称，且覆盖到每一个已登记的
//     packet id（含 e2e 从未触发过的包），起始语料质量更高、维护成本更低。
//   trace 的价值仍可保留：用 tests/e2e 的用例清单核对"真实客户端会发哪些包"，据此
//   决定优先级（见 docs/test/FUZZING.md）。

#include "support/FakeTransport.hpp"
#include "support/FuzzSupport.hpp"

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"
#include "common/registry/RegistryAccess.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace mc;
using namespace mc::network;
using mc::fuzz::FuzzBuf;

namespace {

std::size_t g_written = 0;
std::size_t g_connWritten = 0;
std::filesystem::path g_outDir;

/// Serverbound 全量消息序列（按阶段顺序累加），供状态机 harness 作起始语料。
/// 状态机 fuzz 的关键是"报文顺序"，而随机序列几乎不可能自然形成合法的阶段推进，
/// 故直接给出一条含全部 Serverbound 包的序列作为起点。
std::vector<u8> g_serverboundSequence;

/// 阶段/流向的取值（与 FuzzJavaCodec 的选择子约定一致）。
constexpr std::size_t kFlowServerbound = 0;
constexpr std::size_t kFlowClientbound = 1;

/// 连接 harness 的控制头：bit1 置位表示压缩启用（与生成侧的 setupCompression(256) 对应），
/// [1] 初始阶段、[2] 分块粒度选择子（63 → 64 字节一块）、[3] 密钥种子（未启用加密）。
constexpr u8 kConnectionFlagsCompressionOn = 0x02;
constexpr u8 kConnectionChunkSelector = 63;
constexpr u8 kConnectionSecretSeed = 0;

/**
 * @brief 写一个种子文件：2 字节选择子 + 编码后的 packetID+payload
 *
 * 按流向分目录输出（sb/ 与 cb/）：fuzz_java_codec_sb / _cb 只读自己那一份，
 * 避免把对向的种子喂给它们造成无谓的启动开销。
 */
void writeSeed(std::size_t phase, std::size_t flow, std::size_t altIndex, const std::vector<u8>& bytes)
{
    std::vector<u8> seed;
    seed.reserve(bytes.size() + 2);
    seed.push_back(static_cast<u8>(phase));
    seed.push_back(static_cast<u8>(flow));
    seed.insert(seed.end(), bytes.begin(), bytes.end());

    char name[64];
    std::snprintf(name, sizeof(name), "p%zu_f%zu_a%zu.bin", phase, flow, altIndex);

    const std::filesystem::path dir = g_outDir / "java_codec" / (flow == kFlowClientbound ? "cb" : "sb");
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    std::ofstream out(dir / name, std::ios::binary);
    if (!out) {
        return;
    }
    out.write(reinterpret_cast<const char*>(seed.data()), static_cast<std::streamsize>(seed.size()));
    ++g_written;
}

/**
 * @brief 用一条真实 Connection 把 IR 包"发出去"，捕获流水线施加后的完整线上字节
 *
 * 这条种子给 fuzz_connection_wire 用：它的输入是**已经过帧化/压缩的完整帧**，
 * 而不是裸的 packetID+payload。与其在生成器里手工重实现帧化与压缩（一旦流水线改动
 * 就会与真实实现脱节），不如直接复用 Connection::send —— 生成侧与消费侧共享同一实现。
 *
 * 生成侧流向取该包自身的流向（服务端包用 Serverbound 发出），于是消费侧（连接 harness
 * 以 Clientbound 为本端流向）的入站表正好是它。
 */
template <typename Variant>
void writeConnectionSeed(
    std::size_t phase, std::size_t flow, std::size_t altIndex, const Variant& value, std::size_t encodedSize)
{
    if (encodedSize == 0) {
        return; // 该 (阶段, 流向) 无出站表或编码为空，跳过
    }

    auto transportOwner = std::make_unique<fuzz::FakeTransport>();
    fuzz::FakeTransport* transport = transportOwner.get();

    const auto packetFlow =
        (flow == kFlowServerbound) ? protocol::PacketFlow::Serverbound : protocol::PacketFlow::Clientbound;
    pipeline::Connection<FuzzBuf> conn(std::move(transportOwner), fuzz::tables(), packetFlow);
    const auto connectionPhase = static_cast<protocol::ConnectionProtocol>(phase);
    conn.setOutboundPhase(connectionPhase);
    conn.setupCompression(256);

    ir::IrPacket packet;
    packet.phase = connectionPhase;
    packet.packet = value;
    if (!conn.send(std::move(packet)).success()) {
        return;
    }
    if (transport->sentBytes().size() <= 1) {
        return; // 只有长度前缀说明没编出内容
    }

    std::vector<u8> seed;
    seed.reserve(transport->sentBytes().size() + 4);
    seed.push_back(kConnectionFlagsCompressionOn);
    seed.push_back(static_cast<u8>(phase));
    seed.push_back(kConnectionChunkSelector);
    seed.push_back(kConnectionSecretSeed);
    seed.insert(seed.end(), transport->sentBytes().begin(), transport->sentBytes().end());

    char name[64];
    std::snprintf(name, sizeof(name), "p%zu_f%zu_a%zu.bin", phase, flow, altIndex);

    const std::filesystem::path dir = g_outDir / "connection_wire";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    std::ofstream out(dir / name, std::ios::binary);
    if (!out) {
        return;
    }
    out.write(reinterpret_cast<const char*>(seed.data()), static_cast<std::streamsize>(seed.size()));
    ++g_connWritten;
}

/**
 * @brief 尝试把变体的第 I 个备选项编码成种子
 *
 * 只在备选项可默认构造时参与：少数备选项含不可默认构造的成员，跳过即可。
 * 编码失败（如 codec 要求某字段非空）同样跳过——种子必须是**可解码的合法字节**。
 */
template <typename Variant, typename Info, std::size_t I>
void _tryEncode(const Info& info, std::size_t phase, std::size_t flow)
{
    using Alt = std::variant_alternative_t<I, Variant>;
    if constexpr (std::is_default_constructible_v<Alt>) {
        Variant value{std::in_place_index<I>};
        FuzzBuf buf;
        buf.bindRegistry(RegistryAccess::instance());
        if (info.encode(buf, value).success() && !buf.bytes().empty()) {
            writeSeed(phase, flow, I, buf.bytes());
            writeConnectionSeed<Variant>(phase, flow, I, value, buf.bytes().size());
            if (flow == kFlowServerbound && buf.bytes().size() <= 255u) {
                g_serverboundSequence.push_back(static_cast<u8>(buf.bytes().size()));
                g_serverboundSequence.insert(g_serverboundSequence.end(), buf.bytes().begin(), buf.bytes().end());
            }
        }
    }
}

template <typename Variant, typename Info, std::size_t... I>
void _genForVariant(const Info& info, std::size_t phase, std::size_t flow, std::index_sequence<I...>)
{
    (void)std::initializer_list<int>{(_tryEncode<Variant, Info, I>(info, phase, flow), 0)...};
}

template <typename Variant>
void _genTable(const protocol::ProtocolInfo<FuzzBuf, Variant>* info, std::size_t phase, std::size_t flow)
{
    if (info == nullptr) {
        return; // 该 (阶段, 流向) 无表（如 Java 握手无 Clientbound 表）
    }
    _genForVariant<Variant>(*info, phase, flow, std::make_index_sequence<std::variant_size_v<Variant>>{});
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "用法: %s <输出目录>\n", argv[0]);
        return 2;
    }
    g_outDir = argv[1];
    std::error_code ec;
    std::filesystem::create_directories(g_outDir, ec);

    fuzz::initializeOnce();
    const auto& t = fuzz::tables();

    _genTable<ir::HandshakePacket>(t->handshakeSb.get(), 0, kFlowServerbound);
    _genTable<ir::HandshakePacket>(t->handshakeCb.get(), 0, kFlowClientbound);
    _genTable<ir::StatusPacket>(t->statusSb.get(), 1, kFlowServerbound);
    _genTable<ir::StatusPacket>(t->statusCb.get(), 1, kFlowClientbound);
    _genTable<ir::LoginPacket>(t->loginSb.get(), 2, kFlowServerbound);
    _genTable<ir::LoginPacket>(t->loginCb.get(), 2, kFlowClientbound);
    _genTable<ir::ConfigurationPacket>(t->configurationSb.get(), 3, kFlowServerbound);
    _genTable<ir::ConfigurationPacket>(t->configurationCb.get(), 3, kFlowClientbound);
    _genTable<ir::PlayPacket>(t->playSb.get(), 4, kFlowServerbound);
    _genTable<ir::PlayPacket>(t->playCb.get(), 4, kFlowClientbound);

    std::printf("生成 java_codec 种子 %zu 个、connection_wire 种子 %zu 个 -> %s\n",
        g_written,
        g_connWritten,
        g_outDir.string().c_str());

    // 状态机 harness 的起始语料：控制头（离线模式 + 压缩启用）+ 全量 Serverbound 序列。
    if (!g_serverboundSequence.empty()) {
        const std::filesystem::path dir = g_outDir / "handshake_session";
        std::error_code seqEc;
        std::filesystem::create_directories(dir, seqEc);

        std::vector<u8> seed;
        seed.reserve(g_serverboundSequence.size() + 1);
        seed.push_back(0x03); // bit0 离线模式、bit1 压缩阈值 256
        seed.insert(seed.end(), g_serverboundSequence.begin(), g_serverboundSequence.end());

        std::ofstream out(dir / "sb-sequence.bin", std::ios::binary);
        if (out) {
            out.write(reinterpret_cast<const char*>(seed.data()), static_cast<std::streamsize>(seed.size()));
            std::printf("生成 handshake_session 序列种子 %zu 字节\n", seed.size());
        }
    }

    return 0;
}
