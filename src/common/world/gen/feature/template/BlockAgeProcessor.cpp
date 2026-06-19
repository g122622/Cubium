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

// ============================================================================
// 注意：本文件不在 CMakeLists.txt 编译列表中，仅供参考。
// BlockAgeProcessor 的实际实现在 Template.hpp/cpp 中。
// 修改处理器逻辑时，务必修改 Template.hpp/cpp，而非仅修改本文件。
// ============================================================================

#include "BlockAgeProcessor.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// BlockAgeProcessor 常量
// ============================================================================

// 完整石砖方块被替换的概率（50%）
static constexpr f32 PROBABILITY_OF_REPLACING_FULL_BLOCK = 0.5f;

// 楼梯方块被替换的概率（50%）
static constexpr f32 PROBABILITY_OF_REPLACING_STAIRS = 0.5f;

// 黑曜石变哭泣黑曜石的概率（固定 15%，不受 mossiness 影响）
static constexpr f32 PROBABILITY_OF_REPLACING_OBSIDIAN = 0.15f;

// 非 mossiness 组候选数组大小
static constexpr size_t REPLACEMENT_OPTIONS_COUNT = 2;

BlockAgeProcessor::BlockAgeProcessor(f32 mossiness)
    : m_mossiness(mossiness)
{}

std::optional<ProcessedBlockInfo> BlockAgeProcessor::process(const BlockPos& seedPos,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 随机将石砖相关方块替换为苔藓化或裂变版本
    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    const Block& block = state->getBlock();

    // 使用确定性随机（基于位置）
    u64 hash = math::hashBlockPos(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(static_cast<u64>(hash) ^ static_cast<u64>(seedPos.x * 31 + seedPos.z * 17));

    const BlockState* newState = nullptr;

    // 石砖类完整方块（石砖、石头、錾刻石砖）
    if ((VanillaBlocks::STONE_BRICKS && &block == VanillaBlocks::STONE_BRICKS) ||
        (VanillaBlocks::STONE && &block == VanillaBlocks::STONE) ||
        (VanillaBlocks::CHISELED_STONE_BRICKS && &block == VanillaBlocks::CHISELED_STONE_BRICKS)) {
        newState = _maybeReplaceFullStoneBlock(rng);
    }
    // 楼梯方块（使用标签匹配，保留 facing/half/shape/waterlogged 属性）
    else if (BlockTags::STAIRS().contains(block)) {
        newState = _maybeReplaceStairs(*state, rng);
    }
    // 台阶方块（使用标签匹配，保留 type/waterlogged 属性）
    else if (BlockTags::SLABS().contains(block)) {
        newState = _maybeReplaceSlab(*state, rng);
    }
    // 墙壁方块（使用标签匹配，保留 up/north/south/east/west/waterlogged 属性）
    else if (BlockTags::WALLS().contains(block)) {
        newState = _maybeReplaceWall(*state, rng);
    }
    // 黑曜石
    else if (VanillaBlocks::OBSIDIAN && &block == VanillaBlocks::OBSIDIAN) {
        newState = _maybeReplaceObsidian(rng);
    }

    if (newState) {
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;
        result.blockStateId = newState->stateId();
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 未被替换，返回原始方块
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

const BlockState* BlockAgeProcessor::_maybeReplaceFullStoneBlock(math::Random& rng)
{
    // 50% 概率不替换
    if (rng.nextFloat() >= PROBABILITY_OF_REPLACING_FULL_BLOCK) {
        return nullptr;
    }

    // 非 mossiness 组候选：裂纹石砖 或 随机朝向的石砖楼梯
    const BlockState* nonMossyOptions[] = {
        VanillaBlocks::CRACKED_STONE_BRICKS ? &VanillaBlocks::CRACKED_STONE_BRICKS->defaultState() : nullptr,
        VanillaBlocks::STONE_BRICK_STAIRS ? &_getRandomFacingStairs(rng, *VanillaBlocks::STONE_BRICK_STAIRS) : nullptr};

    // mossiness 组候选：苔藓石砖 或 随机朝向的苔藓石砖楼梯
    const BlockState* mossyOptions[] = {
        VanillaBlocks::MOSSY_STONE_BRICKS ? &VanillaBlocks::MOSSY_STONE_BRICKS->defaultState() : nullptr,
        VanillaBlocks::MOSSY_STONE_BRICK_STAIRS ? &_getRandomFacingStairs(rng, *VanillaBlocks::MOSSY_STONE_BRICK_STAIRS)
                                                : nullptr};

    return _getRandomBlock(rng, nonMossyOptions, mossyOptions);
}

const BlockState* BlockAgeProcessor::_maybeReplaceStairs(const BlockState& state, math::Random& rng)
{
    // 50% 概率不替换
    if (rng.nextFloat() >= PROBABILITY_OF_REPLACING_STAIRS) {
        return nullptr;
    }

    // mossiness 组候选：苔藓石砖楼梯（保留原属性）或 苔藓石砖台阶（默认状态）
    const BlockState* mossyOptions[] = {VanillaBlocks::MOSSY_STONE_BRICK_STAIRS
            ? &VanillaBlocks::MOSSY_STONE_BRICK_STAIRS->defaultState().withPropertiesOf(state)
            : nullptr,
        VanillaBlocks::MOSSY_STONE_BRICK_SLAB ? &VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState() : nullptr};

    // 非 mossiness 组候选：石台阶 或 石砖台阶（默认状态）
    const BlockState* nonMossyOptions[] = {
        VanillaBlocks::STONE_SLAB ? &VanillaBlocks::STONE_SLAB->defaultState() : nullptr,
        VanillaBlocks::STONE_BRICK_SLAB ? &VanillaBlocks::STONE_BRICK_SLAB->defaultState() : nullptr};

    return _getRandomBlock(rng, nonMossyOptions, mossyOptions);
}

const BlockState* BlockAgeProcessor::_maybeReplaceSlab(const BlockState& state, math::Random& rng)
{
    // mossiness 概率替换为苔藓石砖台阶，保留原属性
    if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_STONE_BRICK_SLAB) {
        return &VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState().withPropertiesOf(state);
    }
    return nullptr;
}

const BlockState* BlockAgeProcessor::_maybeReplaceWall(const BlockState& state, math::Random& rng)
{
    // mossiness 概率替换为苔藓石砖墙，保留原属性
    if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_STONE_BRICK_WALL) {
        return &VanillaBlocks::MOSSY_STONE_BRICK_WALL->defaultState().withPropertiesOf(state);
    }
    return nullptr;
}

const BlockState* BlockAgeProcessor::_maybeReplaceObsidian(math::Random& rng)
{
    // 固定 15% 概率替换为哭泣黑曜石
    if (rng.nextFloat() < PROBABILITY_OF_REPLACING_OBSIDIAN && VanillaBlocks::CRYING_OBSIDIAN) {
        return &VanillaBlocks::CRYING_OBSIDIAN->defaultState();
    }
    return nullptr;
}

const BlockState& BlockAgeProcessor::_getRandomFacingStairs(math::Random& rng, const Block& stairsBlock)
{
    // 生成随机朝向的楼梯状态：随机水平朝向 + 随机上半/下半
    const BlockState& defaultState = stairsBlock.defaultState();
    const BlockState* result = &defaultState;

    // 设置随机水平朝向
    static constexpr Direction horizontalDirs[] = {
        Direction::North, Direction::South, Direction::East, Direction::West};
    Direction facing = horizontalDirs[rng.nextInt(4)];
    if (defaultState.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        result = &defaultState.with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 设置随机上半/下半
    if (result->hasProperty(BlockStateProperties::HALF())) {
        auto half = static_cast<BlockStateProperties::Half>(rng.nextInt(2));
        result = &result->with(BlockStateProperties::HALF(), half);
    }

    return *result;
}

const BlockState* BlockAgeProcessor::_getRandomBlock(
    math::Random& rng, const BlockState* const nonMossy[], const BlockState* const mossy[])
{
    // mossiness 概率选择 mossy 组，否则选择 non-mossy 组
    if (rng.nextFloat() < m_mossiness) {
        return _pickRandomNonNull(rng, mossy, REPLACEMENT_OPTIONS_COUNT);
    }
    return _pickRandomNonNull(rng, nonMossy, REPLACEMENT_OPTIONS_COUNT);
}

const BlockState* BlockAgeProcessor::_pickRandomNonNull(
    math::Random& rng, const BlockState* const options[], size_t count)
{
    // 从选项数组中随机选取一个非空元素
    size_t nonNullCount = 0;
    for (size_t i = 0; i < count; ++i) {
        if (options[i] != nullptr) {
            ++nonNullCount;
        }
    }
    if (nonNullCount == 0) {
        return nullptr;
    }
    size_t targetIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(nonNullCount)));
    size_t currentIndex = 0;
    for (size_t i = 0; i < count; ++i) {
        if (options[i] != nullptr) {
            if (currentIndex == targetIndex) {
                return options[i];
            }
            ++currentIndex;
        }
    }
    return nullptr;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
