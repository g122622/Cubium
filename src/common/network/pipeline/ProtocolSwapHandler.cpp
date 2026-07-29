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

#include "common/network/pipeline/ProtocolSwapHandler.hpp"

#include <variant>

namespace mc::network::pipeline {

namespace {

/**
 * @brief 访问器：返回该备选项是否 terminal（不关心具体类型，只看 kTerminal 特征）
 *
 * C++17 不能在 lambda 里直接返回类型相关的编译期常量到运行期 bool，故用结构体访问器。
 * 每个备选项类型经 kIsTerminalV 判定。返回 TerminalProbe{isTerminal}。
 */
struct TerminalProbe {
    bool isTerminal;
};

struct TerminalVisitor {
    template <typename T>
    TerminalProbe operator()(const T&) const
    {
        return TerminalProbe{kIsTerminalV<T>};
    }
};

} // namespace

protocol::ConnectionProtocol ProtocolSwapHandler::nextForTerminal(
    const ir::IrPacket& packet, protocol::PacketFlow /*flow*/)
{
    using protocol::ConnectionProtocol;
    switch (packet.phase) {
        case ConnectionProtocol::Handshaking: {
            // ClientIntention.intendedState: 1=Status, 2=Login。
            const auto* intention = std::get_if<ir::HandshakePacket>(&packet.packet);
            if (intention != nullptr) {
                if (const auto* ci = std::get_if<ir::handshake::ClientIntention>(intention)) {
                    if (ci->intendedState == 1) {
                        return ConnectionProtocol::Status;
                    }
                    return ConnectionProtocol::Login; // 2=Login 或 3=Transfer 均走 Login
                }
            }
            return ConnectionProtocol::Login;
        }
        case ConnectionProtocol::Login:
            // LoginAcknowledged(C→S，terminal）→ Configuration。对齐 MC Java：登录阶段
            // 只有 ServerboundLoginAcknowledgedPacket 是 terminal；LoginFinished/Key 非终端，
            // 阶段切换由收方监听器显式驱动（见 ServerHandshake/ClientNetwork）。
            return ConnectionProtocol::Configuration;
        case ConnectionProtocol::Configuration:
            // FinishConfiguration（双向）→ Play。
            return ConnectionProtocol::Play;
        case ConnectionProtocol::Status:
            return packet.phase;
        case ConnectionProtocol::Play: {
            // ConfigurationAcknowledged（C→S，terminal）→ 回 Configuration（服务端发
            // StartConfiguration 后客户端确认，双方切回配置阶段）。其余 Play 包非 terminal。
            const auto* play = std::get_if<ir::PlayPacket>(&packet.packet);
            if (play != nullptr && std::holds_alternative<ir::play::ConfigurationAcknowledged>(*play)) {
                return ConnectionProtocol::Configuration;
            }
            return ConnectionProtocol::Play;
        }
    }
    return packet.phase;
}

TerminalCheck ProtocolSwapHandler::check(const ir::IrPacket& packet, protocol::PacketFlow flow)
{
    TerminalCheck result;
    // std::visit 对阶段变体再对内层阶段包变体取 TerminalProbe。
    // packet.packet 是 std::variant<5 个阶段变体>，每阶段变体是 std::variant<该阶段包>。
    result.isTerminal = std::visit(
        [](const auto& phaseVariant) { return std::visit(TerminalVisitor{}, phaseVariant).isTerminal; }, packet.packet);
    if (result.isTerminal) {
        result.nextPhase = nextForTerminal(packet, flow);
    } else {
        result.nextPhase = packet.phase;
    }
    return result;
}

} // namespace mc::network::pipeline
