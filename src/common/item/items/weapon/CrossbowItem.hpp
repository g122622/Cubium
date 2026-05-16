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
#include "../../../entity/inventory/PlayerInventory.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/Item.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/UseAction.hpp"
#include <functional>
#include <vector>

namespace mc {

// 前向声明
class Player;
class World;
class LivingEntity;

namespace entity {
class AbstractArrowEntity;
}

namespace item {

/**
 * @brief 弩物品
 *
 * 弩是可以预先装填箭矢的远程武器。
 *
 * 装填机制:
 * - 基础装填时间: 25 tick (1.25秒)
 * - 快速装填附魔: 每级减少 5 tick
 * - 装填过程中播放音效 (20% 和 50%)
 *
 * 发射机制:
 * - 箭矢速度: 3.15 (烟花 1.6)
 * - 支持多重射击: 发射 3 支箭矢
 * - 支持穿透: 箭矢可穿透实体
 * - 支持烟花火箭: 作为弹药
 *
 * 附魔支持:
 * - 多重射击 (Multishot): 同时发射 3 支箭矢
 * - 穿透 (Piercing): 箭矢可穿透实体
 * - 快速装填 (Quick Charge): 减少装填时间
 *
 * 参考 MC 1.16.5 CrossbowItem
 */
class CrossbowItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit CrossbowItem(const ItemProperties& properties);

    ~CrossbowItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * MC 1.16.5: 装填时间 + 3 tick
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Crossbow
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 如果已装填则发射，否则开始装填。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 停止使用物品（松开右键）
     *
     * 完成装填。
     */
    void onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft) override;

    // ========== 弩特有方法 ==========

    /**
     * @brief 检查弩是否已装填
     */
    [[nodiscard]] static bool isCharged(const ItemStack& stack);

    /**
     * @brief 设置弩的装填状态
     */
    static void setCharged(ItemStack& stack, bool charged);

    /**
     * @brief 获取装填时间
     *
     * 基础 25 tick，快速装填每级减少 5 tick。
     */
    [[nodiscard]] static i32 getChargeTime(const ItemStack& stack);

    /**
     * @brief 获取弹药预测谓词
     *
     * 接受箭矢和烟花火箭。
     */
    [[nodiscard]] std::function<bool(const ItemStack&)> getAmmoPredicate() const;

    /**
     * @brief 获取背包弹药预测谓词
     *
     * 只接受箭矢。
     */
    [[nodiscard]] std::function<bool(const ItemStack&)> getInventoryAmmoPredicate() const;

    /**
     * @brief 检查弩中是否装填了指定物品
     * @param stack 弩物品堆
     * @param item 要检查的物品类型
     * @return 如果装填了指定物品返回true
     */
    [[nodiscard]] static bool hasChargedProjectile(const ItemStack& stack, const Item* item);

    /**
     * @brief 清除已装填的弹丸
     * @param stack 弩物品堆
     */
    static void clearProjectiles(ItemStack& stack);

private:
    /**
     * @brief 检查物品是否是弩的弹药
     *
     * 接受箭矢和烟花火箭。
     */
    [[nodiscard]] static bool isAmmo(const ItemStack& stack);

    /**
     * @brief 查找玩家身上的弹药
     * @return 弹药物品堆，如果没有则返回空
     */
    static ItemStack findAmmo(Player& player);

    /**
     * @brief 检查是否有足够的弹药并装填
     */
    static bool loadProjectiles(Player& player, ItemStack& crossbow);

    /**
     * @brief 发射弹丸
     */
    static void fireProjectiles(
        IWorld& world, LivingEntity& shooter, ItemStack& crossbow, f32 velocity, f32 inaccuracy);

    /**
     * @brief 获取已装填的弹丸列表
     */
    static std::vector<ItemStack> getChargedProjectiles(const ItemStack& stack);

    /**
     * @brief 添加弹丸到弩
     */
    static void addChargedProjectile(ItemStack& crossbow, const ItemStack& projectile);

    /**
     * @brief 获取多重射击等级
     */
    static i32 getMultishotLevel(const ItemStack& stack);

    /**
     * @brief 获取穿透等级
     */
    static i32 getPiercingLevel(const ItemStack& stack);
};

} // namespace item
} // namespace mc
