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
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

namespace mc::network::transport {

/**
 * @brief 同进程零拷贝传输接口（直传 ir::IrPacket，不经序列化）
 *
 * Local 模式（集成服同进程）专用。Wire 模式走 ITransport（字节传输）；
 * 本接口传 IR 包对象本身，Connection 内部按模式分流——游戏逻辑只见 Connection::send/onPacket。
 *
 * 实现是一对互连的 LocalTransport（pair）：A.send 把 IrPacket push 到 B 的入队，
 * B.pollFromQueue 取出回调 onPacket。零序列化、零拷贝（IrPacket 按值移动）。
 *
 * TODO(Phase7): 接入 Connection 双模式，IntegratedServer 用 LocalTransportPair。
 */
class ILocalTransport {
public:
    using PacketCallback = std::function<void(ir::IrPacket)>;
    using DisconnectCallback = std::function<void()>;

    virtual ~ILocalTransport() = default;

    /**
     * @brief 投递一个 IR 包到对端
     */
    [[nodiscard]] virtual Result<void> send(ir::IrPacket packet) = 0;

    /**
     * @brief 注册包到达回调
     */
    virtual void onPacket(PacketCallback callback) = 0;

    /**
     * @brief 注册断开回调
     */
    virtual void onDisconnect(DisconnectCallback callback) = 0;

    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
    virtual void close() = 0;

    /**
     * @brief 把队列中待投递的包依次回调 onPacket
     *
     * 由持有方（如集成服 tick）在合适时机调用，驱动包到达。
     * Local 模式必由外部 pump 驱动（push 模型）：send 仅入对端队列，不触发回调。
     * Wire 模式的 ITransport 由接收线程异步驱动，无对应方法。
     */
    virtual void pump() = 0;
};

/**
 * @brief 同进程零拷贝传输实现
 *
 * 一对 LocalTransport 经 LocalTransportPair 互连：send 把包放进对端队列，
 * 对端经 pump()（或线程）从队列取出回调 onPacket。
 *
 * 线程模型：pump 由持有方在合适时机调用（如集成服 tick 中同步 pump），
 * 避免引入额外线程与跨线程回调复杂度。也可由专用线程驱动。
 */
class LocalTransport final : public ILocalTransport {
public:
    LocalTransport() = default;
    ~LocalTransport() override { close(); }

    LocalTransport(const LocalTransport&) = delete;
    LocalTransport& operator=(const LocalTransport&) = delete;

    // === ILocalTransport ===
    [[nodiscard]] Result<void> send(ir::IrPacket packet) override;
    void onPacket(PacketCallback callback) override;
    void onDisconnect(DisconnectCallback callback) override;
    [[nodiscard]] bool isConnected() const noexcept override;
    void close() override;
    void pump() override;

    /**
     * @brief 绑定对端（由 LocalTransportPair 在配对时调用）
     */
    void setPeer(LocalTransport* peer) noexcept { m_peer = peer; }

private:
    LocalTransport* m_peer = nullptr;
    std::queue<ir::IrPacket> m_inbox; // 对端 send 进来的待投递包
    std::mutex m_inboxMutex;
    PacketCallback m_packetCallback;
    DisconnectCallback m_disconnectCallback;
    std::mutex m_callbackMutex;
    std::atomic<bool> m_connected{true};
};

/**
 * @brief 一对互连的 LocalTransport 工厂（集成服同进程两端）
 *
 * 创建两个互为 peer 的 LocalTransport，分别交给客户端侧与服务端侧 Connection。
 */
struct LocalTransportPair {
    std::unique_ptr<LocalTransport> client;
    std::unique_ptr<LocalTransport> server;

    static LocalTransportPair create();
};

} // namespace mc::network::transport
