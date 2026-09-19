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
 * 记录本地玩家的实体实例标识，供网络回调判定「这个包是否发给我」。
 *
 * ## 为什么只保存实体标识
 *
 * 服务端有两条互相独立的标识序列：**玩家注册 id** 是玩家列表内的索引，属服务端内部概念、
 * 从不跨网传输；**实体实例 id** 是世界实体的标识。客户端能拿到的只有后者——Login 包首
 * 字段下发的就是它。早期实现假定两者相等并互相强转，在「世界里除玩家外还存在其他实体」
 * 时该假定不成立，会让本地玩家实体指向别的实体。
 *
 * ## 使用场景
 *
 * 1. 登录成功后，设置本地玩家身份：
 *    ```cpp
 *    m_localIdentity.setIdentity(entityId);
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
     * 在登录成功后调用。
     *
     * @param entityId 本地玩家的实体实例标识（由服务端分配，随 Login 包首字段下发）
     *
     * @pre entityId != INVALID_ENTITY_ID
     */
    void setIdentity(EntityInstanceId entityId);

    /**
     * @brief 清除本地玩家身份
     *
     * 在登出或断开连接时调用。
     */
    void clear();

    /**
     * @brief 检查是否已设置身份
     *
     * @return true 如果已设置有效的实体实例 id
     */
    [[nodiscard]] bool hasIdentity() const;

    // ========== 查询 ==========

    /**
     * @brief 获取本地玩家的实体标识
     *
     * @return 实体标识，未设置时返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId entityId() const;

    /**
     * @brief 检查给定的实体标识是否是本地玩家
     *
     * 这是网络回调中最常用的判断方法。
     * 正确区分本地玩家和远程实体。
     *
     * @param entityId 要检查的实体标识
     * @return true 如果是本地玩家的实体标识
     */
    [[nodiscard]] bool isLocalPlayerEntity(EntityInstanceId entityId) const;

private:
    EntityInstanceId m_entityId = INVALID_ENTITY_ID;
    bool m_hasIdentity = false;
};

} // namespace mc::client
