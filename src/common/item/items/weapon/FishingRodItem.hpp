#pragma once

#include "../../core/Item.hpp"
#include "../../core/UseAction.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../core/Types.hpp"

namespace mc {

// 前向声明
class Player;
class World;
class ItemStack;

namespace entity {
    class FishingBobberEntity;
}

namespace item {

/**
 * @brief 钓鱼竿物品
 *
 * 钓鱼竿用于钓鱼和拉扯实体。
 *
 * 使用机制:
 * - 右键抛出浮标
 * - 再次右键收杆
 * - 浮标在水中等待鱼咬钩
 *
 * 钓鱼机制:
 * - 等待时间：5-45秒随机
 * - 咬钩提示：浮标下沉
 * - 收杆时机：咬钩后及时收杆
 *
 * 附魔支持:
 * - 海之眷顾 (Luck of the Sea): 增加宝藏概率
 * - 饵钓 (Lure): 减少等待时间
 *
 * 参考 MC 1.16.5 FishingRodItem
 */
class FishingRodItem : public Item {
public:
    explicit FishingRodItem(const ItemProperties& properties);

    ~FishingRodItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * MC 1.16.5: 返回 0（即时使用）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Bow (钓鱼竿也用弓的动作)
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 抛出或收回浮标。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(
        IWorld& world,
        Player& player,
        Hand hand) override;

    // ========== 钓鱼竿特有方法 ==========

    /**
     * @brief 检查玩家是否有浮标
     * @param player 玩家
     * @return 是否有浮标
     */
    [[nodiscard]] static bool hasBobber(Player& player);

    /**
     * @brief 获取玩家的浮标
     * @param player 玩家
     * @return 浮标实体（如果没有则返回 nullptr）
     */
    [[nodiscard]] static entity::FishingBobberEntity* getBobber(Player& player);
};

} // namespace item
} // namespace mc
