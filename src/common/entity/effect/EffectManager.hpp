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

#include "EffectInstance.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace mc {

class LivingEntity;

namespace entity {
namespace effect {

/**
 * @brief 效果管理器
 *
 * 管理实体身上的所有效果实例。
 */
class EffectManager {
public:
    EffectManager() noexcept = default;

    // ========== 效果管理 ==========

    /**
     * @brief 添加效果
     * @param effect 效果实例
     * @param entity 受影响的实体
     * @return 是否成功添加（如果被覆盖也返回 true）
     */
    bool addEffect(EffectInstance effect, LivingEntity& entity);

    /**
     * @brief 移除效果
     * @param type 效果类型
     * @param entity 受影响的实体
     */
    void removeEffect(EffectType type, LivingEntity& entity);

    /**
     * @brief 移除所有效果
     * @param entity 受影响的实体
     */
    void removeAllEffects(LivingEntity& entity);

    /**
     * @brief 获取效果实例
     * @param type 效果类型
     * @return 效果实例指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const EffectInstance* getEffect(EffectType type) const;
    [[nodiscard]] EffectInstance* getEffect(EffectType type);

    /**
     * @brief 检查是否有效果
     * @param type 效果类型
     */
    [[nodiscard]] bool hasEffect(EffectType type) const;

    /**
     * @brief 获取效果等级
     * @param type 效果类型
     * @return 效果等级（0 = 无效果，1 = I级，2 = II级，等）
     */
    [[nodiscard]] i32 getEffectLevel(EffectType type) const;

    /**
     * @brief 获取所有效果
     */
    [[nodiscard]] const std::vector<EffectInstance>& getAllEffects() const { return m_effects; }
    [[nodiscard]] std::vector<EffectInstance>& getAllEffects() { return m_effects; }

    // ========== 更新 ==========

    /**
     * @brief 更新所有效果（每tick调用）
     * @param entity 受影响的实体
     */
    void tick(LivingEntity& entity);

    // ========== 变更通知（供实体层广播给追踪者） ==========

    /**
     * @brief 一条待广播的效果变更。
     *
     * EffectManager 自身不持有世界上下文，无法直接下发网络包；它只把「哪些效果新增/更新、
     * 哪些被移除」记进队列，由持有 IWorld 的 LivingEntity 在 tick 中消费并广播
     * （UpdateMobEffect / RemoveMobEffect）。这保持了 common 层对网络层的零依赖。
     */
    struct EffectChange {
        EffectType type;
        bool added; // true=新增或更新（需下发完整效果），false=移除
    };

    /**
     * @brief 取出并清空累计的效果变更队列。
     *
     * 调用方（LivingEntity）在每 tick 消费一次，对 added=true 的项从 `getEffect(type)` 取
     * 当前实例下发 UpdateMobEffect，对 added=false 的项下发 RemoveMobEffect。
     */
    [[nodiscard]] std::vector<EffectChange> takePendingChanges();

    // ========== 查询 ==========

    /**
     * @brief 获取效果数量
     */
    [[nodiscard]] size_t getEffectCount() const { return m_effects.size(); }

    /**
     * @brief 检查是否有任何有益效果
     */
    [[nodiscard]] bool hasBeneficialEffect() const;

    /**
     * @brief 检查是否有任何有害效果
     */
    [[nodiscard]] bool hasHarmfulEffect() const;

private:
    /**
     * @brief 查找效果索引
     * @param type 效果类型
     * @return 效果索引，如果不存在返回 -1
     */
    [[nodiscard]] i32 _findEffectIndex(EffectType type) const;

    /// 记录一条待广播的效果变更（同类型合并为最后一条）。
    void _recordChange(EffectType type, bool added);

private:
    std::vector<EffectInstance> m_effects;
    /// 待广播的效果变更队列（由 LivingEntity::tick 消费）。
    std::vector<EffectChange> m_pendingChanges;
};

} // namespace effect
} // namespace entity
} // namespace mc
