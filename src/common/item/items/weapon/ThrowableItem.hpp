#pragma once

#include "../../core/Item.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../core/Types.hpp"
#include <functional>

namespace mc {

// 前向声明
class Player;
class World;
class ItemStack;

namespace entity {
    class ProjectileItemEntity;
    class ThrowableEntity;
}

namespace item {

/**
 * @brief 投掷物品基类
 *
 * 用于雪球、鸡蛋、末影珍珠等可投掷物品。
 *
 * 投掷机制:
 * - 右键投掷
 * - 飞行轨迹受重力影响
 * - 命中后触发特定效果
 *
 * 参考 MC 1.16.5 SnowballItem
 */
class ThrowableItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit ThrowableItem(const ItemProperties& properties);

    ~ThrowableItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * MC 1.16.5: 返回 0（即时使用）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 投掷物品。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(
        IWorld& world,
        Player& player,
        Hand hand) override;

    // ========== 投掷物品特有方法 ==========

    /**
     * @brief 获取投掷速度
     * @return 投掷速度因子
     */
    [[nodiscard]] virtual f32 getThrowVelocity() const { return 1.5f; }

    /**
     * @brief 获取投掷偏移
     * @return 投掷偏移因子
     */
    [[nodiscard]] virtual f32 getThrowInaccuracy() const { return 0.0f; }

protected:
    /**
     * @brief 创建投掷实体
     * @param world 世界
     * @param player 投掷者
     * @param stack 物品堆
     * @return 投掷实体（调用者负责释放）
     */
    [[nodiscard]] virtual entity::ProjectileItemEntity* createProjectile(
        IWorld& world,
        Player& player,
        const ItemStack& stack) const = 0;

    /**
     * @brief 播放投掷音效
     * @param player 玩家
     */
    virtual void playThrowSound(Player& player) const;
};

} // namespace item
} // namespace mc
