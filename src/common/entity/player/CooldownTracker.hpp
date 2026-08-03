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
#include <cstddef>
#include <optional>
#include <unordered_map>

namespace mc {

class Item;

namespace entity::player {

/**
 * @brief 物品冷却追踪器
 *
 * 追踪物品的冷却时间，用于紫颂果、末影珍珠、盾牌等物品。
 * 冷却期间物品无法使用，客户端会显示冷却进度动画。
 *
 * 冷却进度说明：
 * - 1.0 表示冷却刚开始
 * - 0.0 表示冷却结束，物品可用
 * - 客户端渲染时使用 (1.0 - progress) 显示冷却动画
 *
 * 使用示例：
 * @code
 * // 设置冷却
 * cooldownTracker.setCooldown(item, 20);  // 20 ticks = 1秒
 *
 * // 检查冷却
 * if (!cooldownTracker.hasCooldown(item)) {
 *     // 物品可用
 *     useItem();
 *     cooldownTracker.setCooldown(item, 20);
 * }
 *
 * // 获取冷却进度（用于渲染）
 * float progress = cooldownTracker.getCooldownProgress(item, partialTicks);
 * // progress = 0 表示冷却结束
 * // progress = 1 表示冷却刚开始
 * @endcode
 */
class CooldownTracker {
public:
    /**
     * @brief 单个物品的冷却信息
     */
    struct Cooldown {
        i32 createTicks; ///< 冷却开始时的游戏 tick
        i32 expireTicks; ///< 冷却结束时的游戏 tick

        Cooldown(i32 create, i32 expire)
            : createTicks(create)
            , expireTicks(expire)
        {}
    };

    CooldownTracker() = default;
    virtual ~CooldownTracker() = default;

    // 禁止拷贝
    CooldownTracker(const CooldownTracker&) = delete;
    CooldownTracker& operator=(const CooldownTracker&) = delete;

    // 允许移动
    CooldownTracker(CooldownTracker&&) noexcept = default;
    CooldownTracker& operator=(CooldownTracker&&) noexcept = default;

    /**
     * @brief 每游戏 tick 调用，更新冷却状态
     *
     * 递增内部 tick 计数器，并移除已过期的冷却。
     * 必须在每帧调用一次。
     */
    void tick();

    /**
     * @brief 设置物品冷却
     *
     * @param item 物品指针
     * @param ticks 冷却时间（tick）
     */
    void setCooldown(const Item* item, i32 ticks);

    /**
     * @brief 移除物品冷却
     *
     * 立即清除指定物品的冷却状态。
     * 通常用于客户端同步或特殊效果。
     *
     * @param item 物品指针
     */
    void removeCooldown(const Item* item);

    /**
     * @brief 获取冷却进度
     *
     * 返回 0.0-1.0 之间的值：
     * - 0.0 表示冷却结束或无冷却
     * - 1.0 表示冷却刚开始
     * - 中间值表示冷却进度
     *
     * @param item 物品指针
     * @param partialTicks 部分帧时间，用于平滑插值（默认 0.0）
     * @return 冷却进度（0.0-1.0）
     */
    [[nodiscard]] f32 getCooldownProgress(const Item* item, f32 partialTicks = 0.0f) const;

    /**
     * @brief 检查物品是否在冷却中
     *
     * @param item 物品指针
     * @return 如果物品在冷却中返回 true
     */
    [[nodiscard]] bool hasCooldown(const Item* item) const;

    /**
     * @brief 获取冷却剩余时间
     *
     * @param item 物品指针
     * @return 剩余 tick 数，如果无冷却返回 0
     */
    [[nodiscard]] i32 getCooldownTicks(const Item* item) const;

    /**
     * @brief 获取当前游戏 tick
     * @return 当前 tick
     */
    [[nodiscard]] i32 currentTick() const { return m_ticks; }

    /**
     * @brief 检查冷却追踪器是否为空
     * @return 如果没有任何冷却中的物品返回 true
     */
    [[nodiscard]] bool isEmpty() const { return m_cooldowns.empty(); }

    /**
     * @brief 获取所有冷却中的物品数量
     * @return 冷却中的物品数量
     */
    [[nodiscard]] size_t cooldownCount() const { return m_cooldowns.size(); }

protected:
    /**
     * @brief 冷却设置时的回调
     *
     * 子类可重写此方法以实现自定义行为，
     * 如客户端通知、网络同步等。
     *
     * @param item 物品指针
     * @param ticks 冷却时间
     */
    virtual void notifyOnSet(const Item* item, i32 ticks);

    /**
     * @brief 冷却移除时的回调
     *
     * 子类可重写此方法以实现自定义行为。
     *
     * @param item 物品指针
     */
    virtual void notifyOnRemove(const Item* item);

private:
    /// 物品到冷却的映射
    std::unordered_map<const Item*, Cooldown> m_cooldowns;

    /// 当前游戏 tick
    i32 m_ticks = 0;
};

} // namespace entity::player

// 为方便使用，在 mc 命名空间添加别名
using CooldownTracker = entity::player::CooldownTracker;

} // namespace mc
