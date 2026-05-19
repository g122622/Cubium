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

#include "../../core/Types.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

namespace mc::network {

/**
 * @brief 本地连接端点 - 线程安全的消息队列
 *
 * 用于进程内通信，类似 MC Java 的 LocalChannel
 */
class LocalEndpoint {
public:
    LocalEndpoint() = default;
    ~LocalEndpoint() = default;

    // 禁止拷贝
    LocalEndpoint(const LocalEndpoint&) = delete;
    LocalEndpoint& operator=(const LocalEndpoint&) = delete;

    /**
     * @brief 发送数据到对端
     * @param data 数据指针
     * @param size 数据大小
     */
    void send(const u8* data, size_t size);

    /**
     * @brief 接收数据（非阻塞）
     * @param outData 输出数据缓冲区
     * @return 是否收到数据
     */
    bool receive(std::vector<u8>& outData);

    /**
     * @brief 接收数据（阻塞，带超时）
     * @param outData 输出数据缓冲区
     * @param timeoutMs 超时毫秒，0 表示无限等待
     * @return 是否收到数据
     */
    bool receiveWait(std::vector<u8>& outData, u32 timeoutMs = 0);

    /**
     * @brief 是否有数据可读
     */
    bool hasData() const;

    /**
     * @brief 获取待处理数据数量
     */
    size_t pendingCount() const;

    /**
     * @brief 连接到对端
     * @param remote 对端端点指针
     */
    void connectTo(LocalEndpoint* remote);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 是否已连接
     */
    bool isConnected() const;

private:
    std::queue<std::vector<u8>> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_connected{false};
    LocalEndpoint* m_remote = nullptr;
};

/**
 * @brief 本地连接对 - 管理一对连接的端点
 */
class LocalConnectionPair {
public:
    LocalConnectionPair() = default;

    /**
     * @brief 建立连接
     */
    void connect();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 客户端端点（主线程使用）
     */
    LocalEndpoint& clientEndpoint() { return m_clientEndpoint; }
    const LocalEndpoint& clientEndpoint() const { return m_clientEndpoint; }

    /**
     * @brief 服务端端点（服务端线程使用）
     */
    LocalEndpoint& serverEndpoint() { return m_serverEndpoint; }
    const LocalEndpoint& serverEndpoint() const { return m_serverEndpoint; }

private:
    LocalEndpoint m_clientEndpoint;
    LocalEndpoint m_serverEndpoint;
};

} // namespace mc::network
