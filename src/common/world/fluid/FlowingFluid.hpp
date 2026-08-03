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
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <unordered_map>
#include <utility>

namespace mc {
namespace fluid {

/**
 * @brief 流动流体抽象类
 *
 * 实现流体流动的核心算法，包括向下流动、水平扩散、源头形成等。
 * 水和岩浆都继承此类。
 *
 * 流动规则：
 * 1. 优先向下流动
 * 2. 水平扩散，距离由getSpreadDistance()决定
 * 3. 水可以形成无限源（2+相邻源头）
 * 4. 岩浆不能形成无限源
 */
class FlowingFluid : public Fluid {
public:
    // ========== 属性 ==========

    using Fluid::getTickDelay;

    /**
     * @brief 获取流动流体实例
     *
     * 返回对应的流动版本（非源头）。
     */
    [[nodiscard]] virtual FlowingFluid& getFlowing() = 0;

    /**
     * @brief 获取源头流体实例
     */
    [[nodiscard]] virtual FlowingFluid& getStill() = 0;

    /**
     * @brief 每格高度衰减
     *
     * 水: 1
     * 岩浆(主世界): 2
     * 岩浆(下界): 1
     */
    [[nodiscard]] virtual i32 getLevelDecrease(IWorld& world) const = 0;

    /**
     * @brief 斜坡搜索距离（用于水平扩散路径搜索）
     *
     * 水: 4格
     * 岩浆(主世界): 2格
     * 岩浆(下界): 4格
     */
    [[nodiscard]] virtual i32 getSpreadDistance(IWorld& world) const = 0;

    /**
     * @brief 获取流动状态
     *
     * @param level 等级 (1-8)
     * @param falling 是否下落
     * @return 流体状态
     */
    [[nodiscard]] FluidState getFlowingState(i32 level, bool falling) const;

    /**
     * @brief 获取源头状态
     *
     * @param falling 是否下落（水源头通常为false）
     * @return 源头流体状态
     */
    [[nodiscard]] FluidState getStillState(bool falling) const;

    // ========== Fluid接口实现 ==========

    void tick(IWorld& world, const BlockPos& pos, FluidState& state) override;

    [[nodiscard]] Vector3 getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const override;

    [[nodiscard]] CollisionShape getShape(
        const FluidState& state, IBlockReader& world, const BlockPos& pos) const override;

protected:
    /**
     * @brief 计算下一个流体 tick 的延迟
     *
     * 默认直接使用流体自身的基础 tick 延迟；岩浆会在此基础上做额外节流。
     */
    [[nodiscard]] virtual i32 getTickDelay(
        IWorld& world, const BlockPos& pos, const FluidState& state, const FluidState& correctState) const;

    /**
     * @brief 替换方块前的处理
     *
     * 当流体替换方块时调用。岩浆会在此处理与水的交互。
     *
     * @param world 世界
     * @param pos 位置
     * @param state 被替换的方块状态
     */
    virtual void beforeReplacingBlock(IWorld& world, const BlockPos& pos, const BlockState* state) = 0;

    /**
     * @brief 执行流动扩散
     *
     * 计算流体应该流向哪些方向，并执行流动。
     *
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前流体状态
     */
    void flowAround(IWorld& world, const BlockPos& pos, const FluidState& state);

    /**
     * @brief 尝试向指定方向流动
     *
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前流体状态
     * @param dir 流动方向
     */
    void tryFlow(IWorld& world, const BlockPos& pos, const FluidState& state, Direction dir);

    /**
     * @brief 流入指定位置
     *
     * @param world 世界
     * @param pos 目标位置
     * @param blockState 目标位置的方块状态
     * @param dir 流入方向
     * @param state 流入的流体状态
     */
    virtual void flowInto(
        IWorld& world, const BlockPos& pos, const BlockState* blockState, Direction dir, const FluidState& state);

    /**
     * @brief 计算正确的流动状态
     *
     * 根据周围流体计算当前位置应该有的流体状态。
     * 包括源头形成检测和下落流体检测。
     *
     * @param world 世界
     * @param pos 位置
     * @param blockState 当前方块状态
     * @return 正确的流体状态，如果应该消失则返回空
     */
    [[nodiscard]] FluidState calculateCorrectFlowingState(
        IWorld& world, const BlockPos& pos, const BlockState* blockState) const;

    /**
     * @brief 检查是否可以流向指定位置
     *
     * @param world 世界
     * @param fromPos 源位置
     * @param fromBlock 源方块状态
     * @param dir 流动方向
     * @param toPos 目标位置
     * @param toBlock 目标方块状态
     * @param toFluid 目标流体状态
     * @param fluid 流入的流体类型
     * @return 是否可以流动
     */
    [[nodiscard]] bool canFlow(IWorld& world,
        const BlockPos& fromPos,
        const BlockState* fromBlock,
        Direction dir,
        const BlockPos& toPos,
        const BlockState* toBlock,
        const FluidState& toFluid,
        const Fluid& fluid) const;

    /**
     * @brief 检查是否可以流入指定位置
     *
     * @param world 世界
     * @param fromPos 源位置
     * @param fromBlock 源方块状态
     * @param dir 流动方向
     * @param toPos 目标位置
     * @param toBlock 目标方块状态
     * @param toFluid 目标流体状态
     * @param fluidIn 用于阻挡判断的流体类型
     * @return 是否可以流入
     */
    [[nodiscard]] bool canFlowInto(IWorld& world,
        const BlockPos& fromPos,
        const BlockState* fromBlock,
        Direction dir,
        const BlockPos& toPos,
        const BlockState* toBlock,
        const FluidState& toFluid,
        const Fluid& fluidIn) const;

    /**
     * @brief 检查侧面是否有孔洞
     *
     * 判断两个方块之间是否可以通过流体。
     *
     * @param dir 方向
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前方块状态
     * @param neighborPos 相邻位置
     * @param neighborState 相邻方块状态
     * @return 是否有孔洞
     */
    [[nodiscard]] bool doesSideHaveHoles(Direction dir,
        IWorld& world,
        const BlockPos& pos,
        const BlockState* state,
        const BlockPos& neighborPos,
        const BlockState* neighborState) const;

    /**
     * @brief 检查是否可以形成源头
     *
     * 水: 需要2+相邻源头且下方为固体或同种流体
     * 岩浆: 永远返回false
     *
     * @param world 世界
     * @param pos 位置
     * @return 是否可以形成源头
     */
    [[nodiscard]] bool canFormSource(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取水平相邻源头数量
     *
     * @param world 世界
     * @param pos 位置
     * @return 相邻源头数量
     */
    [[nodiscard]] i32 getHorizontalSourceCount(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查方块是否阻挡流体（不可被流体替换）
     *
     * 返回 true 表示方块阻挡流体流入；false 表示流体可以替换/流入该方块。
     * 在 canBeReplacedByFluid() 之上叠加 MC Java canHoldAnyFluid 黑名单（门/告示牌/梯子/甘蔗/气泡柱/传送门/结构空位）。
     *
     * @param block 方块状态
     * @param world 世界
     * @param pos 位置
     * @param fluid 流体类型
     * @return true=阻挡流体，false=可被流体替换
     */
    [[nodiscard]] bool isBlocked(IWorld& world, const BlockPos& pos, const BlockState* block, const Fluid& fluid) const;

    /**
     * @brief 检查是否为相同流体或空
     *
     * @param state 流体状态
     * @return 是否为相同流体或空
     */
    [[nodiscard]] bool isSameOrEmpty(const FluidState& state) const;

    /**
     * @brief 检查是否为同种流体源头
     *
     * @param state 流体状态
     * @return 是否为同种流体源头
     */
    [[nodiscard]] bool isSameSource(const FluidState& state) const;

    /**
     * @brief 获取相邻流动方向和状态
     *
     * 计算流体应该流向哪些方向，按优先级排序。
     *
     * @param world 世界
     * @param pos 位置
     * @param blockState 方块状态
     * @return 方向 -> 流体状态的映射
     */
    [[nodiscard]] std::unordered_map<Direction, FluidState> getFlowDirections(
        IWorld& world, const BlockPos& pos, const BlockState* blockState);

    /**
     * @brief 检查是否会产生向下流动
     *
     * @param world 世界
     * @param pos 位置
     * @param side 方向
     * @return 是否会导致向下流动
     */
    [[nodiscard]] bool causesDownwardCurrent(IBlockReader& world, const BlockPos& pos, Direction side) const;

    /**
     * @brief 检查流体是否为满高度
     *
     * @param state 流体状态
     * @param world 世界
     * @param pos 位置
     * @return 是否为满高度
     */
    [[nodiscard]] bool isFullHeight(const FluidState& state, IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 水平扩散流体
     *
     * @param world 世界
     * @param pos 位置
     * @param state 流体状态
     * @param blockState 方块状态
     */
    void spreadHorizontally(IWorld& world, const BlockPos& pos, const FluidState& state, const BlockState* blockState);

    /**
     * @brief 检查是否可以向下流动
     *
     * @param world 世界
     * @param fluid 流体类型
     * @param pos 当前位置
     * @param blockState 当前方块状态
     * @param belowPos 下方位置
     * @param belowBlock 下方方块状态
     * @return 是否可以向下流动
     */
    [[nodiscard]] bool canFlowDown(IWorld& world,
        const Fluid& fluid,
        const BlockPos& pos,
        const BlockState* blockState,
        const BlockPos& belowPos,
        const BlockState* belowBlock) const;

    /**
     * @brief 检查是否可以流入指定位置
     *
     * @param world 世界
     * @param fromPos 源位置
     * @param fromBlock 源方块状态
     * @param dir 流动方向
     * @param toPos 目标位置
     * @param toBlock 目标方块状态
     * @param toFluid 目标流体状态
     * @return 是否可以流入
     */
    /**
     * @brief 计算流动衰减值
     *
     * @param world 世界
     * @param pos 位置
     * @param decay 当前衰减值
     * @param excludeDir 排除的方向
     * @param blockState 方块状态
     * @param sourcePos 源位置
     * @param stateCache 状态缓存
     * @param fallCache 下落缓存
     * @return 最小衰减值
     */
    [[nodiscard]] i32 calculateFlowDecay(IWorld& world,
        const BlockPos& pos,
        i32 decay,
        Direction excludeDir,
        const BlockState* blockState,
        const BlockPos& sourcePos,
        std::unordered_map<i16, std::pair<const BlockState*, const FluidState*>>& stateCache,
        std::unordered_map<i16, bool>& fallCache) const;

    /**
     * @brief 打包相对位置为16位整数
     *
     * @param source 源位置
     * @param target 目标位置
     * @return 打包后的值
     */
    [[nodiscard]] i16 packRelativePos(const BlockPos& source, const BlockPos& target) const noexcept;
};

} // namespace fluid
} // namespace mc
