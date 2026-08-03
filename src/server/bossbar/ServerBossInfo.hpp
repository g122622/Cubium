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

#include "BossInfo.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <cstddef>
#include <memory>
#include <set>
#include <vector>

namespace mc {

// 前向声明
class ServerPlayer;

namespace server {

/**
 * @brief Boss 栏更新类型
 *
 * 用于追踪需要发送哪种更新包。
 */
enum class BossInfoUpdateType : u8 {
    None = 0,            // 无更新
    Add = 1,             // 添加（完整信息）
    Remove = 2,          // 移除
    UpdatePercent = 3,   // 更新百分比
    UpdateName = 4,      // 更新名称
    UpdateStyle = 5,     // 更新样式（颜色和边框）
    UpdateProperties = 6 // 更新属性标志
};

/**
 * @brief 服务端 Boss 信息
 *
 * 扩展 BossInfo，添加玩家管理功能。
 * 管理可见此 Boss 栏的玩家列表，并提供玩家添加/移除功能。
 */
class ServerBossInfo : public BossInfo {
public:
    /**
     * @brief 构造函数
     *
     * @param uuid 唯一标识符
     * @param name 显示名称
     * @param color 颜色
     * @param overlay 样式
     */
    ServerBossInfo(Uuid uuid, std::unique_ptr<text::ITextComponent> name, BossInfoColor color, BossInfoOverlay overlay);

    /**
     * @brief 析构函数
     */
    ~ServerBossInfo() override = default;

    // ========== 玩家管理 ==========

    /**
     * @brief 添加玩家到可见列表
     *
     * 玩家将能看到此 Boss 栏。
     *
     * @param player 服务端玩家
     */
    virtual void addPlayer(::mc::ServerPlayer& player);

    /**
     * @brief 从可见列表移除玩家
     *
     * 玩家将不再能看到此 Boss 栏。
     *
     * @param player 服务端玩家
     */
    virtual void removePlayer(::mc::ServerPlayer& player);

    /**
     * @brief 移除所有玩家
     *
     * 清空可见玩家列表。
     */
    virtual void removeAllPlayers();

    /**
     * @brief 获取可见玩家 ID 列表
     */
    [[nodiscard]] const std::set<PlayerId>& players() const noexcept { return m_players; }

    /**
     * @brief 获取可见玩家数量
     */
    [[nodiscard]] size_t playerCount() const noexcept { return m_players.size(); }

    // ========== 属性设置（覆写以触发同步） ==========

    /**
     * @brief 设置显示名称
     *
     * @param name 新的显示名称
     */
    void setName(std::unique_ptr<text::ITextComponent> name) override;

    /**
     * @brief 设置生命值百分比
     *
     * @param percent 新的百分比
     */
    void setPercent(f32 percent) override;

    /**
     * @brief 设置颜色
     *
     * @param color 新的颜色
     */
    void setColor(BossInfoColor color) override;

    /**
     * @brief 设置样式
     *
     * @param overlay 新的样式
     */
    void setOverlay(BossInfoOverlay overlay) override;

    /**
     * @brief 设置是否变暗天空
     *
     * @param darken 是否变暗
     */
    void setDarkenSky(bool darken) override;

    /**
     * @brief 设置是否播放末影龙 Boss 音乐
     *
     * @param play 是否播放
     */
    void setPlayEndBossMusic(bool play) override;

    /**
     * @brief 设置是否创建迷雾
     *
     * @param create 是否创建
     */
    void setCreateFog(bool create) override;

    /**
     * @brief 设置是否可见
     *
     * @param visible 是否可见
     */
    void setVisible(bool visible) override;

    // ========== 更新类型追踪 ==========

    /**
     * @brief 获取待发送的更新类型
     */
    [[nodiscard]] BossInfoUpdateType pendingUpdateType() const noexcept { return m_pendingUpdateType; }

    /**
     * @brief 清除待发送的更新类型
     */
    void clearPendingUpdate() { m_pendingUpdateType = BossInfoUpdateType::None; }

protected:
    /**
     * @brief 发送 Boss 信息更新包给所有可见玩家
     */
    virtual void broadcastUpdate();

    /**
     * @brief 发送 Boss 信息添加包给指定玩家
     *
     * @param player 玩家
     */
    virtual void sendAddPacket(::mc::ServerPlayer& player);

    /**
     * @brief 发送 Boss 信息移除包给指定玩家
     *
     * @param player 玩家
     */
    virtual void sendRemovePacket(::mc::ServerPlayer& player);

    /// 可见玩家 ID 列表
    std::set<PlayerId> m_players;

    /// 待发送的更新类型
    BossInfoUpdateType m_pendingUpdateType = BossInfoUpdateType::None;
};

} // namespace server
} // namespace mc
