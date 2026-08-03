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

#include "../../../../core/Types.hpp"
#include "GuardianEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 远古守卫者实体
 *
 * 海底神殿的Boss级怪物。
 *
 * 特性：
 * - 更强大：比普通守卫者更强
 * - 挖掘疲劳：给予附近的玩家挖掘疲劳
 * - 激光攻击：更强的激光攻击
 * - Boss血条：显示Boss血条
 */
class ElderGuardianEntity : public GuardianEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    ElderGuardianEntity(EntityInstanceId id);

    ~ElderGuardianEntity() override = default;

    // 禁止拷贝
    ElderGuardianEntity(const ElderGuardianEntity&) = delete;
    ElderGuardianEntity& operator=(const ElderGuardianEntity&) = delete;

    // 允许移动
    ElderGuardianEntity(ElderGuardianEntity&&) = delete;
    ElderGuardianEntity& operator=(ElderGuardianEntity&&) = delete;

    /**
     * @brief 创建远古守卫者实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 挖掘疲劳 ==========

    /**
     * @brief 是否应该给予挖掘疲劳
     */
    [[nodiscard]] bool shouldApplyMiningFatigue() const { return true; }

    /**
     * @brief 获取挖掘疲劳范围
     */
    [[nodiscard]] f32 getMiningFatigueRange() const { return MINING_FATIGUE_RANGE; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.0f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerAttributes() override;

private:
    i32 m_fatigueTimer = 0;

    static constexpr f32 MINING_FATIGUE_RANGE = 50.0f; // 50格范围
    static constexpr i32 FATIGUE_INTERVAL = 600;       // 每30秒应用一次
};

} // namespace mc
