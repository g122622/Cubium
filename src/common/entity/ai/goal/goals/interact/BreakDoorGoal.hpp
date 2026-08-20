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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "DoorInteractGoal.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <string>

namespace mc {

class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 破门目标
 *
 * 使亡灵生物（如僵尸、卫道士）破坏木门的AI目标。
 * 只有在允许的难度下且 mobGriefing 游戏规则开启时才会触发。
 * 破门过程中会显示方块破坏动画，并在完成后移除门方块。
 *
 * MC 1.21.11 对齐：BreakDoorGoal extends DoorInteractGoal
 * - 默认破门时间 240 ticks（12秒），getDoorBreakTime 取 max(240, 自定义值)
 * - 破门难度门控由调用方传入谓词决定：僵尸仅 Hard（Zombie.java:88
 *   DOOR_BREAKING_PREDICATE），卫道士 Normal+Hard（Vindicator.java:52）
 * - 破门过程中播放攻击音效和挥臂动画
 * - 破坏进度通过 BlockBreakAnimPacket 同步到客户端
 * - 破坏后不移除上半部分（由 DoorBlock::updatePostPlacement 自动处理）
 */
class BreakDoorGoal : public DoorInteractGoal {
public:
    /**
     * @brief 难度判断谓词类型
     * 接受 Difficulty 枚举，返回是否允许在该难度下破门
     */
    using DifficultyPredicate = std::function<bool(Difficulty)>;

    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param validDifficulties 允许破门的难度判断谓词
     */
    BreakDoorGoal(MobEntity* mob, DifficultyPredicate validDifficulties);

    /**
     * @brief 构造函数（自定义破门时间）
     * @param mob 拥有此目标的生物
     * @param doorBreakTime 自定义破门时间（ticks），必须 >= 240
     * @param validDifficulties 允许破门的难度判断谓词
     */
    BreakDoorGoal(MobEntity* mob, i32 doorBreakTime, DifficultyPredicate validDifficulties);

    ~BreakDoorGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BreakDoorGoal"; }

    /**
     * @brief 获取破门所需时间（ticks）
     */
    [[nodiscard]] i32 getDoorBreakTime() const;

private:
    /**
     * @brief 检查当前难度是否允许破门
     */
    [[nodiscard]] bool _isValidDifficulty() const;

    /// 默认破门时间（240 ticks = 12秒）
    static constexpr i32 DEFAULT_DOOR_BREAK_TIME = 240;

    /// 允许破门的难度判断谓词
    DifficultyPredicate m_validDifficulties;

    /// 自定义破门时间，-1 表示使用默认值
    i32 m_customDoorBreakTime = -1;

    /// 已破坏时间（ticks）
    i32 m_breakTime = 0;

    /// 上一次发送的破坏阶段（0-9），-1 表示未发送
    i32 m_lastBreakProgress = -1;
};

/**
 * @brief 创建标准的灾厄村民（卫道士）破门难度谓词（Normal 和 Hard 难度允许破门）。
 *
 * 对齐 MC Java 1.21.11 Vindicator.java:52 DOOR_BREAKING_PREDICATE（Normal || Hard）。
 * 卫道士在袭击中破门，Normal 与 Hard 均可。
 */
inline BreakDoorGoal::DifficultyPredicate defaultDoorBreakDifficultyPredicate()
{
    return [](Difficulty difficulty) { return difficulty == Difficulty::Normal || difficulty == Difficulty::Hard; };
}

/**
 * @brief 创建标准的僵尸破门难度谓词（仅 Hard 难度允许破门）。
 *
 * 对齐 MC Java 1.21.11 Zombie.java:88 DOOR_BREAKING_PREDICATE（仅 Hard）。
 * 僵尸仅在困难难度下破门（Normal 难度僵尸只开门不破门）。
 */
inline BreakDoorGoal::DifficultyPredicate zombieDoorBreakDifficultyPredicate()
{
    return [](Difficulty difficulty) { return difficulty == Difficulty::Hard; };
}

} // namespace entity::ai::goal
} // namespace mc
