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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../IWaterLoggable.hpp"
#include "AbstractCandleBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <vector>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 蜡烛方块
 *
 * 可堆叠1-4根蜡烛的装饰性光源方块，可被点燃/熄灭，支持含水功能。
 *
 * ## 状态属性
 * - CANDLES: 蜡烛数量 (1-4)
 * - LIT: 是否点燃
 * - WATERLOGGED: 是否含水
 *
 * ## 放置行为
 * - 首次放置：CANDLES=1，根据环境检测含水
 * - 右键已有蜡烛：CANDLES+1（最多4），需手持同类蜡烛物品且未潜行
 * - 潜行右键：正常放置新方块而非堆叠
 *
 * ## 点燃/熄灭
 * - 未点燃且未含水时可被点燃
 * - 含水时无法点燃
 * - 点燃时亮度 = 3 * CANDLES（1根=3, 2根=6, 3根=9, 4根=12）
 *
 * ## 支撑条件
 * - 下方方块必须有坚固的上表面（isSolidSide(Direction::Up)）
 */
class CandleBlock : public AbstractCandleBlock, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CandleBlock(BlockProperties properties);

    ~CandleBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取蜡烛数量属性
     */
    [[nodiscard]] static const IntegerProperty& CANDLES() { return BlockStateProperties::CANDLES(); }

    /**
     * @brief 获取点燃状态属性
     */
    [[nodiscard]] static const BooleanProperty& LIT() { return BlockStateProperties::LIT(); }

    /**
     * @brief 获取含水状态属性
     */
    [[nodiscard]] static const BooleanProperty& WATERLOGGED() { return BlockStateProperties::WATERLOGGED(); }

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 如果目标位置已有同类型蜡烛且数量<4，增加数量；
     * 否则创建新方块，CANDLES=1，检测含水状态。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查是否可以放置在指定位置
     *
     * 下方方块必须有坚固的上表面。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居方块更新
     *
     * 含水时调度水流tick；下方支撑变化时检查有效性。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 检查方块是否可被替换（用于堆叠放置）
     *
     * 当玩家未潜行、手持同类蜡烛物品且数量<4时返回true。
     */
    [[nodiscard]] bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取碰撞形状（根据蜡烛数量变化）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 点燃时亮度 = 3 * 蜡烛数量，熄灭时不发光。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 点燃 ==========

    /**
     * @brief 检查蜡烛是否可以被点燃
     *
     * 未点燃且未含水时可点燃。
     */
    [[nodiscard]] bool canBeLit(const BlockState& state) const override;

    // ========== 粒子 ==========

    /**
     * @brief 获取粒子偏移位置
     *
     * 根据蜡烛数量返回对应的火焰粒子偏移位置列表。
     */
    [[nodiscard]] std::vector<Vector3f> getParticleOffsets(const BlockState& state) const override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键交互
     *
     * 空手右键点燃的蜡烛可熄灭。
     * 点燃（打火石/火焰弹）由物品自身处理，不在此覆写中处理。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== Tick ==========

    /**
     * @brief 方块 tick
     *
     * 含水时自动熄灭蜡烛。
     * 通过 ticksRandomly() 使随机刻系统调用此方法。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     *
     * 蜡烛需要响应随机刻以检测含水状态并熄灭。
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查蜡烛是否可以被点燃
     *
     * 静态工具方法，检查方块状态是否包含 LIT 和 WATERLOGGED 属性，
     * 且未点燃、未含水。
     *
     * @param state 方块状态
     * @return 是否可点燃
     */
    [[nodiscard]] static bool canLight(const BlockState& state);

private:
    /// 各数量的碰撞形状（索引0=1根，索引1=2根，...）
    std::array<CollisionShape, 4> m_shapesByCount;
};

} // namespace blocks
} // namespace mc
