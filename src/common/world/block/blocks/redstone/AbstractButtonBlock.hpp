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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"

namespace mc {

// 前向声明
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 抽象按钮方块基类
 *
 * 按钮可以输出短脉冲红石信号。
 *
 * ## 特性
 * - 按压输出信号
 * - 自动复位（不同材质延迟不同）
 * - 可附着在不同面上
 * - 短脉冲输出
 *
 * ## 容易踩的坑
 * - 脉冲持续时间需要精确控制
 * - 附着面变化时需要检测支撑
 * - 木按钮和石按钮延迟不同
 */
class AbstractButtonBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param ticksToStayPressed 按压持续时间（tick）
     * @param arrowSensitive 是否可被箭矢持续触发（木按钮为 true，石质按钮为 false）
     */
    AbstractButtonBlock(const BlockProperties& properties, i32 ticksToStayPressed, bool arrowSensitive);

    // ========== Block 接口实现 ==========

    // 放置朝向与附着面：由玩家点击面决定。点击顶面→ATTACH_FACE=Floor、facing=玩家水平朝向；
    // 点击底面→Ceiling、facing=玩家水平朝向；点击墙面→Wall、facing=点击面（水平四向）。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING
    // 恒 North、ATTACH_FACE 恒 Wall），与预期按点击面决定朝向/附着面的行为不一致。重写后修正。
    // 石按钮（StoneButtonBlock）/木按钮（WoodButtonBlock）继承本类，继承本方法自动获得正确朝向。
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 按钮是小型薄型方块，需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 按钮特有方法 ==========

    /**
     * @brief 检查按钮是否被按下
     *
     * @param state 方块状态
     * @return true 如果被按下
     */
    [[nodiscard]] static bool isPowered(const BlockState& state) noexcept;

    /**
     * @brief 设置按钮的按下状态
     *
     * @param state 方块状态
     * @param powered 是否按下
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered) noexcept;

    /**
     * @brief 获取按钮附着的方向
     *
     * @param state 方块状态
     * @return Direction 附着方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state) noexcept;

    /**
     * @brief 获取按钮附着的方块方向（附面方向）
     *
     * 地板按钮附在上方方块（Up），天花板按钮附在下方方块（Down），墙按钮附在 FACING 方向。
     * 该方向与 getConnectedDirection().opposite() 等价。
     *
     * @param state 方块状态
     * @return 附着方向
     */
    [[nodiscard]] static Direction getAttachDirection(const BlockState& state) noexcept;

    /**
     * @brief 检查按钮能否留在当前位置（原版 `ButtonBlock` 经
     * `FaceAttachedHorizontalDirectionalBlock#canSurvive` 继承的实现）。
     *
     * 按钮只能贴在附着方块「朝向按钮的那一面」坚固的位置上：墙按钮要求侧面坚固，
     * 地板按钮要求上方坚固，天花板按钮要求下方坚固。此前没有覆写，按钮因此能悬浮在
     * 空气中（由差分能力 block_support 查出）。
     *
     * @param state 方块状态
     * @param world 方块读取器
     * @param pos 按钮位置
     * @return true 如果附着面坚固
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 获取按钮信号输出所面向的方向（原版 getConnectedDirection）
     *
     * 与附着方向相反：地板按钮输出到 Up，天花板按钮输出到 Down，墙按钮输出到 FACING 的反方向。
     *
     * @param state 方块状态
     * @return 输出方向
     */
    [[nodiscard]] static Direction getOutputDirection(const BlockState& state) noexcept;

    /**
     * @brief 按下按钮
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void press(IWorld& world, const BlockPos& pos, const BlockState& state);

protected:
    /// 按压持续时间（tick）
    i32 m_ticksToStayPressed;

    /// 是否可被箭矢触发（木按钮为 true，石质按钮为 false）
    bool m_arrowSensitive;

    /**
     * @brief 播放点击音效
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param pressed true为按下，false为弹起
     */
    virtual void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const = 0;

    /**
     * @brief 检查是否可以附着在指定面
     *
     * @param facing 附着方向
     * @return true 如果可以附着
     */
    [[nodiscard]] bool canAttachToFace(Direction facing) const;

    /**
     * @brief 按"是否有箭矢停留在按钮内"重算 POWERED 状态
     *
     * 对应 vanilla ButtonBlock.checkPressed：状态变化时写回方块并通知邻居、播放音效；
     * 仍为按下时重排计划刻，使箭矢停留期间持续续期。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void checkPressed(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 按钮按下/弹起后通知相邻方块更新
     *
     * 更新按钮自身六个方向的邻居，并额外更新支撑方块位置的邻居（覆盖紧贴支撑的
     * 红石导线等元件）。
     *
     * @param world 世界引用
     * @param pos 方块位置
     */
    void _updateNeighbours(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
