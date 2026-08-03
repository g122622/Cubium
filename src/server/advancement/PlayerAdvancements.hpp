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

#include "common/advancement/Advancement.hpp"
#include "common/advancement/AdvancementProgress.hpp"
#include "common/advancement/AdvancementRewards.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/core/Types.hpp"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class ServerPlayer;

namespace advancement {
class AdvancementManager;
}

} // namespace mc

namespace mc::server {

/**
 * @brief 玩家成就进度管理器
 *
 * 负责追踪玩家在所有成就上的进度，管理触发器监听，
 * 并处理持久化和网络同步。
 */
class PlayerAdvancements {
public:
    using Ptr = std::shared_ptr<PlayerAdvancements>;

    /**
     * @brief 构造函数
     * @param playerId 玩家ID
     */
    explicit PlayerAdvancements(PlayerId playerId);

    /**
     * @brief 析构函数
     */
    ~PlayerAdvancements() noexcept;

    /**
     * @brief 设置关联的 ServerPlayer
     * @param player 服务器玩家指针（非拥有）
     */
    void setServerPlayer(::mc::ServerPlayer* player) { m_player = player; }

    /**
     * @brief 获取关联的 ServerPlayer
     */
    [[nodiscard]] ::mc::ServerPlayer* getServerPlayer() noexcept { return m_player; }
    [[nodiscard]] const ::mc::ServerPlayer* getServerPlayer() const noexcept { return m_player; }

    // ========== 进度操作 ==========

    /**
     * @brief 授予条件
     * @param advancement 成就
     * @param criterion 条件名称
     * @return 是否成功授予
     */
    bool grantCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion);

    /**
     * @brief 撤销条件
     * @param advancement 成就
     * @param criterion 条件名称
     * @return 是否成功撤销
     */
    bool revokeCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion);

    /**
     * @brief 授予所有条件
     * @param advancement 成就
     * @return 是否成功授予任何条件
     */
    bool grantAllCriteria(mc::advancement::AdvancementPtr advancement);

    /**
     * @brief 撤销所有条件
     * @param advancement 成就
     * @return 是否成功撤销任何条件
     */
    bool revokeAllCriteria(mc::advancement::AdvancementPtr advancement);

    // ========== 进度查询 ==========

    /**
     * @brief 获取成就进度
     * @param advancement 成就
     * @return 进度（如果不存在返回nullptr）
     */
    mc::advancement::AdvancementProgress* getProgress(mc::advancement::AdvancementPtr advancement);
    const mc::advancement::AdvancementProgress* getProgress(mc::advancement::AdvancementPtr advancement) const;

    /**
     * @brief 检查成就是否完成
     * @param advancement 成就
     * @return 是否完成
     */
    bool isDone(mc::advancement::AdvancementPtr advancement) const;

    /**
     * @brief 检查成就是否有进度
     * @param advancement 成就
     * @return 是否有进度
     */
    bool hasProgress(mc::advancement::AdvancementPtr advancement) const;

    // ========== 可见性 ==========

    /**
     * @brief 检查成就是否可见
     * @param advancement 成就
     * @return 是否可见
     */
    bool isVisible(mc::advancement::AdvancementPtr advancement) const;

    /**
     * @brief 获取所有可见成就
     */
    const std::set<mc::advancement::AdvancementPtr>& getVisibleAdvancements() const;

    /**
     * @brief 获取进度变化的成就（用于网络同步）
     */
    const std::set<mc::advancement::AdvancementPtr>& getProgressChangedAdvancements() const;

    /**
     * @brief 清除进度变化标记
     */
    void clearProgressChanged();

    // ========== 成就重载响应 ==========

    /**
     * @brief 当成就重新加载时调用
     * @param manager 成就管理器
     */
    void onAdvancementsReloaded(mc::advancement::AdvancementManager& manager);

    /**
     * @brief 初始化成就监听器
     *
     * 遍历成就管理器中所有已注册的成就，为玩家尚未完成且尚未有进度的成就
     * 注册触发器监听器。通常在玩家首次加入服务器时调用。
     *
     * @param manager 成就管理器
     */
    void flushAdvancements(mc::advancement::AdvancementManager& manager);

    // ========== 持久化 ==========

    /**
     * @brief 从JSON加载
     * @param json JSON数据
     * @param manager 成就管理器
     * @return 是否成功
     */
    bool loadFromJson(const nlohmann::json& json, mc::advancement::AdvancementManager& manager);

    /**
     * @brief 保存为JSON
     * @return JSON数据
     */
    nlohmann::json toJson() const;

    // ========== 玩家信息 ==========

    /**
     * @brief 获取玩家ID
     */
    PlayerId getPlayerId() const noexcept { return m_playerId; }

    // ========== 触发器监听管理 ==========

    /**
     * @brief 注册成就的触发器监听
     * @param advancement 成就
     */
    void registerListeners(mc::advancement::AdvancementPtr advancement);

    /**
     * @brief 注销成就的触发器监听
     * @param advancement 成就
     */
    void unregisterListeners(mc::advancement::AdvancementPtr advancement);

private:
    PlayerId m_playerId;
    ::mc::ServerPlayer* m_player = nullptr;                   ///< 关联的 ServerPlayer（非拥有指针）
    mc::advancement::AdvancementManager* m_manager = nullptr; ///< 关联的成就管理器（非拥有指针）

    /// 成就进度映射
    std::map<mc::advancement::AdvancementPtr, mc::advancement::AdvancementProgress> m_progress;

    /// 可见成就集合
    std::set<mc::advancement::AdvancementPtr> m_visible;

    /// 进度变化的成就（用于网络同步）
    std::set<mc::advancement::AdvancementPtr> m_progressChanged;

    /// 可见性变化的成就（用于网络同步）
    std::set<mc::advancement::AdvancementPtr> m_visibilityChanged;

    /// 当前选中的标签页
    mc::advancement::AdvancementPtr m_selectedTab;

    /// 是否首次同步
    bool m_firstSync = true;

    /**
     * @brief 确保成就可见性正确
     *
     * 当单个成就的状态变化时，需要重新评估整棵成就树的可见性，
     * 因为一个成就的完成/撤销可能级联影响子成就的可见性。
     * 使用 AdvancementVisibilityEvaluator 从变更成就所在树的根节点重新计算。
     *
     * @param advancement 状态变化的成就
     */
    void _ensureVisibility(mc::advancement::AdvancementPtr advancement);

    /**
     * @brief 使用 AdvancementVisibilityEvaluator 更新所有成就的可见性
     *
     * 从成就树的根节点开始，使用 MC 原版的递归算法计算每个成就的可见性。
     *
     * @param manager 成就管理器（用于获取根成就列表，不能为nullptr）
     */
    void _updateVisibility(mc::advancement::AdvancementManager* manager = nullptr);

    /**
     * @brief 检查成就是否应该可见（简化回退路径，当 m_manager 为空时使用）
     *
     * 实现 MC 原版的可见性规则（与 AdvancementVisibilityEvaluator 一致）：
     * - 已完成的成就始终可见
     * - 无 display 的成就始终不可见（技术成就）
     * - 隐藏成就（hidden=true）在完成前不可见
     * - 非隐藏且未完成的成就：向上回溯 VISIBILITY_DEPTH(2) 层祖先，
     *   如果有已完成的祖先则可见，否则不可见
     *
     * 注意：此方法仅使用 isDone 判定完成状态，不使用 hasProgress，
     * 与 MC Java 原版行为一致。部分完成（有进度但未完成）不影响可见性。
     *
     * @param advancement 成就
     * @param manager 成就管理器（用于查找父成就，可为nullptr）
     */
    bool _shouldShow(
        mc::advancement::AdvancementPtr advancement, mc::advancement::AdvancementManager* manager = nullptr) const;

    /**
     * @brief 发放成就奖励
     * @param rewards 奖励内容
     */
    void _grantRewards(const mc::advancement::AdvancementRewards& rewards);
};

} // namespace mc::server
