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

#include "../../core/Item.hpp"
#include "../../food/Food.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/UseAction.hpp"

namespace mc {

// Forward declarations
class Player;
class LivingEntity;
class ZombieVillagerEntity;

namespace item::items {

/**
 * @brief 金苹果物品
 *
 * 金苹果是特殊的食物，具有以下功能：
 * 1. 食用：恢复饥饿值并给予药水效果
 * 2. 对僵尸村民使用：如果僵尸村民有虚弱效果，开始治愈过程
 */
class GoldenAppleItem : public Item {
public:
    /**
     * @brief 构造金苹果物品
     * @param food 食物属性
     * @param properties 物品属性
     */
    GoldenAppleItem(const food::Food* food, ItemProperties properties);

    ~GoldenAppleItem() override = default;

    /**
     * @brief 是否为食物
     */
    [[nodiscard]] bool isFood() const override { return m_food != nullptr; }

    /**
     * @brief 获取食物属性
     */
    [[nodiscard]] const food::Food* getFood() const override { return m_food; }

    /**
     * @brief 获取使用时长
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 使用完成（食用完成）
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 与实体交互
     *
     * 对僵尸村民使用金苹果：
     * 1. 检查目标是否为僵尸村民
     * 2. 检查僵尸村民是否有虚弱效果
     * 3. 如果满足条件，开始治愈过程
     * 4. 消耗一个金苹果（非创造模式）
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;

    /**
     * @brief 是否可以食用
     *
     * 金苹果可以在任何时候食用（不要求饥饿）
     */
    [[nodiscard]] bool canEat(const ItemStack& stack, const Player& player) const override;

private:
    const food::Food* m_food;
};

} // namespace item::items
} // namespace mc
