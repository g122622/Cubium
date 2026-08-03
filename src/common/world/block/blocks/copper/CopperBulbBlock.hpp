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

#include "WeatheringCopperBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

class IWorld;
class BlockPos;

namespace blocks {

/**
 * @brief 铜灯方块
 *
 * 红石可控光源。铜灯在红石信号的上升沿切换开关状态（LIT）。
 * POWERED 属性追踪红石信号状态，LIT 属性追踪灯是否点亮。
 * 氧化等级不影响红石逻辑，仅影响外观。
 *
 * 状态属性：
 * - LIT: 是否点亮
 * - POWERED: 是否被红石信号驱动
 * - OXIDATION: 氧化等级（通过 WeatheringCopperBlock 继承）
 *
 * 参考: net.minecraft.block.CopperBulbBlock (1.21)
 */
class CopperBulbBlock : public WeatheringCopperBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 氧化等级
     */
    CopperBulbBlock(const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel);

    ~CopperBulbBlock() override = default;

    /**
     * @brief 邻居更新 - 检测红石信号变化并切换灯状态
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 获取发光等级
     *
     * 点亮时发光等级15，熄灭时为0
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return state.get(BlockStateProperties::LIT()) ? 15 : 0;
    }

    /**
     * @brief 是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return false; }

    /**
     * @brief 是否有比较器输入覆盖
     *
     * 铜灯支持比较器输出
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override;

    /**
     * @brief 获取比较器输入覆盖值
     *
     * 点亮时返回15，熄灭时返回0
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 涂蜡铜灯方块
 *
 * 涂蜡版本的铜灯，不会氧化。同样具有LIT和POWERED属性。
 *
 * 参考: net.minecraft.block.CopperBulbBlock (waxed variant)
 */
class WaxedCopperBulbBlock : public WaxedCopperBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WaxedCopperBulbBlock(const BlockProperties& properties);

    ~WaxedCopperBulbBlock() override = default;

    /**
     * @brief 邻居更新 - 检测红石信号变化并切换灯状态
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 获取发光等级
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return state.get(BlockStateProperties::LIT()) ? 15 : 0;
    }

    /**
     * @brief 是否有比较器输入覆盖
     *
     * 涂蜡铜灯支持比较器输出
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override;

    /**
     * @brief 获取比较器输入覆盖值
     *
     * 点亮时返回15，熄灭时返回0
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
