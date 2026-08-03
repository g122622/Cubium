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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR USE OF
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "ThrowableItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include <memory>

namespace mc {
namespace item {

/**
 * @brief 雪球物品
 *
 * 雪球可以对烈焰人造成3点伤害，对其他实体无伤害。
 */
class SnowballItem : public ThrowableItem {
public:
    explicit SnowballItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;

    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> createProjectileEntity(
        IWorld& world, const ItemStack& stack) const override;
};

/**
 * @brief 鸡蛋物品
 *
 * 鸡蛋投掷后有12.5%概率孵化小鸡。
 */
class EggItem : public ThrowableItem {
public:
    explicit EggItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;

    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> createProjectileEntity(
        IWorld& world, const ItemStack& stack) const override;
};

/**
 * @brief 末影珍珠物品
 *
 * 末影珍珠投掷后会将玩家传送至落点，并造成5点伤害。
 */
class EnderPearlItem : public ThrowableItem {
public:
    explicit EnderPearlItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;

    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> createProjectileEntity(
        IWorld& world, const ItemStack& stack) const override;
};

/**
 * @brief 附魔之瓶物品
 *
 * 投掷后破裂并释放3-11点经验值。
 * 发射器配置：散布减半（3.0）、力度增加25%（1.375）。
 */
class ExperienceBottleItem : public ThrowableItem {
public:
    explicit ExperienceBottleItem(const ItemProperties& properties);

    [[nodiscard]] f32 getThrowVelocity() const override { return 1.5f; }

    /**
     * @brief 获取发射器配置（药水/经验瓶专用）
     *
     * 发射器配置：散布减半（3.0）、力度增加 25%（1.375）
     */
    [[nodiscard]] ProjectileDispenseConfig getDispenseConfig() const override
    {
        return ProjectileDispenseConfig::potion();
    }

protected:
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;

    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> createProjectileEntity(
        IWorld& world, const ItemStack& stack) const override;
};

} // namespace item
} // namespace mc
