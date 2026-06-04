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

#include "common/core/Types.hpp"
#include <memory>
#include <string>

namespace mc::network {

/**
 * @brief 服务端连接类型
 */
enum class ConnectionType : u8 {
    Tcp,  ///< TCP 远程连接
    Local ///< 本地进程内连接
};

/**
 * @brief 服务端连接接口
 *
 * 抽象服务端与客户端之间的通信连接，支持 TCP 远程连接和本地连接。
 * 这允许 ServerWorld 和 EntityTracker 网络无关，可用于 IntegratedServer 和 StandaloneServer。
 *
 * 使用示例：
 * @code
 * void sendData(ConnectionPtr conn, const std::vector<u8>& data) {
 *     if (conn && conn->isConnected()) {
 *         conn->send(data.data(), data.size());
 *     }
 * }
 * @endcode
 */
class IServerConnection {
public:
    virtual ~IServerConnection() = default;

    /**
     * @brief 发送数据到对端
     * @param data 数据指针
     * @param size 数据大小
     */
    virtual void send(const u8* data, size_t size) = 0;

    /**
     * @brief 断开连接
     * @param reason 断开原因
     */
    virtual void disconnect(const std::string& reason = "") = 0;

    /**
     * @brief 检查是否已连接
     * @return true 如果连接有效
     */
    [[nodiscard]] virtual bool isConnected() const = 0;

    /**
     * @brief 获取连接标识符（用于日志和调试）
     * @return 标识符字符串
     */
    [[nodiscard]] virtual std::string identifier() const = 0;

    /**
     * @brief 获取连接类型
     * @return 连接类型
     */
    [[nodiscard]] virtual ConnectionType type() const = 0;

    /**
     * @brief 获取远程地址（IP 地址）
     *
     * 对于 TCP 连接，返回客户端的 IP 地址（如 "192.168.1.100"）。
     * 对于本地连接，返回空字符串。
     *
     * @return IP 地址字符串，本地连接返回空字符串
     */
    [[nodiscard]] virtual std::string getAddress() const = 0;
};

/// 连接共享指针类型
using ConnectionPtr = std::shared_ptr<IServerConnection>;

/// 连接弱指针类型
using ConnectionWeakPtr = std::weak_ptr<IServerConnection>;

} // namespace mc::network
