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

namespace mc::client {

/**
 * @brief 本地玩家身份信息
 *
 * 维护本地玩家的 playerId 和 entityId 映射关系。
 * 用于网络回调正确路由玩家相关包。
 *
 * ## 设计原则
 *
 * - PlayerId：网络会话标识，用于认证、权限、网络路由
 * - EntityInstanceId：世界实体标识，用于实体系统内部
 * - 两者是独立的标识符，不能互换或强转
 *
 * ## 使用场景
 *
 * 1. 登录成功后，设置本地玩家身份：
 *    ```cpp
 *    m_localIdentity.setIdentity(playerId, entityId);
 *    ```
 *
 * 2. 网络回调中判断是否是本地玩家：
 *    ```cpp
 *    if (m_localIdentity.isLocalPlayerEntity(entityId)) {
 *        // 本地玩家，交给预测系统处理
 *    }
 *    ```
 *
 * 3. 登出时清除身份：
 *    ```cpp
 *    m_localIdentity.clear();
 *    ```
 *
 * ## 线程安全
 *
 * 此类不是线程安全的。调用者需要确保在正确的线程访问。
 */
class LocalPlayerIdentity {
public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个未设置身份的空实例。
     */
    LocalPlayerIdentity() = default;

    /**
     * @brief 析构函数
     */
    ~LocalPlayerIdentity() = default;

    // 禁止拷贝
    LocalPlayerIdentity(const LocalPlayerIdentity&) = delete;
    LocalPlayerIdentity& operator=(const LocalPlayerIdentity&) = delete;

    // 允许移动
    LocalPlayerIdentity(LocalPlayerIdentity&&) noexcept = default;
    LocalPlayerIdentity& operator=(LocalPlayerIdentity&&) noexcept = default;

    // ========== 身份管理 ==========

    /**
     * @brief 设置本地玩家身份
     *
     * 在登录成功后调用，建立 playerId 和 entityId 的映射关系。
     *
     * @param playerId 网络会话标识（由服务端分配）
     * @param entityId 世界实体标识（由服务端 EntityManager 分配）
     *
     * @pre playerId != 0
     * @pre entityId != INVALID_ENTITY_ID
     */
    void setIdentity(PlayerId playerId, EntityInstanceId entityId);

    /**
     * @brief 清除本地玩家身份
     *
     * 在登出或断开连接时调用。
     */
    void clear();

    /**
     * @brief 检查是否已设置身份
     *
     * @return true 如果已设置有效的 playerId 和 entityId
     */
    [[nodiscard]] bool hasIdentity() const;

    // ========== 查询 ==========

    /**
     * @brief 获取玩家ID
     *
     * @return 玩家ID，未设置时返回 0
     */
    [[nodiscard]] PlayerId playerId() const;

    /**
     * @brief 获取实体ID
     *
     * @return 实体ID，未设置时返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId entityId() const;

    /**
     * @brief 检查给定的实体ID是否是本地玩家
     *
     * 这是网络回调中最常用的判断方法。
     * 正确区分本地玩家和远程实体。
     *
     * @param entityId 要检查的实体ID
     * @return true 如果是本地玩家的实体ID
     */
    [[nodiscard]] bool isLocalPlayerEntity(EntityInstanceId entityId) const;

    /**
     * @brief 检查给定的玩家ID是否是本地玩家
     *
     * 用于处理玩家相关的网络包（如聊天消息、权限等）。
     *
     * @param playerId 要检查的玩家ID
     * @return true 如果是本地玩家的玩家ID
     */
    [[nodiscard]] bool isLocalPlayer(PlayerId playerId) const;

private:
    PlayerId m_playerId = 0;
    EntityInstanceId m_entityId = INVALID_ENTITY_ID;
    bool m_hasIdentity = false;
};

} // namespace mc::client
