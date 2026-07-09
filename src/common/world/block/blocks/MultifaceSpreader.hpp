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

#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/MultifaceBlock.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"

#include <optional>
#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 扩散目标位置（MC MultifaceSpreader.SpreadPos）
 *
 * pos 为要放置的格子，face 为该格新增面的朝向。
 */
struct MultifaceSpreadPos {
    BlockPos pos;
    Direction face;
};

/**
 * @brief 扩散类型（MC MultifaceSpreader.SpreadType）
 *
 * 从源面 fromFace 向目标方向 spreadDir 扩散时的三种候选位置：
 * - SAME_POSITION：原位 + spreadDir 面。
 * - SAME_PLANE：沿 spreadDir 挪一格，保留 fromFace 面。
 * - WRAP_AROUND：沿 spreadDir 与 fromFace 各挪一格，face 为 fromFace 反方向。
 */
enum class MultifaceSpreadType {
    SamePosition,
    SamePlane,
    WrapAround,
};

/// MC DEFAULT_SPREAD_ORDER = {SAME_POSITION, SAME_PLANE, WRAP_AROUND}。
extern const std::vector<MultifaceSpreadType>& defaultSpreadOrder();

/**
 * @brief 扩散配置策略（MC MultifaceSpreader.SpreadConfig）
 *
 * 不同多面方块（如 SculkVein）可覆写 stateCanBeReplaced / getSpreadTypes /
 * isOtherBlockValidAsSource 来定制蔓延行为。默认实现 DefaultSpreaderConfig
 * 适用于普通多面方块（发光地衣等）。
 */
class MultifaceSpreadConfig {
public:
    virtual ~MultifaceSpreadConfig() = default;

    /// MC SpreadConfig.getStateForPlacement。current 为 nullptr 表示空气。
    [[nodiscard]] virtual const BlockState* getStateForPlacement(
        const BlockState* current, IWorld& world, const BlockPos& pos, Direction face) const = 0;

    /// MC SpreadConfig.canSpreadInto（sourcePos 为扩散源格，spreadPos 为候选目标）。
    [[nodiscard]] virtual bool canSpreadInto(
        IWorld& world, const BlockPos& sourcePos, const MultifaceSpreadPos& spreadPos) const = 0;

    /// MC SpreadConfig.getSpreadTypes（默认 DEFAULT_SPREAD_ORDER）。
    [[nodiscard]] virtual const std::vector<MultifaceSpreadType>& getSpreadTypes() const
    {
        return defaultSpreadOrder();
    }

    /// MC SpreadConfig.hasFace。
    [[nodiscard]] virtual bool hasFace(const BlockState& state, Direction direction) const
    {
        return MultifaceBlock::hasFace(state, direction);
    }

    /// MC SpreadConfig.isOtherBlockValidAsSource（默认 false）。
    [[nodiscard]] virtual bool isOtherBlockValidAsSource(const BlockState& /*state*/) const { return false; }

    /// MC SpreadConfig.canSpreadFrom（isOtherBlockValidAsSource || hasFace）。
    [[nodiscard]] bool canSpreadFrom(const BlockState& state, Direction direction) const
    {
        return isOtherBlockValidAsSource(state) || hasFace(state, direction);
    }

    /// MC SpreadConfig.placeBlock：取 getStateForPlacement 并 setBlock（worldGen 时标记后处理）。
    /// current 为 nullptr 表示空气（项目空气方块用 nullptr 表示）。
    [[nodiscard]] bool placeBlock(
        IWorld& world, const MultifaceSpreadPos& spreadPos, const BlockState* current, bool worldGen) const;
};

/**
 * @brief 默认扩散配置（MC MultifaceSpreader.DefaultSpreaderConfig）
 *
 * stateCanBeReplaced：空气 / 同种多面方块 / 水源。
 * canSpreadInto：stateCanBeReplaced 且 isValidStateForPlacement。
 */
class DefaultSpreaderConfig : public MultifaceSpreadConfig {
public:
    explicit DefaultSpreaderConfig(const MultifaceBlock& block)
        : m_block(block)
    {}

    [[nodiscard]] const BlockState* getStateForPlacement(
        const BlockState* current, IWorld& world, const BlockPos& pos, Direction face) const override
    {
        return m_block.getStateForPlacement(current, world, pos, face);
    }

    /// MC DefaultSpreaderConfig.stateCanBeReplaced：空气 / 同种多面方块 / 水源。
    [[nodiscard]] virtual bool stateCanBeReplaced(const BlockState& state) const
    {
        if (state.isAir()) {
            return true;
        }
        if (state.is(&m_block)) {
            return true;
        }
        const fluid::FluidState* fluid = state.getFluidState();
        return fluid != nullptr && fluid->isSource() && &fluid->getFluid() == fluid::Fluids::WATER();
    }

    [[nodiscard]] bool canSpreadInto(
        IWorld& world, const BlockPos& sourcePos, const MultifaceSpreadPos& spreadPos) const override
    {
        // MC DefaultSpreaderConfig.canSpreadInto: stateCanBeReplaced(target) && isValidStateForPlacement。
        // nullptr 视为空气：可替换，走 isValidStateForPlacement 的 canAttachTo 判定。
        MC_UNUSED(sourcePos);
        const BlockState* target = world.getBlockState(spreadPos.pos);
        if (target == nullptr) {
            return m_block.canAttachTo(world, spreadPos.face, spreadPos.pos.offset(spreadPos.face));
        }
        if (!stateCanBeReplaced(*target)) {
            return false;
        }
        return m_block.isValidStateForPlacement(world, *target, spreadPos.pos, spreadPos.face);
    }

protected:
    const MultifaceBlock& m_block;
};

/**
 * @brief 多面方块扩散器（MC MultifaceSpreader）
 *
 * 从某一面出发，按 SpreadType 优先级（SAME_POSITION > SAME_PLANE > WRAP_AROUND）
 * 向一个目标方向尝试扩散：原地加面 / 同平面相邻格 / 对角卷绕。
 */
class MultifaceSpreader {
public:
    /// MC MultifaceSpreader(MultifaceBlock)：用 DefaultSpreaderConfig 包装。
    explicit MultifaceSpreader(const MultifaceBlock& block)
        : m_config(std::make_unique<DefaultSpreaderConfig>(block))
    {}

    /// MC MultifaceSpreader(SpreadConfig)。
    explicit MultifaceSpreader(std::unique_ptr<MultifaceSpreadConfig> config)
        : m_config(std::move(config))
    {}

    MultifaceSpreader(const MultifaceSpreader&) = delete;
    MultifaceSpreader& operator=(const MultifaceSpreader&) = delete;
    MultifaceSpreader(MultifaceSpreader&&) noexcept = default;
    MultifaceSpreader& operator=(MultifaceSpreader&&) noexcept = default;

    /**
     * @brief MC spreadFromFaceTowardRandomDirection（5 参便捷版，worldGen=true）：
     *        从 fromFace 出发，对打乱后的所有方向逐个尝试扩散，命中即止。
     * @return 是否至少成功扩散一次。
     *
     * multiface_growth（发光地衣/脉络骨粉催生）使用此版本。
     */
    bool spreadFromFaceTowardRandomDirection(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, math::IRandom& random) const;

    /**
     * @brief MC spreadFromFaceTowardRandomDirection（6 参版）：
     *        返回首个成功扩散的 SpreadPos。
     */
    [[nodiscard]] std::optional<MultifaceSpreadPos> spreadFromFaceTowardRandomDirection(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction fromFace,
        math::IRandom& random,
        bool worldGen) const;

    /**
     * @brief MC spreadAll：对所有可扩散面，向所有方向尝试扩散，返回成功次数。
     *
     * SculkVein.attemptPlaceSculk 用此在新放置的 sculk 周围蔓延脉络。
     */
    [[nodiscard]] i64 spreadAll(const BlockState& state, IWorld& world, const BlockPos& pos, bool worldGen) const;

    /// MC MultifaceSpreader.canSpreadInAnyDirection。
    [[nodiscard]] bool canSpreadInAnyDirection(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace) const;

private:
    /// MC getSpreadFromFaceTowardDirection：计算单方向的 SpreadPos（满足 canSpreadInto）。
    [[nodiscard]] std::optional<MultifaceSpreadPos> getSpreadFromFaceTowardDirection(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, Direction spreadDir) const;

    /// MC spreadFromFaceTowardAllDirections：从 fromFace 向所有方向扩散，返回成功次数。
    [[nodiscard]] i64 spreadFromFaceTowardAllDirections(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, bool worldGen) const;

    /// MC SpreadType.getSpreadPos。
    [[nodiscard]] static MultifaceSpreadPos getSpreadPos(
        const BlockPos& pos, Direction spreadDir, Direction fromFace, MultifaceSpreadType type);

    /// MC spreadToFace：在 spreadPos 放置方块，成功则返回该 SpreadPos。
    [[nodiscard]] std::optional<MultifaceSpreadPos> spreadToFace(
        IWorld& world, const MultifaceSpreadPos& spreadPos, bool worldGen) const;

    std::unique_ptr<MultifaceSpreadConfig> m_config;
};

} // namespace blocks
} // namespace mc
