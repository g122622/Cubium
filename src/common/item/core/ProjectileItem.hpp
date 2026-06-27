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

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc {

class IWorld;
class ItemStack;
class Entity;

namespace entity {
class ProjectileEntity;
}

namespace item {

/**
 * @brief 弹射物发射配置
 *
 * 对齐 MC 1.21.11 ProjectileItem.DispenseConfig，封装弹射物发射参数。
 * 用于发射器（Dispenser）和不祥物品生成器（OminousItemSpawner）等场景。
 */
struct ProjectileDispenseConfig {
    /// 发射力度（默认 1.1，对齐 MC DEFAULT）
    f32 power = 1.1f;

    /// 发射散布精度（默认 6.0，对齐 MC DEFAULT）
    f32 uncertainty = 6.0f;

    /**
     * @brief 构建默认配置
     */
    static ProjectileDispenseConfig defaults() { return {}; }

    /**
     * @brief 构建药水投掷配置（散布减半、力度增加 25%）
     *
     * 对齐 MC ThrowablePotionItem / ExperienceBottleItem 的 DispenseConfig：
     * uncertainty = DEFAULT * 0.5 = 3.0
     * power = DEFAULT * 1.25 = 1.375
     */
    static ProjectileDispenseConfig potion() { return {.power = 1.375f, .uncertainty = 3.0f}; }

    /**
     * @brief 构建风弹配置
     *
     * 对齐 MC WindChargeItem 的 DispenseConfig：
     * power = 1.0, uncertainty = 6.6666665
     */
    static ProjectileDispenseConfig windCharge() { return {.power = 1.0f, .uncertainty = 6.6666665f}; }
};

/**
 * @brief 弹射物物品接口
 *
 * 对齐 MC 1.21.11 net.minecraft.world.item.ProjectileItem，提供统一的
 * "物品 -> 弹射物" 创建接口。实现此接口的物品可以被发射器、不祥物品生成器等
 * 通用代码通过多态方式创建弹射物，无需硬编码物品到弹射物的映射表。
 *
 * 实现类：
 * - ThrowableItem（雪球、鸡蛋、末影珍珠、附魔之瓶、药水等）
 * - WindChargeItem（风弹）
 * - 未来可扩展：FireChargeItem（火焰弹）等
 *
 * 使用方式：
 * @code
 * const Item* item = itemStack.getItem();
 * if (auto* projectileItem = dynamic_cast<const ProjectileItem*>(item)) {
 *     auto entity = projectileItem->asProjectile(world, position, itemStack, direction);
 *     auto config = projectileItem->getDispenseConfig();
 *     // ... shoot and spawn
 * }
 * @endcode
 */
class ProjectileItem {
public:
    virtual ~ProjectileItem() = default;

    /**
     * @brief 创建弹射物实体
     *
     * 根据给定的世界、位置、物品和方向，构造并返回一个弹射物实体。
     * 实体应已设置好位置和方向，但尚未调用 shoot() 或添加到世界。
     *
     * 对齐 MC ProjectileItem.asProjectile(Level, Position, ItemStack, Direction)。
     *
     * @param world 世界引用
     * @param position 生成位置（x, y, z）
     * @param stack 物品堆（某些弹射物需要从中读取数据，如药水效果）
     * @param directionX 发射方向 X 分量（归一化）
     * @param directionY 发射方向 Y 分量（归一化）
     * @param directionZ 发射方向 Z 分量（归一化）
     * @return 创建的弹射物实体，失败返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<entity::ProjectileEntity> asProjectile(IWorld& world,
        const Vector3& position,
        const ItemStack& stack,
        f32 directionX,
        f32 directionY,
        f32 directionZ) const = 0;

    /**
     * @brief 获取发射器配置
     *
     * 返回此物品的发射器行为参数（力度、散布等）。
     * 子类可覆盖以自定义配置。
     *
     * 对齐 MC ProjectileItem.createDispenseConfig()。
     *
     * @return 发射器配置
     */
    [[nodiscard]] virtual ProjectileDispenseConfig getDispenseConfig() const
    {
        return ProjectileDispenseConfig::defaults();
    }

    /**
     * @brief 发射弹射物
     *
     * 默认实现委托给 ProjectileEntity::shoot()。
     * 某些弹射物（如风弹）在 asProjectile() 中已设置初速度，
     * 需要覆盖此方法为空操作以避免覆盖已设置的速度。
     *
     * 对齐 MC ProjectileItem.shoot(Projectile, double, double, double, float, float)。
     *
     * @param projectile 弹射物实体
     * @param directionX 方向 X 分量
     * @param directionY 方向 Y 分量
     * @param directionZ 方向 Z 分量
     * @param power 力度
     * @param uncertainty 散布
     */
    virtual void shoot(entity::ProjectileEntity& projectile,
        f32 directionX,
        f32 directionY,
        f32 directionZ,
        f32 power,
        f32 uncertainty) const;
};

} // namespace item
} // namespace mc
