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

#include "../../../core/Types.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/Item.hpp"
#include "../../core/UseAction.hpp"

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
 * MC 1.16.5 对齐:
 * - 不重写 getUseDuration()（默认返回 0，即时使用）
 * - 不重写 getUseAction()（默认返回 NONE，无使用动画）
 * - 附魔能力为 1
 *
 * 参考 MC 1.16.5 FishingRodItem
 */
class FishingRodItem : public Item {
public:
    explicit FishingRodItem(const ItemProperties& properties);

    ~FishingRodItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值（MC 1.16.5: 1）
     */
    [[nodiscard]] i32 getItemEnchantability() const override;

    /**
     * @brief 右键使用物品
     *
     * 抛出或收回浮标。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

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
