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

#include "ServerBossInfo.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class ServerPlayer;

namespace server {

// 前向声明
class CustomServerBossInfoManager;

/**
 * @brief 自定义服务端 Boss 信息
 *
 * 用于 /bossbar 命令创建的自定义 Boss 栏。
 * 支持持久化存储、玩家重连后恢复可见性。
 */
class CustomServerBossInfo : public ServerBossInfo {
public:
    /**
     * @brief 构造函数
     *
     * @param id 资源位置 ID（如 "minecraft:my_bossbar"）
     * @param name 显示名称
     * @param manager 管理器引用（用于网络同步）
     */
    CustomServerBossInfo(
        const ResourceLocation& id, std::unique_ptr<text::ITextComponent> name, CustomServerBossInfoManager& manager);

    /**
     * @brief 析构函数
     */
    ~CustomServerBossInfo() override = default;

    // ========== ID 访问 ==========

    /**
     * @brief 获取资源位置 ID
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    // ========== 数值管理 ==========

    /**
     * @brief 获取当前值
     *
     * @return 当前值 (0 ~ max)
     */
    [[nodiscard]] i32 value() const noexcept { return m_value; }

    /**
     * @brief 设置当前值
     *
     * @param value 新值 (会被 clamp 到 0 ~ max)
     */
    void setValue(i32 value);

    /**
     * @brief 获取最大值
     *
     * @return 最大值
     */
    [[nodiscard]] i32 max() const noexcept { return m_max; }

    /**
     * @brief 设置最大值
     *
     * @param max 新最大值 (必须 >= 1)
     */
    void setMax(i32 max);

    // ========== 玩家管理（覆写） ==========

    /**
     * @brief 添加玩家到可见列表
     *
     * @param player 服务端玩家
     */
    void addPlayer(::mc::ServerPlayer& player) override;

    /**
     * @brief 从可见列表移除玩家
     *
     * @param player 服务端玩家
     */
    void removePlayer(::mc::ServerPlayer& player) override;

    /**
     * @brief 通过 UUID 添加玩家（用于重连恢复）
     *
     * @param playerUuid 玩家 UUID
     */
    void addPlayerByUuid(const std::string& playerUuid);

    /**
     * @brief 移除所有玩家
     */
    void removeAllPlayers() override;

    /**
     * @brief 设置玩家列表
     *
     * @param players 新的玩家列表
     * @return 如果列表有变化返回 true
     */
    bool setPlayers(const std::vector<::mc::ServerPlayer*>& players);

    /**
     * @brief 获取持久化的玩家 UUID 集合
     *
     * 用于重连恢复。
     */
    [[nodiscard]] const std::set<std::string>& playerUuids() const noexcept { return m_playerUuids; }

    // ========== 格式化名称 ==========

    /**
     * @brief 获取格式化的显示名称
     *
     * 返回带颜色和悬停事件的文本组件。
     * 悬停事件显示 Boss 栏 ID。
     *
     * @return 格式化的文本组件
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> formattedName() const;

    // ========== 玩家事件 ==========

    /**
     * @brief 玩家登录时调用
     *
     * 如果玩家在持久化列表中，添加到可见列表。
     *
     * @param player 服务端玩家
     */
    void onPlayerLogin(::mc::ServerPlayer& player);

    /**
     * @brief 玩家登出时调用
     *
     * 从可见列表移除玩家（保留 UUID 记录）。
     *
     * @param player 服务端玩家
     */
    void onPlayerLogout(::mc::ServerPlayer& player);

    // ========== 持久化 ==========

    /**
     * @brief 序列化为 NBT
     */
    [[nodiscard]] nbt::tags::compound_tag toNbt() const;

    /**
     * @brief 从 NBT 反序列化
     *
     * @param nbt NBT 数据
     * @param id 资源位置 ID
     * @param manager 管理器引用
     * @return 反序列化的 Boss 栏实例
     */
    static std::unique_ptr<CustomServerBossInfo> fromNbt(
        const nbt::tags::compound_tag& nbt, const ResourceLocation& id, CustomServerBossInfoManager& manager);

private:
    /**
     * @brief 发送 Boss 信息添加包给指定玩家
     */
    void sendAddPacket(::mc::ServerPlayer& player) override;

    /**
     * @brief 发送 Boss 信息移除包给指定玩家
     */
    void sendRemovePacket(::mc::ServerPlayer& player) override;

    /**
     * @brief 发送 Boss 信息更新包给所有可见玩家
     */
    void broadcastUpdate() override;

    /// 资源位置 ID
    ResourceLocation m_id;

    /// 管理器引用
    CustomServerBossInfoManager& m_manager;

    /// 当前值
    i32 m_value = 0;

    /// 最大值
    i32 m_max = 100;

    /// 持久化的玩家 UUID 集合（用于重连恢复）
    std::set<std::string> m_playerUuids;

    /// 用于生成随机 UUID 的随机数生成器
    static math::Random s_random;
};

} // namespace server
} // namespace mc
