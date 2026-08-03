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

#include "common/core/Types.hpp"
#include "entity/ai/goal/Goal.hpp"
#include <functional>
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;
class Player;
class ItemStack;

namespace entity::ai::goal {

/**
 * @brief 食物诱惑目标
 *
 * 当玩家手持特定物品时，动物会被诱惑跟随玩家。
 */
class TemptGoal : public Goal {
public:
    /**
     * @brief 物品检查函数类型
     */
    using ItemPredicate = std::function<bool(const ItemStack&)>;

    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     * @param itemPredicate 物品检查函数
     * @param scaredByMovement 是否被玩家移动吓跑
     */
    TemptGoal(CreatureEntity* creature, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement = false);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 检查是否正在执行
     */
    [[nodiscard]] bool isRunning() const { return m_isRunning; }

    [[nodiscard]] std::string getTypeName() const override { return "TemptGoal"; }

protected:
    /**
     * @brief 检查玩家手持物品是否为诱惑物品
     * @param stack 物品堆
     * @return 是否为诱惑物品
     */
    [[nodiscard]] bool isTempting(const ItemStack& stack) const;

    /**
     * @brief 检查是否被玩家移动吓跑
     * 子类可以重写此方法实现自定义行为（如豹猫信任后不再害怕）
     */
    [[nodiscard]] virtual bool isScaredByPlayerMovement() const;

    /**
     * @brief 寻找附近手持诱惑物品的玩家
     * @return 玩家实体，如果没有则返回 nullptr
     */
    Player* findTemptingPlayer();

    CreatureEntity* m_creature;
    f64 m_speed;
    ItemPredicate m_itemPredicate;
    bool m_scaredByMovement;
    Player* m_temptingPlayer = nullptr;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    f64 m_prevPitch = 0.0;
    f64 m_prevYaw = 0.0;
    i32 m_delayTemptCounter = 0;
    bool m_isRunning = false;
};

} // namespace entity::ai::goal
} // namespace mc
