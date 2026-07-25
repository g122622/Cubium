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

#pragma once

#include "common/core/Result.hpp"
#include "common/item/Items.hpp"
#include "common/network/backend/java/JavaProtocolTables.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/codec/StreamCodec.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/registry/RegistryAccess.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/network/ServerHandshake.hpp"
#include "server/network/ServerNetwork.hpp"

#include <gtest/gtest.h>

#include <ios>
#include <memory>
#include <string>
#include <vector>

namespace mc::network::test {

/**
 * @brief 网络模块单元测试共享基类
 *
 * SetUp 幂等初始化方块/物品注册表（VanillaBlocks::initialize / Items::initialize，
 * 均带 s_initialized 守卫，重复调用安全），并构建一次 Java 1.21.11 五阶段包表存于 m_tables，
 * 供所有网络单元测试复用——避免每个 TEST 各自 build（表构建开销集中在 fixture）。
 */
class NetworkTestBase : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        if (m_tables == nullptr) {
            m_tables = backend::java::JavaProtocolTables::build();
        }
    }

    /**
     * @brief 构造一个绑定默认注册表的 RegistryByteBuf（play codec 用 ByteBuf 原语 + opaque 透传，
     *        不强制 registry，但绑定保安全）。
     */
    [[nodiscard]] buffer::RegistryByteBuf makeBoundBuf() const
    {
        buffer::RegistryByteBuf buf;
        buf.bindRegistry(RegistryAccess::instance());
        return buf;
    }

    [[nodiscard]] const std::shared_ptr<pipeline::ProtocolTableSet<buffer::RegistryByteBuf>>& tables() const noexcept
    {
        return m_tables;
    }

private:
    // 跨 TEST 共享：表构建一次即可（gtest 对每个 TEST 创建独立 fixture 实例，故用 inline static）。
    inline static std::shared_ptr<pipeline::ProtocolTableSet<buffer::RegistryByteBuf>> m_tables;
};

/**
 * @brief 表级往返：用某 (phase,flow) 的 ProtocolInfo 把 input 编码再解码，返回解码后的 variant。
 *
 * 首选此路径——它同时验证 packetID 分发 + payload codec。失败时（编码/解码错误）触发
 * gtest ADD_FAILURE 而非抛异常，便于定位。
 *
 * @tparam Variant 阶段变体类型（如 ir::PlayPacket）
 * @param info ProtocolInfo（由 tables()->playSb 等取）
 * @param input 待往返的变体值
 * @return 往返后的变体（调用方按需 std::get 取出具体包）
 */
template <typename Variant>
[[nodiscard]] Variant roundTripGeneric(
    const protocol::ProtocolInfo<buffer::RegistryByteBuf, Variant>& info, const Variant& input)
{
    buffer::RegistryByteBuf encodeBuf;
    encodeBuf.bindRegistry(RegistryAccess::instance());
    auto enc = info.encode(encodeBuf, input);
    if (!enc.success()) {
        ADD_FAILURE() << "encode 失败: " << enc.error().toString();
        return Variant{};
    }

    // 用同一份字节构造读缓冲（带 registry）。
    buffer::RegistryByteBuf decodeBuf(encodeBuf.data(), encodeBuf.size(), RegistryAccess::instance());
    auto dec = info.decode(decodeBuf);
    if (!dec.success()) {
        ADD_FAILURE() << "decode 失败: " << dec.error().toString();
        return Variant{};
    }
    return dec.value();
}

/**
 * @brief Play 包表级往返（便捷重载，自动选 playSb/playCb）。
 *
 * @param useSb true=用 playSb 表（C→S 包），false=用 playCb 表（S→C 包）
 */
[[nodiscard]] inline ir::PlayPacket roundTripPlay(
    const pipeline::ProtocolTableSet<buffer::RegistryByteBuf>& tables, const ir::PlayPacket& input, bool useSb)
{
    if (useSb) {
        return roundTripGeneric(*tables.playSb, input);
    }
    return roundTripGeneric(*tables.playCb, input);
}

/**
 * @brief 字节流相等兜底：用同一 codec 分别编码 a 与 b，比较产生的字节流。
 *
 * 用于无法直接 EXPECT_EQ 的场景：shared_ptr<NBT>（指针相等无意义）、缺显式 operator== 的 struct。
 * 同 codec 输入相同语义值应产生相同字节（CFB8/AES 无随机 IV、NBT 序列化确定）。
 */
template <typename Codec, typename V>
[[nodiscard]] ::testing::AssertionResult byteStreamsEqual(const Codec& codec, const V& a, const V& b)
{
    buffer::RegistryByteBuf bufA;
    bufA.bindRegistry(RegistryAccess::instance());
    codec.encode(bufA, a);

    buffer::RegistryByteBuf bufB;
    bufB.bindRegistry(RegistryAccess::instance());
    codec.encode(bufB, b);

    const auto& bytesA = bufA.bytes();
    const auto& bytesB = bufB.bytes();
    if (bytesA.size() != bytesB.size()) {
        return ::testing::AssertionFailure() << "字节长度不等: " << bytesA.size() << " vs " << bytesB.size();
    }
    for (usize i = 0; i < bytesA.size(); ++i) {
        if (bytesA[i] != bytesB[i]) {
            return ::testing::AssertionFailure()
                << "第 " << i << " 字节不等: 0x" << std::hex << static_cast<int>(bytesA[i]) << " vs 0x"
                << static_cast<int>(bytesB[i]);
        }
    }
    return ::testing::AssertionSuccess();
}

/**
 * @brief Local 模式服务端夹具：一对 LocalTransport 互连服务端 ServerClientConnection
 *        与客户端侧 ILocalTransport，无 socket 无线程，纯进程内驱动。
 *
 * 用于 ServerHandshakeStateMachine / ServerNetwork Local 路径的单元测试。
 * - serverConn()：服务端侧连接（ServerNetwork 拥有，返回裸指针），其 onPacket 监听器
 *   由测试自行装配（典型：handshake.handleInbound 分流）。
 * - clientTransport()：客户端侧 ILocalTransport，测试用它 send 包到服务端，再调
 *   serverConn()->pumpLocal() 驱动到达。
 * - pumpServer()：便捷封装，等价于 serverConn()->pumpLocal()。
 *
 * 复用 NetworkTestBase 的注册表/表初始化（VanillaBlocks::initialize 等）。
 */
class LocalServerFixture : public NetworkTestBase {
protected:
    using PlayerReadySignature = void(const std::string& username, const std::array<u8, 16>& offlineUuid);

    void SetUp() override
    {
        NetworkTestBase::SetUp();
        m_serverNetwork = std::make_unique<mc::server::net::ServerNetwork>();
        std::unique_ptr<mc::network::transport::ILocalTransport> clientSide;
        m_serverConn = m_serverNetwork->createLocalClientSide(&clientSide);
        m_clientTransport = std::move(clientSide);
    }

    void TearDown() override
    {
        // 先关客户端侧 transport 再销毁 ServerNetwork，避免对端 pump 到已析构连接。
        m_clientTransport.reset();
        m_serverNetwork.reset();
    }

    /// 在服务端连接上装配离线握手状态机 + onPlayerReady 捕获回调 + onPacket 分流监听器。
    /// 返回握手状态机引用（测试可断言 playReady() 等状态）。
    mc::server::net::ServerHandshakeStateMachine& installOfflineHandshake(i32 compressionThreshold = -1)
    {
        m_handshake = std::make_unique<mc::server::net::ServerHandshakeStateMachine>(
            *m_serverConn, /*isOfflineMode=*/true, compressionThreshold);
        m_handshake->onPlayerReady([this](const std::string& username, const std::array<u8, 16>& offlineUuid) {
            ++m_playerReadyCount;
            m_readyUsername = username;
            m_readyUuid = offlineUuid;
        });
        m_serverConn->onPacket([this](const mc::network::ir::IrPacket& packet) {
            if (m_handshake != nullptr) {
                auto r = m_handshake->handleInbound(packet);
                if (!r.success()) {
                    // 记录而非 ADD_FAILURE：握手失败是否预期由各测试自行断言
                    // （如 UnsupportedIntentionRejected 故意触发错误）。
                    ++m_inboundErrorCount;
                    m_lastInboundError = r.error().toString();
                    return;
                }
                if (r.value()) {
                    return; // 握手/Configuration 包已消费
                }
            }
            // Play 包：本夹具不接路由器，仅计数（握手测试不驱动 Play 逻辑）。
            ++m_playPacketCount;
        });
        return *m_handshake;
    }

    /// 客户端侧发一个包到服务端入站队列（不触发回调，须 pumpServer 驱动）。
    void clientSend(mc::network::ir::IrPacket packet)
    {
        ASSERT_NE(m_clientTransport, nullptr);
        auto r = m_clientTransport->send(std::move(packet));
        ASSERT_TRUE(r.success()) << "client transport send failed: " << r.error().toString();
    }

    /// 驱动服务端连接 pump 一次（主线程派发对端投递的包）。
    void pumpServer() { m_serverConn->pumpLocal(); }

    [[nodiscard]] mc::server::net::ServerClientConnection* serverConn() const noexcept { return m_serverConn; }
    [[nodiscard]] mc::server::net::ServerNetwork& net() const noexcept { return *m_serverNetwork; }
    [[nodiscard]] int playerReadyCount() const noexcept { return m_playerReadyCount; }
    [[nodiscard]] int playPacketCount() const noexcept { return m_playPacketCount; }
    [[nodiscard]] int inboundErrorCount() const noexcept { return m_inboundErrorCount; }
    [[nodiscard]] const std::string& lastInboundError() const noexcept { return m_lastInboundError; }
    [[nodiscard]] const std::string& readyUsername() const noexcept { return m_readyUsername; }
    [[nodiscard]] const std::array<u8, 16>& readyUuid() const noexcept { return m_readyUuid; }

private:
    std::unique_ptr<mc::server::net::ServerNetwork> m_serverNetwork;
    mc::server::net::ServerClientConnection* m_serverConn = nullptr;
    std::unique_ptr<mc::network::transport::ILocalTransport> m_clientTransport;
    std::unique_ptr<mc::server::net::ServerHandshakeStateMachine> m_handshake;

    int m_playerReadyCount = 0;
    int m_playPacketCount = 0;
    int m_inboundErrorCount = 0;
    std::string m_lastInboundError;
    std::string m_readyUsername;
    std::array<u8, 16> m_readyUuid{};
};

} // namespace mc::network::test
