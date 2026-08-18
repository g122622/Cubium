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

#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <string>

namespace mc {

// 前向声明
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 红石二极管基类
 *
 * 中继器和比较器的公共基类。
 * 提供方向性信号传输、锁定检测等通用功能。
 *
 * ## 特性
 * - 只能水平放置
 * - 有明确的输入端和输出端
 * - 支持侧面锁定
 * - 支持延迟更新
 *
 * ## 子类需要实现
 * - getDelay(): 返回延迟tick数
 * - shouldBePowered(): 判断是否应该输出信号
 * - calculateOutputSignal(): 计算输出信号强度
 */
class RedstoneDiodeBlock : public Block {
public:
    // ========== 构造函数 ==========

    RedstoneDiodeBlock(const std::string& id, const BlockProperties& behaviour);

    // ========== Block 接口实现 ==========

    // 放置朝向：facing = opposite(玩家水平视线方向)（水平四向，输入端朝玩家、输出端背离玩家）。
    // 朝向仅由玩家 yaw 决定（水平四向 South/West/North/East），不含 pitch。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒 North），
    // 与预期按水平视线决定朝向的行为不一致。重写后修正为按水平视线决定朝向。
    // 中继器（RedstoneRepeaterBlock）与比较器（RedstoneComparatorBlock）继承本类，继承本方法自动获得
    // 正确朝向（两者均不重写该方法，靠基类实现）。
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取强信号强度
     *
     * 二极管输出的是强信号，可以充能方块。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 方向
     * @return i32 强信号强度
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 红石二极管特有方法 ==========

    /**
     * @brief 获取延迟tick数
     *
     * 子类必须实现，返回信号延迟。
     * - 中继器：2-8 ticks
     * - 比较器：2 ticks
     *
     * @param state 当前方块状态
     * @return i32 延迟tick数
     */
    [[nodiscard]] virtual i32 getDelay(const BlockState& state) const = 0;

    /**
     * @brief 判断是否应该输出信号
     *
     * 根据输入信号判断是否应该激活。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果应该输出信号
     */
    [[nodiscard]] virtual bool shouldBePowered(IWorld& world, const BlockPos& pos, const BlockState& state) const = 0;

    /**
     * @brief 计算输出信号强度
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return i32 输出信号强度 0-15
     */
    [[nodiscard]] virtual i32 calculateOutputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查是否被锁定
     *
     * 当侧面有信号输入时，二极管被锁定，
     * 保持当前输出状态不变。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果被锁定
     */
    [[nodiscard]] virtual bool isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 获取输入信号强度
     *
     * 从前方获取红石信号，支持红石线信号检测。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return i32 输入信号强度 0-15
     */
    [[nodiscard]] i32 getInputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 获取侧面信号强度
     *
     * 用于锁定检测。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return i32 侧面信号最大强度
     */
    [[nodiscard]] i32 getPowerOnSides(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 获取朝向
     *
     * @param state 方块状态
     * @return Direction 朝向（输出方向）
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 检查是否已充能
     *
     * @param state 方块状态
     * @return true 如果已充能
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

protected:
    /**
     * @brief 触发状态更新
     *
     * 检查是否需要改变状态，如果需要则调度延迟更新。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void updateState(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 通知邻居方块更新
     *
     * 当二极管放置或移除时，需要通知输入端周围的方块更新。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    void notifyNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查是否朝向另一个二极管
     *
     * 用于确定更新优先级。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果朝向另一个二极管
     */
    [[nodiscard]] bool isFacingTowardsRepeater(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查方块是否是二极管（中继器或比较器）
     *
     * 用于锁定检测，只有其他二极管的侧面信号才能锁定中继器。
     *
     * @param state 方块状态
     * @return true 如果是二极管
     */
    [[nodiscard]] bool isDiode(const BlockState& state) const;

    /// 方块ID（用于日志和调试）
    std::string m_id;
};

} // namespace blocks
} // namespace mc
