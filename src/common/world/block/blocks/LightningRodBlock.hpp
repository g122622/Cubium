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

#include "../IWaterLoggable.hpp"
#include "DirectionalBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <cstddef>

namespace mc {
namespace blocks {

/**
 * @brief 避雷针方块
 *
 * 可指向6个方向的避雷针，可被闪电激活并输出红石信号。
 * 支持含水功能。
 *
 * 状态属性：
 * - FACING: 朝向（6方向）
 * - POWERED: 是否被充能
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.LightningRodBlock
 */
class LightningRodBlock : public DirectionalBlock, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LightningRodBlock(const BlockProperties& properties);

    ~LightningRodBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    // ========== Tick ==========

    /**
     * @brief 定时Tick - 充能状态到期后关闭
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 红石 ==========

    /**
     * @brief 避雷针可提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override { return true; }

    /**
     * @brief 获取弱信号强度（POWERED时为15）
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取强信号强度（POWERED时为15）
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== IWaterLoggable ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 避雷针特有 ==========

    /**
     * @brief 闪电击中时调用
     *
     * 设置POWERED为true并安排tick来关闭
     */
    void onLightningStrike(IWorld& world, const BlockPos& pos);

    /**
     * @brief 降水处理
     *
     * 在雷暴天气且避雷针朝上时，有概率被闪电击中而激活。
     * 参考: net.minecraft.block.LightningRodBlock#handlePrecipitation
     *
     * @param world 世界
     * @param pos 方块位置
     * @param precipitation 降水类型（Rain / Snow）
     */
    void handlePrecipitation(
        IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation) override;

    /// 充能持续tick数
    static constexpr i32 ACTIVATION_TICKS = 8;

    /// 闪电检测范围
    static constexpr i32 RANGE = 128;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /// 形状缓存（6方向 × 2充能状态 = 12）
    std::array<CollisionShape, 12> m_shapes;

    /**
     * @brief 计算形状索引
     */
    [[nodiscard]] static size_t _getShapeIndex(Direction facing, bool powered) noexcept;
};

} // namespace blocks
} // namespace mc
