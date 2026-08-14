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

#include "AmbientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

/**
 * @brief 蝙蝠实体
 *
 * 生活在洞穴中的飞行生物。
 *
 * 特性：
 * - 飞行：在空中飞行
 * - 倒挂：白天会倒挂在方块下
 * - 休息：休息时不发出声音
 * - 睡眠：白天睡眠，夜间活动
 *
 * AI 目标：
 * - BatRandomFlyGoal: 随机飞行目标（优先级0）
 * - BatRestGoal: 挂墙休息目标（优先级1）
 */
class BatEntity : public AmbientEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param registry 实体注册表（ECS）
     */
    BatEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~BatEntity() override = default;

    // 禁止拷贝
    BatEntity(const BatEntity&) = delete;
    BatEntity& operator=(const BatEntity&) = delete;

    // 允许移动
    BatEntity(BatEntity&&) = delete;
    BatEntity& operator=(BatEntity&&) = delete;

    /**
     * @brief 创建蝙蝠实体
     * @param world 世界实例
     * @return 新的蝙蝠实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 飞行状态 ==========

    /**
     * @brief 是否正在飞行
     */
    [[nodiscard]] bool isFlying() const { return m_flying; }

    /**
     * @brief 设置飞行状态
     */
    void setFlying(bool flying) { m_flying = flying; }

    // ========== 休息状态 ==========

    /**
     * @brief 是否正在休息（倒挂）
     */
    [[nodiscard]] bool isResting() const { return m_resting; }

    /**
     * @brief 设置休息状态
     */
    void setResting(bool resting) { m_resting = resting; }

    /**
     * @brief 是否可以休息
     * 检查上方是否有方块可以倒挂
     */
    [[nodiscard]] bool canRest() const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

    /**
     * @brief 蝙蝠不触发压力板和绊线
     * @return true 蝙蝠不触发压力板
     */
    [[nodiscard]] bool doesEntityNotTriggerPressurePlate() const override { return true; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 飞行状态
    bool m_flying = true;
    bool m_resting = false;

    // 休息位置
    BlockPos m_restPos;
    f32 m_restAngle = 0.0f;

    // 飞行计时器
    i32 m_flyTimer = 0;

    // 常量
    static constexpr f32 FLY_SPEED = 0.1f;
};

} // namespace mc
