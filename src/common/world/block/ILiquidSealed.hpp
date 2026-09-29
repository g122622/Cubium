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

#include "common/world/block/ILiquidContainer.hpp"

namespace mc {

/**
 * @brief 液体密封方块：既不接收液体，也不被液体替换
 *
 * 对齐原版 LiquidBlockContainer 中 canPlaceLiquid()/placeLiquid() 恒返 false 的水下植物
 * （kelp / kelp_plant / seagrass / tall_seagrass）。它们占据的空间是"水中的植物"，
 * 流体既不能灌入该格，也不能把该格上的植物替换掉。
 *
 * 【为何必须显式实现】FlowingFluid::isBlocked 对**未实现 ILiquidContainer** 的方块只按
 * `canBeReplacedByFluid()`（= `canBeReplaced || !isSolid`）判定是否可被流体替换。这四类
 * 方块注册时都带 `.noCollision().notSolid()`，`m_isSolid` 为 false，于是被判为"可被流体替换"：
 * 水流每次 tick 都把海带当作被冲毁的方块，经 WaterFluid::beforeReplacingBlock 当作"方块被
 * 水破坏"生成一次掉落物。实测单次会话即堆积 19326 个掉落物实体（该批区块常驻内存，而实体在
 * 模拟距离外被冻结、age 不增长，故永不超龄消失）。
 *
 * 语义上本接口是 IWaterLoggable（总是接收水）的对立面，两者都派生自 ILiquidContainer。
 */
class ILiquidSealed : public ILiquidContainer {
public:
    ~ILiquidSealed() override = default;

    /**
     * @brief 永不接收液体
     *
     * @return 恒返 false（对齐原版 LiquidBlockContainer::canPlaceLiquid 的 false 实现）
     */
    [[nodiscard]] bool canContainFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid) const override;

    /**
     * @brief 永不接收液体，故无接收动作
     *
     * @return 恒返 false
     */
    bool receiveFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState) override;

    /**
     * @brief 永不包含液体
     *
     * @return 恒返 false
     */
    [[nodiscard]] bool containsFluid(IWorld& world, const BlockPos& pos, const BlockState& state) const override;
};

} // namespace mc
