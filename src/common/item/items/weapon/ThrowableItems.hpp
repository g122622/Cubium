#pragma once

#include "ThrowableItem.hpp"

namespace mc {
namespace item {

/**
 * @brief 雪球物品
 *
 * 雪球可以对烈焰人造成3点伤害，对其他实体无伤害。
 *
 * 参考 MC 1.16.5 SnowballItem
 */
class SnowballItem : public ThrowableItem {
public:
    explicit SnowballItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;
};

/**
 * @brief 鸡蛋物品
 *
 * 鸡蛋投掷后有12.5%概率孵化小鸡。
 *
 * 参考 MC 1.16.5 EggItem
 */
class EggItem : public ThrowableItem {
public:
    explicit EggItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;
};

/**
 * @brief 末影珍珠物品
 *
 * 末影珍珠投掷后会将玩家传送至落点，并造成5点伤害。
 *
 * 参考 MC 1.16.5 EnderPearlItem
 */
class EnderPearlItem : public ThrowableItem {
public:
    explicit EnderPearlItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;
};

/**
 * @brief 附魔之瓶物品
 *
 * 投掷后破裂并释放3-11点经验值。
 *
 * 参考 MC 1.16.5 ExperienceBottleItem
 */
class ExperienceBottleItem : public ThrowableItem {
public:
    explicit ExperienceBottleItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;
};

} // namespace item
} // namespace mc
