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
#include "common/network/protocol/GameActions.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include "common/world/dimension/end/IDragonBossBar.hpp"
#include <memory>
#include <set>

namespace mc {
namespace server {

// 前向声明
class IServer;

/**
 * @brief 服务端末影龙 Boss 栏
 *
 * 实现 common 层的 IDragonBossBar 接口，内部持有 ServerBossInfo 并通过
 * IServer::connectionManager() 发送 ir::play::BossEvent 实现网络同步。
 *
 * 与 CustomServerBossInfo 的区别：
 * - 不持有 ResourceLocation ID（龙 Boss 栏不是 /bossbar 命令创建的）
 * - 不持久化到 NBT（龙 Boss 栏随 EndDragonFight 运行时存在）
 * - 不维护玩家 UUID 集合（龙 Boss 栏不需要重连恢复）
 *
 * 生命周期由 EndDragonFight 通过 IDragonBossBar 接口管理，
 * ServerDragonBossBar 在 EndDragonFight 构造时由服务端注入。
 *
 * 对应 MC 1.21.11: EndDragonFight.dragonEvent (ServerBossEvent)
 */
class ServerDragonBossBar final : public IDragonBossBar {
public:
    /**
     * @brief 构造函数
     *
     * @param server IServer 引用，用于通过 connectionManager() 发送网络包
     * @param uuid Boss 栏 UUID（由调用方生成，保证唯一性）
     * @param name 初始显示名称（如 "entity.minecraft.ender_dragon" 翻译键）
     * @param color 初始颜色（MC 默认 PINK）
     * @param overlay 初始样式（MC 默认 PROGRESS）
     */
    ServerDragonBossBar(IServer& server,
        Uuid uuid,
        std::unique_ptr<text::ITextComponent> name,
        BossInfoColor color,
        BossInfoOverlay overlay);

    ~ServerDragonBossBar() override;

    // 禁止拷贝
    ServerDragonBossBar(const ServerDragonBossBar&) = delete;
    ServerDragonBossBar& operator=(const ServerDragonBossBar&) = delete;

    // ========== IDragonBossBar 接口实现 ==========

    void setPercent(f32 percent) override;
    void setName(std::unique_ptr<text::ITextComponent> name) override;
    void setVisible(bool visible) override;
    void addPlayer(PlayerId playerId) override;
    void removePlayer(PlayerId playerId) override;
    void removeAllPlayers() override;
    void replacePlayers(const std::set<PlayerId>& playerIds) override;
    [[nodiscard]] bool hasPlayers() const override;
    [[nodiscard]] const std::set<PlayerId>& getPlayers() const override;
    [[nodiscard]] f32 percent() const override;
    [[nodiscard]] bool visible() const override;

private:
    /**
     * @brief 向指定玩家发送 Boss 栏添加包
     */
    void _sendAddPacket(PlayerId playerId);

    /**
     * @brief 向指定玩家发送 Boss 栏移除包
     */
    void _sendRemovePacket(PlayerId playerId);

    /**
     * @brief 向所有可见玩家广播更新包
     * @param action 更新操作类型（UpdatePercent / UpdateName / UpdateProperties）
     */
    void _broadcastUpdate(network::BossInfoAction action);

    /// IServer 引用（用于 connectionManager 发送网络包）
    IServer& m_server;

    /// Boss 栏 UUID
    Uuid m_uuid;

    /// 显示名称
    std::unique_ptr<text::ITextComponent> m_name;

    /// 血量百分比 (0.0 ~ 1.0)
    f32 m_percent = 1.0f;

    /// 颜色
    BossInfoColor m_color;

    /// 样式
    BossInfoOverlay m_overlay;

    /// 是否变暗天空
    bool m_darkenSky = false;

    /// 是否播放末影龙 Boss 音乐
    bool m_playEndBossMusic = true;

    /// 是否创建迷雾
    bool m_createFog = true;

    /// 是否可见
    bool m_visible = true;

    /// 可见玩家 ID 集合
    std::set<PlayerId> m_players;
};

} // namespace server
} // namespace mc
