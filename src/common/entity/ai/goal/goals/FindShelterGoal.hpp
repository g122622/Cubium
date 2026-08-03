/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/util/math/Vector3.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 寻找庇护所目标
 *
 * 控制生物在危险情况下寻找安全庇护所的行为。
 * 与FleeSunGoal不同，此目标在白天或雷暴天气时激活，
 * 并且只在有明确威胁时（如光照条件）寻找阴影位置。
 * 用于僵尸、骷髅等亡灵生物的日间避难行为。
 *
 * TODO: 当前无实体使用此目标。MC原版中FoxEntity使用自定义的FoxFindShelterGoal（继承FleeSunGoal），
 * 而非直接使用此目标。待FoxEntity等实体实现后，评估是否需要接入或由子类覆盖。
 */
class FindShelterGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     */
    FindShelterGoal(CreatureEntity* creature, f64 speed);

    ~FindShelterGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FindShelterGoal"; }

protected:
    /**
     * @brief 检查当前位置是否需要避难
     * @return 如果需要避难返回true
     */
    [[nodiscard]] bool _needsShelter() const;

    /**
     * @brief 寻找庇护位置
     * @return 是否找到有效的庇护位置
     */
    [[nodiscard]] bool _findShelterPosition();

    CreatureEntity* m_creature;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_timeout = 0;

    // 搜索范围常量
    static constexpr i32 SHELTER_XZ_RANGE = 10;  // 水平搜索范围
    static constexpr i32 SHELTER_Y_RANGE = 7;    // 垂直搜索范围
    static constexpr i32 MAX_SHELTER_TIME = 600; // 最大庇护时间（tick）
};

} // namespace entity::ai::goal
} // namespace mc
