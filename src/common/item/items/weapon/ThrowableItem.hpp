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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EXPRESS OR
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR USE OF OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/util/math/Vector3.hpp"
#include <functional>
#include <memory>

namespace mc {

class Player;
class World;
class ItemStack;

namespace entity {
class ProjectileItemEntity;
class ThrowableEntity;
} // namespace entity

namespace item {

/**
 * @brief 投掷物品基类
 *
 * 用于雪球、鸡蛋、末影珍珠等可投掷物品。
 * 同时实现 ProjectileItem 接口，提供 asProjectile() 方法
 * 供发射器和不祥物品生成器等通用代码创建弹射物。
 *
 * 投掷机制:
 * - 右键投掷
 * - 飞行轨迹受重力影响
 * - 命中后触发特定效果
 */
class ThrowableItem : public Item, public ProjectileItem {
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
     * 返回 0（即时使用）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 投掷物品。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

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

    // ========== ProjectileItem 接口实现 ==========

    /**
     * @brief 创建弹射物实体（ProjectileItem 接口）
     *
     * 调用 createProjectileEntity() 创建弹射物，设置位置，但不添加到世界。
     * 调用方负责将实体添加到世界、设置发射者和调用 shoot()。
     *
     * 默认发射器配置为 ProjectileDispenseConfig::defaults()（power=1.1, uncertainty=6.0），
     * ThrowablePotionItem 和 ExperienceBottleItem 子类覆写 getDispenseConfig() 返回不同配置。
     */
    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> asProjectile(IWorld& world,
        const Vector3& position,
        const ItemStack& stack,
        f32 directionX,
        f32 directionY,
        f32 directionZ) const override;

    /**
     * @brief 获取发射器配置
     *
     * 默认返回 ProjectileDispenseConfig::defaults()（power=1.1, uncertainty=6.0）。
     * ThrowablePotionItem 和 ExperienceBottleItem 覆写返回 potion() 配置。
     */
    [[nodiscard]] ProjectileDispenseConfig getDispenseConfig() const override
    {
        return ProjectileDispenseConfig::defaults();
    }

protected:
    /**
     * @brief 创建投掷实体（玩家投掷场景）
     *
     * 创建弹射物实体，设置位置和发射者，并添加到世界。
     * 此方法仅供 onItemRightClick() 内部使用。
     *
     * @param world 世界
     * @param player 投掷者
     * @param stack 物品堆
     * @return 投掷实体（调用者不拥有所有权，实体已添加到世界）
     */
    [[nodiscard]] virtual entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const = 0;

    /**
     * @brief 创建弹射物实体（通用场景）
     *
     * 创建弹射物实体但不设置位置、发射者，也不添加到世界。
     * 供 asProjectile() 和 createProjectile() 使用。
     * 子类必须实现此方法，返回对应类型的弹射物实体。
     *
     * @param world 世界引用
     * @param stack 物品堆（可能包含 NBT 数据，如药水效果）
     * @return 创建的弹射物实体（调用者拥有所有权）
     */
    [[nodiscard]] virtual std::unique_ptr<entity::ProjectileEntity> createProjectileEntity(
        IWorld& world, const ItemStack& stack) const = 0;

    /**
     * @brief 播放投掷音效
     * @param player 玩家
     */
    virtual void playThrowSound(Player& player) const;
};

} // namespace item
} // namespace mc
