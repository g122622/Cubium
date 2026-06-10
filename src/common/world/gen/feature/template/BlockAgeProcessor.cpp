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

#include "BlockAgeProcessor.hpp"

#include "common/util/math/MathUtils.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// 黑曜石 -> 哭泣黑曜石的概率（固定 15%，不受 mossiness 影响）
inline constexpr f32 OBSIDIAN_TO_CRYING_PROBABILITY = 0.15f;

// 石砖类方块不替换的概率（50%）
inline constexpr f32 STONE_BRICK_NO_REPLACE_CHANCE = 0.5f;

// 石砖楼梯苔藓化概率
inline constexpr f32 STONE_BRICK_STAIRS_MOSS_CHANCE = 0.5f;

// 裂纹石砖出现概率（非 mossiness 分支中）
inline constexpr f32 CRACKED_STONE_BRICK_CHANCE = 0.3f;

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

    ProcessedBlockInfo result;
    result.pos = blockInfo.pos;
    result.blockStateId = blockInfo.blockStateId; // 默认保持原样

    // 黑曜石 -> 哭泣黑曜石（固定 15% 概率，不受 mossiness 影响）
    if (VanillaBlocks::OBSIDIAN && &block == VanillaBlocks::OBSIDIAN) {
        if (rng.nextFloat() < OBSIDIAN_TO_CRYING_PROBABILITY && VanillaBlocks::CRYING_OBSIDIAN) {
            result.blockStateId = VanillaBlocks::CRYING_OBSIDIAN->defaultState().stateId();
        }
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 石砖类方块处理（石砖、石头、錾刻石砖）
    bool isStoneBrickType = (VanillaBlocks::STONE_BRICKS && &block == VanillaBlocks::STONE_BRICKS) ||
        (VanillaBlocks::STONE && &block == VanillaBlocks::STONE) ||
        (VanillaBlocks::CHISELED_STONE_BRICKS && &block == VanillaBlocks::CHISELED_STONE_BRICKS);

    if (isStoneBrickType) {
        // 50% 概率不替换
        if (rng.nextFloat() < STONE_BRICK_NO_REPLACE_CHANCE) {
            if (blockInfo.nbt) {
                result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
            }
            return result;
        }

        // mossiness 概率组 vs 非 mossiness 组
        if (rng.nextFloat() < m_mossiness) {
            // Mossiness 组：苔藓石砖
            if (VanillaBlocks::MOSSY_STONE_BRICKS) {
                result.blockStateId = VanillaBlocks::MOSSY_STONE_BRICKS->defaultState().stateId();
            }
        } else {
            // 非 mossiness 组：裂纹石砖
            if (rng.nextFloat() < CRACKED_STONE_BRICK_CHANCE && VanillaBlocks::CRACKED_STONE_BRICKS) {
                result.blockStateId = VanillaBlocks::CRACKED_STONE_BRICKS->defaultState().stateId();
            }
        }

        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 圆石 -> 苔藓圆石
    if (VanillaBlocks::COBBLESTONE && &block == VanillaBlocks::COBBLESTONE) {
        if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_COBBLESTONE) {
            result.blockStateId = VanillaBlocks::MOSSY_COBBLESTONE->defaultState().stateId();
        }
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 石砖楼梯 -> 苔藓石砖楼梯
    // TODO: 保留原方块的属性
    if (VanillaBlocks::STONE_BRICK_STAIRS && &block == VanillaBlocks::STONE_BRICK_STAIRS) {
        if (rng.nextFloat() < STONE_BRICK_STAIRS_MOSS_CHANCE && VanillaBlocks::MOSSY_STONE_BRICK_STAIRS) {
            result.blockStateId = VanillaBlocks::MOSSY_STONE_BRICK_STAIRS->defaultState().stateId();
        }
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 石砖台阶 -> 苔藓石砖台阶
    if (VanillaBlocks::STONE_BRICK_SLAB && &block == VanillaBlocks::STONE_BRICK_SLAB) {
        if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_STONE_BRICK_SLAB) {
            result.blockStateId = VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState().stateId();
        }
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 石砖墙 -> 苔藓石砖墙
    if (VanillaBlocks::STONE_BRICK_WALL && &block == VanillaBlocks::STONE_BRICK_WALL) {
        if (rng.nextFloat() < m_mossiness && VanillaBlocks::MOSSY_STONE_BRICK_WALL) {
            result.blockStateId = VanillaBlocks::MOSSY_STONE_BRICK_WALL->defaultState().stateId();
        }
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    // 复制 NBT（如果有）
    if (blockInfo.nbt) {
        result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
    }

    return result;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
