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

#include "../../Goal.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

class MobEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 门交互目标基类
 *
 * 提供门的检测和位置追踪功能，是 BreakDoorGoal 和 OpenDoorGoal 的公共基类。
 * 当生物水平碰撞并沿路径遇到木门时激活。
 *
 * 检测逻辑：
 * 1. 检查生物是否水平碰撞
 * 2. 沿路径节点查找木门（当前节点和后续2个节点）
 * 3. 如果路径上没找到，检查生物正上方的方块
 *
 * 通过逻辑：
 * 通过计算生物相对于门中心的方向向量变化来检测是否已穿过门。
 * 当方向向量的点积从正变负时，判定为已穿过。
 */
class DoorInteractGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     */
    explicit DoorInteractGoal(MobEntity* mob);

    ~DoorInteractGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void tick() override;

protected:
    /**
     * @brief 检查门是否处于打开状态
     * 如果门方块不存在或不再是门，将 hasDoor 设为 false
     */
    [[nodiscard]] bool _isDoorOpen();

    /**
     * @brief 设置门的打开/关闭状态
     * @param open true 为打开，false 为关闭
     */
    void _setDoorOpen(bool open);

    MobEntity* m_mob;
    IWorld* m_world = nullptr;

    /// 门的位置
    BlockPos m_doorPos{0, 0, 0};

    /// 是否找到了有效的门
    bool m_hasDoor = false;

    /// 生物是否已穿过门
    bool m_hasPassedDoor = false;

    /// 开始时，生物到门中心方向向量的X分量
    f32 m_doorOpenDirX = 0.0f;

    /// 开始时，生物到门中心方向向量的Z分量
    f32 m_doorOpenDirZ = 0.0f;
};

} // namespace entity::ai::goal
} // namespace mc
