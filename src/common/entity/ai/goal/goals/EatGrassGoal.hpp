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

#include "../Goal.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <string>

namespace mc {

// Forward declarations
class MobEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 吃草目标
 *
 * 使动物（如羊）吃草方块的AI目标。
 * 当动物站在草方块或草上时触发，吃草后方块变为泥土或空气，
 * 并调用吃草回调函数。
 *
 * 吃草逻辑：
 * 1. 概率触发（幼年 1/50，成年 1/1000）
 * 2. 检测脚下方块是否为草或草方块
 * 3. 动画计时器 40 ticks
 * 4. 第 4 tick 时执行吃草动作
 */
class EatGrassGoal : public Goal {
public:
    /**
     * @brief 吃草回调函数类型
     * 当吃草成功时调用，用于通知实体更新状态
     */
    using EatGrassCallback = std::function<void()>;

    /**
     * @brief 检查是否为幼年实体的回调类型
     */
    using IsChildCallback = std::function<bool()>;

    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param onEatGrass 吃草时的回调函数
     * @param isChild 检查是否为幼年的回调函数
     */
    EatGrassGoal(MobEntity* mob, EatGrassCallback onEatGrass, IsChildCallback isChild);

    ~EatGrassGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EatGrassGoal"; }

    /**
     * @brief 获取吃草动画计时器
     * @return 当前计时器值
     */
    [[nodiscard]] i32 getEatingGrassTimer() const { return m_eatingGrassTimer; }

private:
    /**
     * @brief 检查位置是否为可吃的草
     * @param world 世界
     * @param pos 位置
     * @return 如果是草方块或草返回true
     */
    [[nodiscard]] bool _isGrassAt(IWorld* world, const BlockPos& pos) const;

    /**
     * @brief 执行吃草动作
     * 将草方块变成泥土或破坏草
     */
    void _eatGrass();

    MobEntity* m_mob;
    IWorld* m_world = nullptr;
    EatGrassCallback m_onEatGrass;
    IsChildCallback m_isChild;
    i32 m_eatingGrassTimer = 0;
    BlockPos m_targetPos{0, 0, 0};
    bool m_isEatingGrassBlock = false; // true = 草方块，false = 草

    // 动画持续时间
    static constexpr i32 EAT_DURATION = 40;
    // 执行吃草的时机
    static constexpr i32 EAT_TICK = 4;
    // 幼年动物触发概率倒数
    static constexpr i32 CHILD_CHANCE = 50;
    // 成年动物触发概率倒数
    static constexpr i32 ADULT_CHANCE = 1000;
};

} // namespace entity::ai::goal
} // namespace mc
