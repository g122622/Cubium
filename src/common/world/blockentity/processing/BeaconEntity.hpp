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
#include "entity/effect/EffectType.hpp"
#include "item/core/ItemStack.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 信标光束段数据
 *
 * 每段光束有自己的颜色和高度。
 */
struct BeaconBeamSegment {
    /// RGB 颜色值 (0.0-1.0)
    std::array<f32, 3> colors{1.0f, 1.0f, 1.0f};

    /// 段高度（方块数）
    i32 height = 1;

    BeaconBeamSegment() = default;

    explicit BeaconBeamSegment(f32 r, f32 g, f32 b)
        : colors{r, g, b}
        , height(1)
    {}

    explicit BeaconBeamSegment(const std::array<f32, 3>& colorArray)
        : colors(colorArray)
        , height(1)
    {}

    /**
     * @brief 增加高度
     */
    void incrementHeight() { ++height; }

    /**
     * @brief 获取红色分量
     */
    [[nodiscard]] f32 red() const { return colors[0]; }

    /**
     * @brief 获取绿色分量
     */
    [[nodiscard]] f32 green() const { return colors[1]; }

    /**
     * @brief 获取蓝色分量
     */
    [[nodiscard]] f32 blue() const { return colors[2]; }
};

/**
 * @brief 信标方块实体
 *
 * 信标是一种提供状态效果的方块，特点：
 * - 金字塔结构（1-4层）决定效果等级
 * - 激活需要金字塔基座和天空光
 * - 提供主效果（速度/急迫/抗性/跳跃/力量）
 * - 提供辅助效果（生命恢复）
 * - 消耗矿物作为燃料
 */
class BeaconEntity : public BlockEntity {
public:
    /// 效果槽位数量
    static constexpr i32 EFFECT_SLOTS = 2;

    /// 金字塔最大层数
    static constexpr i32 MAX_LEVELS = 4;

    /// 效果类型
    using EffectType = entity::effect::EffectType;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BeaconEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BeaconEntity() override;

    // ========== 信标接口 ==========

    /**
     * @brief 获取金字塔等级
     * @return 等级 (0-4)
     */
    [[nodiscard]] i32 getLevel() const { return m_level; }

    /**
     * @brief 设置金字塔等级
     * @param level 等级
     */
    void setLevel(i32 level);

    /**
     * @brief 检查是否激活
     * @return 如果激活返回true
     */
    [[nodiscard]] bool isActive() const { return m_level > 0 && m_primaryEffect.has_value(); }

    /**
     * @brief 获取主效果
     * @return 主效果类型
     */
    [[nodiscard]] const EffectType* getPrimaryEffect() const
    {
        return m_primaryEffect.has_value() ? &m_primaryEffect.value() : nullptr;
    }

    /**
     * @brief 设置主效果
     * @param effect 效果类型
     */
    void setPrimaryEffect(const EffectType* effect);

    /**
     * @brief 获取辅助效果
     * @return 辅助效果类型
     */
    [[nodiscard]] const EffectType* getSecondaryEffect() const
    {
        return m_secondaryEffect.has_value() ? &m_secondaryEffect.value() : nullptr;
    }

    /**
     * @brief 设置辅助效果
     * @param effect 效果类型
     */
    void setSecondaryEffect(const EffectType* effect);

    /**
     * @brief 获取支付物品（信标槽位中的物品）
     * @return 支付物品
     */
    [[nodiscard]] const ItemStack& getPaymentItem() const { return m_paymentItem; }

    /**
     * @brief 设置支付物品
     * @param stack 物品
     */
    void setPaymentItem(const ItemStack& stack);

    /**
     * @brief 检查是否可以使用该效果
     * @param effect 效果类型
     * @return 如果可以使用返回true
     */
    [[nodiscard]] bool canUseEffect(const EffectType* effect) const;

    /**
     * @brief 计算效果范围
     * @return 效果半径
     */
    [[nodiscard]] i32 getEffectRange() const;

    // ========== 光束渲染接口 ==========

    /**
     * @brief 获取光束段列表（客户端渲染用）
     * @return 光束段列表的常量引用
     */
    [[nodiscard]] const std::vector<BeaconBeamSegment>& getBeamSegments() const { return m_beamSegments; }

    /**
     * @brief 检查光束是否激活
     * @return 如果有光束段返回true
     */
    [[nodiscard]] bool hasBeam() const { return !m_beamSegments.empty(); }

    /**
     * @brief 获取最大渲染距离平方（客户端渲染用）
     * @return 渲染距离平方（256 格 = 16 个区块）
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const { return 65536.0; }

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 检查并更新金字塔等级
     * @param world 世界引用
     */
    void _updateLevels(IWorld& world);

    /**
     * @brief 检查是否可以看到天空
     * @param world 世界引用
     * @return 如果可以看到天空返回true
     */
    [[nodiscard]] bool _canSeeSky(IWorld& world) const;

    /**
     * @brief 应用效果给附近玩家
     * @param world 世界引用
     */
    void _applyEffects(IWorld& world);

    /**
     * @brief 检查矿物是否有效
     * @param itemId 物品ID
     * @return 如果是有效的支付物品返回true
     */
    [[nodiscard]] static bool _isValidPayment(u32 itemId);

    /**
     * @brief 更新光束段（客户端 tick 中调用）
     * @param world 世界引用
     */
    void _updateBeamSegments(IWorld& world);

    /**
     * @brief 获取光束颜色叠加后的颜色
     * @param current 当前颜色
     * @param newColor 新颜色
     * @return 叠加后的颜色
     */
    [[nodiscard]] static std::array<f32, 3> _blendColors(
        const std::array<f32, 3>& current, const std::array<f32, 3>& newColor);

    i32 m_level = 0;                               ///< 金字塔等级 (0-4)
    i32 m_tickCount = 0;                           ///< tick计数器
    std::optional<EffectType> m_primaryEffect;     ///< 主效果
    std::optional<EffectType> m_secondaryEffect;   ///< 辅助效果
    ItemStack m_paymentItem;                       ///< 支付物品槽位
    bool m_lastBeamState = false;                  ///< 上一帧光束状态
    std::vector<BeaconBeamSegment> m_beamSegments; ///< 光束段列表（客户端渲染用）

    /// 等级对应的有效效果
    static const std::array<std::vector<const EffectType*>, 4> VALID_EFFECTS;
};

} // namespace blockentity
} // namespace mc
