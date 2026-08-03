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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/world/block/blocks/vegetation/SaplingBlock.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
namespace mc {
namespace blocks {

namespace {

[[nodiscard]] bool isSaplingGround(const BlockState& groundState)
{
    return groundState.is(VanillaBlocks::GRASS_BLOCK) || groundState.is(VanillaBlocks::DIRT) ||
        groundState.is(VanillaBlocks::COARSE_DIRT) || groundState.is(VanillaBlocks::PODZOL) ||
        groundState.is(VanillaBlocks::FARMLAND);
}

} // namespace

// ========== 构造函数 ==========

SaplingBlock::SaplingBlock(TreeGenerator treeGenerator, const BlockProperties& properties)
    : BushBlock(properties)
    , m_treeGenerator(std::move(treeGenerator))
{

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::STAGE_0_1())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::STAGE_0_1(), 0));

    // 树苗形状：小型植物
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
}

// ========== 状态属性 ==========

i32 SaplingBlock::getStage(const BlockState& state) const
{
    return state.get(BlockStateProperties::STAGE_0_1());
}

const BlockState& SaplingBlock::withStage(i32 stage) const
{
    return defaultState().with(BlockStateProperties::STAGE_0_1(), std::clamp(stage, 0, 1));
}

// ========== 放置逻辑 ==========

BlockState SaplingBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState();
}

// ========== 生长逻辑 ==========

void SaplingBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    const BlockPos lightPos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(lightPos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(lightPos));
    if (std::max(blockLight, skyLight) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    if (random.nextInt(7) != 0) {
        return;
    }

    grow(world, pos, state);
}

bool SaplingBlock::grow(IWorld& world, const BlockPos& pos, BlockState& state)
{
    i32 stage = getStage(state);

    if (stage < 1) {
        const BlockState& nextState = withStage(stage + 1);
        world.setBlockState(pos, &nextState, 2);
        return true;
    }

    if (!m_treeGenerator) {
        return false;
    }

    // 通过 IWorld::createFeatureRegion() 从已加载区块构建 WorldGenRegion
    // ServerWorld 会重写此方法，返回有效的 WorldGenRegion；
    // 客户端和其他实现返回 nullptr
    auto region = world.createFeatureRegion(pos);
    if (region == nullptr) {
        // 非服务器环境或周围区块未加载，无法生成树木
        spdlog::debug("SaplingBlock::grow: cannot create WorldGenRegion at ({}, {}, {})", pos.x, pos.y, pos.z);
        return false;
    }

    // 使用世界种子和位置派生随机数种子
    u64 seed = world.seed();
    seed ^= static_cast<u64>(static_cast<i64>(pos.x)) * 3129871ULL;
    seed ^= static_cast<u64>(static_cast<i64>(pos.y)) * 116129781ULL;
    seed ^= static_cast<u64>(static_cast<i64>(pos.z)) * 42317861ULL;

    math::Random rng(0);
    rng.setSeedWithHash(static_cast<i64>(seed));

    // 清除树苗方块
    const BlockState* airState = BlockRegistry::instance().airState();
    world.setBlockState(pos, airState, 2);

    // 通过 WorldGenRegion 调用树木生成器
    m_treeGenerator(*region, pos, rng);
    return true;
}

// ========== 形状 ==========

const CollisionShape& SaplingBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

// ========== 保护方法 ==========

bool SaplingBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    return isSaplingGround(groundState);
}

// ========== IGrowable 接口 ==========

bool SaplingBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    return true;
}

bool SaplingBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 骨粉有概率成功（MC 原版树苗骨粉概率约 45%）
    return random.nextFloat() < 0.45f;
}

void SaplingBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    // IGrowable::grow 调用内部的 grow 方法
    BlockState mutableState = state;
    SaplingBlock::grow(world, pos, mutableState);
}

} // namespace blocks
} // namespace mc
