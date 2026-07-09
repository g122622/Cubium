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
#include "common/util/text/ITextComponentFwd.hpp"
#include <memory>
#include <set>

namespace mc {

/**
 * @brief 末影龙 Boss 栏抽象接口
 *
 * 定义在 common 层，用于解耦 EndDragonFight（common）与 ServerBossInfo（server）。
 * EndDragonFight 持有一个 IDragonBossBar 指针，默认为 NullDragonBossBar（无操作）。
 * 服务端在初始化时注入 ServerDragonBossBar 实例以实现真实的网络同步。
 *
 * 接口方法对应 MC 1.21.11 ServerBossEvent 的公开 API，但使用 common 层类型
 * （PlayerId 而非 ServerPlayer&，避免 common 依赖 server）。
 *
 * 生命周期对齐 MC 1.21.11 EndDragonFight.dragonEvent：
 * - EndDragonFight 构造时创建 Boss 栏（默认名称、PINK 颜色、PROGRESS 样式）
 * - tick() 中每 20 tick 更新可见玩家列表
 * - updateDragon() 中同步血量百分比和名称
 * - setDragonKilled() 中设置百分比为 0、隐藏
 * - 整个 EndDragonFight 生命周期内复用同一个 Boss 栏实例
 *
 * 参考: net.minecraft.server.level.ServerBossEvent (MC 1.21.11)
 */
class IDragonBossBar {
public:
    virtual ~IDragonBossBar() = default;

    // ========== 属性更新（触发网络同步） ==========

    /**
     * @brief 设置血量百分比
     * @param percent 新的百分比 (0.0 ~ 1.0)，会被 clamp
     *
     * 对应 MC Java: ServerBossEvent.setProgress(float)
     */
    virtual void setPercent(f32 percent) = 0;

    /**
     * @brief 设置显示名称
     * @param name 新的显示名称（文本组件）
     *
     * 对应 MC Java: ServerBossEvent.setName(Component)
     */
    virtual void setName(std::unique_ptr<text::ITextComponent> name) = 0;

    /**
     * @brief 设置是否可见
     * @param visible 是否可见
     *
     * 对应 MC Java: ServerBossEvent.setVisible(boolean)
     */
    virtual void setVisible(bool visible) = 0;

    // ========== 玩家管理 ==========

    /**
     * @brief 添加玩家到可见列表
     * @param playerId 玩家 ID
     *
     * 对应 MC Java: ServerBossEvent.addPlayer(ServerPlayer)
     */
    virtual void addPlayer(PlayerId playerId) = 0;

    /**
     * @brief 从可见列表移除玩家
     * @param playerId 玩家 ID
     *
     * 对应 MC Java: ServerBossEvent.removePlayer(ServerPlayer)
     */
    virtual void removePlayer(PlayerId playerId) = 0;

    /**
     * @brief 移除所有玩家
     *
     * 对应 MC Java: ServerBossEvent.removeAllPlayers()
     */
    virtual void removeAllPlayers() = 0;

    /**
     * @brief 一次性替换可见玩家列表
     *
     * 计算新旧玩家列表的差集：
     * - 移除不在新列表中的旧玩家（发送 Remove 包）
     * - 添加新列表中的新玩家（发送 Add 包）
     * - 已在列表中且仍在新列表中的玩家不受影响（不发送任何包）
     *
     * 对应 MC Java: EndDragonFight.updatePlayers() 中的 add/remove 差集逻辑。
     * 使用此方法而非逐个 addPlayer/removePlayer 可以避免已追踪玩家
     * 收到不必要的 Remove+Add 包对（客户端闪烁）。
     *
     * @param playerIds 新的完整玩家 ID 集合
     */
    virtual void replacePlayers(const std::set<PlayerId>& playerIds) = 0;

    // ========== 状态查询 ==========

    /**
     * @brief 是否有可见玩家
     *
     * 对应 MC Java: ServerBossEvent.getPlayers().isEmpty()
     * 用于 EndDragonFight.tick() 中判断是否需要执行重的战斗逻辑。
     */
    [[nodiscard]] virtual bool hasPlayers() const = 0;

    /**
     * @brief 获取当前可见玩家集合
     *
     * 返回 Boss 栏当前追踪的玩家 ID 集合的 const 引用。
     * 对应 MC Java: ServerBossEvent.getPlayers()（返回 Collection<ServerPlayer>）。
     * 用于 EndDragonFight 在龙重生时遍历玩家触发 SUMMONED_ENTITY 进度。
     *
     * @return 玩家 ID 集合的 const 引用（实现保证返回非空引用，集合可能为空）
     */
    [[nodiscard]] virtual const std::set<PlayerId>& getPlayers() const = 0;

    /**
     * @brief 获取当前血量百分比
     */
    [[nodiscard]] virtual f32 percent() const = 0;

    /**
     * @brief 是否可见
     */
    [[nodiscard]] virtual bool visible() const = 0;
};

/**
 * @brief 空实现的末影龙 Boss 栏
 *
 * 所有方法均为无操作。用于：
 * - 单元测试中（无需网络同步）
 * - 未注入服务端实现时的默认值
 *
 * 对应 MC Java 中不存在此概念——MC 的 EndDragonFight 总是持有真实的 ServerBossEvent。
 * Cubium 因 common/server 分层而引入此空实现作为默认值。
 */
class NullDragonBossBar final : public IDragonBossBar {
public:
    void setPercent(f32 /*percent*/) override {}
    void setName(std::unique_ptr<text::ITextComponent> /*name*/) override {}
    void setVisible(bool /*visible*/) override {}
    void addPlayer(PlayerId /*playerId*/) override {}
    void removePlayer(PlayerId /*playerId*/) override {}
    void removeAllPlayers() override {}
    void replacePlayers(const std::set<PlayerId>& /*playerIds*/) override {}
    [[nodiscard]] bool hasPlayers() const override { return false; }
    [[nodiscard]] const std::set<PlayerId>& getPlayers() const override
    {
        // 返回对函数局部静态空集合的稳定引用，避免引入 .cpp 文件
        static const std::set<PlayerId> kEmptyPlayers;
        return kEmptyPlayers;
    }
    [[nodiscard]] f32 percent() const override { return 0.0f; }
    [[nodiscard]] bool visible() const override { return false; }
};

} // namespace mc
