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

#include "../../core/Types.hpp"
#include "EffectType.hpp"
#include <memory>
#include <optional>
#include <utility>

// Forward declaration
namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt
} // namespace mc

namespace mc {

// 前向声明
class LivingEntity;

namespace entity {
namespace effect {

/**
 * @brief 效果实例
 *
 * 表示一个实体的具体效果实例，包含等级和持续时间。
 */
class EffectInstance {
public:
    /**
     * @brief 构造效果实例
     * @param type 效果类型
     * @param duration 持续时间（tick），-1表示永久
     * @param amplifier 效果等级（0 = I, 1 = II, 等）
     * @param ambient 是否为环境效果（如信标）
     * @param visible 是否显示粒子
     * @param showIcon 是否显示图标
     */
    EffectInstance(EffectType type,
        i32 duration = 600,
        i32 amplifier = 0,
        bool ambient = false,
        bool visible = true,
        bool showIcon = true);

    /**
     * @brief 复制构造
     */
    EffectInstance(const EffectInstance& other);

    /**
     * @brief 移动构造
     */
    EffectInstance(EffectInstance&& other) noexcept = default;

    /**
     * @brief 赋值操作符
     */
    EffectInstance& operator=(const EffectInstance& other);
    EffectInstance& operator=(EffectInstance&& other) noexcept = default;

    // ========== 基本属性 ==========

    [[nodiscard]] EffectType type() const noexcept { return m_type; }
    [[nodiscard]] i32 duration() const noexcept { return m_duration; }
    [[nodiscard]] i32 amplifier() const noexcept { return m_amplifier; }
    [[nodiscard]] bool isAmbient() const noexcept { return m_ambient; }
    [[nodiscard]] bool isVisible() const noexcept { return m_visible; }
    [[nodiscard]] bool showIcon() const noexcept { return m_showIcon; }

    /// 被覆盖的旧效果（vanilla MobEffectInstance.hiddenEffect）。当同类型更强效果施加时，
    /// 旧效果被"隐藏"保留于此，新效果结束后恢复。wire codec 透传（递归 optional Details）。
    /// 返回 nullptr 表示无隐藏效果。本字段在 wire 上以 Bool(present)+递归 Details 编码。
    [[nodiscard]] const EffectInstance* hiddenEffect() const noexcept
    {
        return m_hiddenEffect ? m_hiddenEffect.get() : nullptr;
    }
    void setHiddenEffect(std::shared_ptr<EffectInstance> effect) { m_hiddenEffect = std::move(effect); }

    /**
     * @brief 获取效果等级（1-based，用于显示）
     */
    [[nodiscard]] i32 getEffectLevel() const noexcept { return m_amplifier + 1; }

    /**
     * @brief 检查效果是否过期
     */
    [[nodiscard]] bool isExpired() const noexcept { return m_duration == 0; }

    /**
     * @brief 检查效果是否永久
     */
    [[nodiscard]] bool isPermanent() const noexcept { return m_duration < 0; }

    /**
     * @brief 检查效果是否将在指定tick数内结束
     * @param maxDuration 最大剩余tick数
     * @return 如果效果的剩余持续时间 <= maxDuration 则返回 true
     *
     * 永久效果（duration < 0）始终返回 false。
     * 用于判断是否需要刷新效果（如美西螈的再生效果上限检查）。
     */
    [[nodiscard]] bool endsWithin(i32 maxDuration) const noexcept
    {
        return m_duration >= 0 && m_duration <= maxDuration;
    }

    /**
     * @brief 检查属性修改器是否已应用
     */
    [[nodiscard]] bool isApplied() const noexcept { return m_applied; }

    // ========== 更新 ==========

    /**
     * @brief 更新效果（每tick调用）
     * @param entity 受影响的实体
     * @return 是否仍然有效（true = 继续，false = 移除）
     */
    bool tick(LivingEntity& entity);

    /**
     * @brief 合并另一个效果
     * @param other 要合并的效果
     * @return 是否成功合并
     */
    bool merge(const EffectInstance& other);

    /**
     * @brief 应用效果（添加时调用）
     */
    void apply(LivingEntity& entity);

    /**
     * @brief 移除效果（移除时调用）
     */
    void remove(LivingEntity& entity);

    /**
     * @brief 立即执行效果逻辑（用于瞬间效果）
     *
     * 直接调用效果的 tick 逻辑，不递减持续时间。
     * 用于瞬间效果（InstantHealth、InstantDamage、Saturation）
     * 在添加时立即触发效果。
     *
     * @param entity 受影响的实体
     */
    void applyInstantly(LivingEntity& entity);

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建不祥之兆效果
     * @param level 等级（1-5）
     */
    [[nodiscard]] static EffectInstance badOmen(i32 level = 1);

    /**
     * @brief 创建村庄英雄效果
     * @param level 等级（1-5）
     */
    [[nodiscard]] static EffectInstance heroOfTheVillage(i32 level = 1);

    /**
     * @brief 创建试炼之兆效果
     *
     * 由不祥之兆在试炼刷怪笼范围内转化而来。
     * 持续时间 = 不祥之兆等级 × 15000 ticks
     *
     * @param level 等级（1-5）
     */
    [[nodiscard]] static EffectInstance trialOmen(i32 level = 1);

    /**
     * @brief 创建风充能效果
     *
     * 实体死亡时产生小型风爆效果。
     * 由旋风人的风弹命中或风爆魔咒触发。
     *
     * @param level 等级（1-1）
     */
    [[nodiscard]] static EffectInstance windCharged(i32 level = 1);

    /**
     * @brief 创建袭击之兆效果
     *
     * 不祥之兆在村庄范围内转化为袭击之兆，
     * 随后触发袭击。持续时间与不祥之兆等级相关。
     *
     * @param level 等级（1-5）
     */
    [[nodiscard]] static EffectInstance raidOmen(i32 level = 1);

    // ========== 序列化 ==========

    /**
     * @brief 序列化到 NBT
     * @param tag NBT 复合标签（输出参数）
     *
     * MC 1.16.5 效果格式：
     * - Id (byte): 效果类型ID
     * - Amplifier (byte): 效果等级（0 = I, 1 = II, 等）
     * - Duration (int): 持续时间（tick），-1表示永久
     * - Ambient (byte): 是否为环境效果
     * - ShowParticles (byte): 是否显示粒子
     * - ShowIcon (byte): 是否显示图标
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 反序列化
     * @param tag NBT 复合标签
     * @return 效果实例
     */
    [[nodiscard]] static EffectInstance fromNbt(const nbt::tags::compound_tag& tag);

private:
    /**
     * @brief 执行效果的具体逻辑
     */
    void _applyEffect(LivingEntity& entity);

private:
    EffectType m_type;
    i32 m_duration;
    i32 m_amplifier;
    bool m_ambient;
    bool m_visible;
    bool m_showIcon;
    bool m_applied = false; // 是否已应用属性修改
    /// 被覆盖的旧效果（vanilla hiddenEffect，递归）。shared_ptr 对 incomplete type 析构安全。
    std::shared_ptr<EffectInstance> m_hiddenEffect;
};

} // namespace effect
} // namespace entity
} // namespace mc
