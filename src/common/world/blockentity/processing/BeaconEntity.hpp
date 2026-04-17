#pragma once

#include "world/blockentity/BlockEntity.hpp"
#include "entity/effect/EffectType.hpp"
#include "item/core/ItemStack.hpp"
#include <array>
#include <memory>
#include <vector>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 信标方块实体
 *
 * 信标是一种提供状态效果的方块，特点：
 * - 金字塔结构（1-4层）决定效果等级
 * - 激活需要金字塔基座和天空光
 * - 提供主效果（速度/急迫/抗性/跳跃/力量）
 * - 提供辅助效果（生命恢复）
 * - 消耗矿物作为燃料
 *
 * 参考: net.minecraft.tileentity.BeaconTileEntity
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
    [[nodiscard]] const EffectType* getPrimaryEffect() const { return m_primaryEffect.has_value() ? &m_primaryEffect.value() : nullptr; }

    /**
     * @brief 设置主效果
     * @param effect 效果类型
     */
    void setPrimaryEffect(const EffectType* effect);

    /**
     * @brief 获取辅助效果
     * @return 辅助效果类型
     */
    [[nodiscard]] const EffectType* getSecondaryEffect() const { return m_secondaryEffect.has_value() ? &m_secondaryEffect.value() : nullptr; }

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

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 检查并更新金字塔等级
     * @param world 世界引用
     */
    void updateLevels(IWorld& world);

    /**
     * @brief 检查是否可以看到天空
     * @param world 世界引用
     * @return 如果可以看到天空返回true
     */
    [[nodiscard]] bool canSeeSky(IWorld& world) const;

    /**
     * @brief 应用效果给附近玩家
     * @param world 世界引用
     */
    void applyEffects(IWorld& world);

    /**
     * @brief 检查矿物是否有效
     * @param itemId 物品ID
     * @return 如果是有效的支付物品返回true
     */
    [[nodiscard]] static bool isValidPayment(u32 itemId);

    i32 m_level = 0;                            ///< 金字塔等级 (0-4)
    i32 m_tickCount = 0;                        ///< tick计数器
    Optional<EffectType> m_primaryEffect;   ///< 主效果
    Optional<EffectType> m_secondaryEffect; ///< 辅助效果
    ItemStack m_paymentItem;                    ///< 支付物品槽位
    bool m_lastBeamState = false;               ///< 上一帧光束状态

    /// 等级对应的有效效果
    static const std::array<std::vector<const EffectType*>, 4> VALID_EFFECTS;
};

} // namespace blockentity
} // namespace mc




