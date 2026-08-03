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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING ANY PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc {

class IWorld;

namespace entity {
class SmallFireballEntity;
}

namespace item {

/**
 * @brief 火焰弹物品
 *
 * 可由玩家右键使用或由发射器发射的火焰弹。
 * 发射时创建 SmallFireballEntity 实体。
 * 右键使用时可点燃方块（营火、蜡烛等）或在合适位置放置火焰。
 *
 * 实现 ProjectileItem 接口以支持发射器自动注册弹射物发射行为。
 * 与 WindChargeItem 类似，火焰弹在 asProjectile() 中直接设置加速度，
 * shoot() 为空操作。
 */
class FireChargeItem : public Item, public ProjectileItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit FireChargeItem(const ItemProperties& properties);

    ~FireChargeItem() override = default;

    // 禁止拷贝和移动
    FireChargeItem(const FireChargeItem&) = delete;
    FireChargeItem& operator=(const FireChargeItem&) = delete;
    FireChargeItem(FireChargeItem&&) = delete;
    FireChargeItem& operator=(FireChargeItem&&) = delete;

    /**
     * @brief 在方块上使用物品
     *
     * 功能：
     * 1. 如果点击的方块可以点燃（如未点燃的营火、蜡烛、蜡烛蛋糕），点燃它
     * 2. 否则，在点击面的相邻位置放置火焰（普通火或灵魂火）
     *
     * 与打火石不同，火焰弹消耗物品数量（shrink(1)）而非耐久度。
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    // ============================================================================
    // ProjectileItem 接口实现
    // ============================================================================

    /**
     * @brief 创建弹射物实体
     *
     * 创建 SmallFireballEntity 并设置加速度。
     * 火焰弹在创建时即设置加速度（方向 * 力度），
     * 因此 shoot() 为空操作。
     */
    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> asProjectile(IWorld& world,
        const Vector3& position,
        const ItemStack& stack,
        f32 directionX,
        f32 directionY,
        f32 directionZ) const override;

    /**
     * @brief 获取发射器配置
     * @return 火焰弹配置（power=1.0, uncertainty=6.0）
     */
    [[nodiscard]] ProjectileDispenseConfig getDispenseConfig() const override;

    /**
     * @brief 发射弹射物（空操作）
     *
     * 火焰弹在 asProjectile() 中已设置加速度，
     * 此处不需要再调用 shoot()。
     */
    void shoot(entity::ProjectileEntity& projectile,
        f32 directionX,
        f32 directionY,
        f32 directionZ,
        f32 power,
        f32 uncertainty) const override;

private:
    /**
     * @brief 播放火焰弹使用音效
     * @param world 世界引用
     * @param pos 音效位置
     */
    static void playUseSound(IWorld& world, const BlockPos& pos);
};

} // namespace item
} // namespace mc
