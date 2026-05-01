#pragma once

#include "../core/Item.hpp"
#include "../core/UseAction.hpp"
#include "../../../entity/core/Types.hpp"
#include <functional>

namespace mc {

// 前向声明
class Player;
class World;
class ItemStack;
class AbstractArrowEntity;

namespace item {

/**
 * @brief 弓物品
 *
 * 弓是可蓄力的远程武器，蓄力时间影响箭矢速度和伤害。
 *
 * 蓄力机制:
 * - 最小发射时间: 约 3 tick (速度 >= 0.1)
 * - 满蓄力时间: 20 tick (1 秒)
 * - 最大速度: 3.0 (满蓄力时)
 *
 * 附魔支持:
 * - 力量 (Power): 每级 +0.5 伤害
 * - 冲击 (Punch): 每级 +1 击退等级
 * - 火矢 (Flame): 箭矢点燃目标 5 秒
 * - 无限 (Infinity): 不消耗普通箭矢
 *
 * 参考 MC 1.16.5 BowItem
 */
class BowItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit BowItem(const ItemProperties& properties);

    ~BowItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * MC 1.16.5: 返回 72000 tick（几乎无限制）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Bow
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 检查玩家是否有箭矢或无限附魔，开始蓄力。
     */
    [[nodiscard]] ActionResult onItemRightClick(
        IWorld& world,
        Player& player,
        Hand hand) override;

    /**
     * @brief 停止使用物品（松开右键）
     *
     * 发射箭矢。计算蓄力时间和速度。
     */
    void onPlayerStoppedUse(
        ItemStack& stack,
        IWorld& world,
        LivingEntity& entity,
        i32 timeLeft) override;

    /**
     * @brief 获取箭矢预测谓词
     *
     * 用于检测物品是否为有效箭矢。
     * 默认检查 ItemTags::ARROWS。
     */
    [[nodiscard]] virtual std::function<bool(const ItemStack&)> getAmmoPredicate() const;

    /**
     * @brief 获取背包箭矢预测谓词
     *
     * 用于检测背包中的箭矢。
     * 默认与 getAmmoPredicate 相同。
     */
    [[nodiscard]] virtual std::function<bool(const ItemStack&)> getInventoryAmmoPredicate() const;

    // ========== 弓特有方法 ==========

    /**
     * @brief 计算箭矢速度因子
     *
     * MC 1.16.5 公式: f = charge / 20.0
     *                 f = (f * f + f * 2.0) / 3.0
     *                 最大 1.0
     *
     * @param chargeTicks 蓄力 tick 数
     * @return 速度因子 (0.0 - 1.0)
     */
    [[nodiscard]] static f32 getArrowVelocity(i32 chargeTicks);

    /**
     * @brief 自定义箭矢
     *
     * 子类可重写以修改箭矢属性。
     * 默认返回原箭矢。
     */
    [[nodiscard]] virtual AbstractArrowEntity* customArrow(AbstractArrowEntity* arrow);

private:
    /**
     * @brief 查找玩家的箭矢
     * @param player 玩家
     * @param bowStack 弓物品堆
     * @return 箭矢物品堆（如果没有则返回空）
     */
    [[nodiscard]] ItemStack findAmmo(Player& player, const ItemStack& bowStack) const;

    /**
     * @brief 检查箭矢是否无限
     * @param arrowStack 箭矢物品堆
     * @param bowStack 弓物品堆
     * @param player 玩家
     * @return 是否无限（不消耗）
     */
    [[nodiscard]] bool isInfiniteArrow(const ItemStack& arrowStack,
                                        const ItemStack& bowStack,
                                        Player& player) const;
};

} // namespace item
} // namespace mc
