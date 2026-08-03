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
// 实际实现在 Template.hpp/cpp 中。
// 修改逻辑时，务必修改 Template.hpp/cpp，而非仅修改本文件。
// ============================================================================

#include "BlackstoneReplacementProcessor.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

BlackstoneReplacementProcessor::BlackstoneReplacementProcessor()
{
    // 黑石替换映射：将普通石质方块替换为黑石变体，用于堡垒遗迹

    auto& registry = BlockRegistry::instance();

    // 辅助lambda：根据名称获取方块ID
    auto getBlockId = [&registry](const char* name) -> u32 {
        ResourceLocation loc(name);
        Block* block = registry.getBlock(loc);
        if (block) {
            return block->blockId();
        }
        return 0; // 空气/未找到
    };

    // 基础方块替换映射
    u32 cobblestone = getBlockId("minecraft:cobblestone");
    u32 blackstone = getBlockId("minecraft:blackstone");
    u32 mossyCobblestone = getBlockId("minecraft:mossy_cobblestone");
    u32 stone = getBlockId("minecraft:stone");
    u32 polishedBlackstone = getBlockId("minecraft:polished_blackstone");
    u32 stoneBricks = getBlockId("minecraft:stone_bricks");
    u32 polishedBlackstoneBricks = getBlockId("minecraft:polished_blackstone_bricks");
    u32 mossyStoneBricks = getBlockId("minecraft:mossy_stone_bricks");
    u32 crackedStoneBricks = getBlockId("minecraft:cracked_stone_bricks");
    u32 crackedPolishedBlackstoneBricks = getBlockId("minecraft:cracked_polished_blackstone_bricks");
    u32 chiseledStoneBricks = getBlockId("minecraft:chiseled_stone_bricks");
    u32 chiseledPolishedBlackstone = getBlockId("minecraft:chiseled_polished_blackstone");
    u32 ironBars = getBlockId("minecraft:iron_bars");
    u32 chain = getBlockId("minecraft:iron_chain");

    // 基础方块替换
    if (cobblestone && blackstone) {
        m_replacements[cobblestone] = blackstone;
    }
    if (mossyCobblestone && blackstone) {
        m_replacements[mossyCobblestone] = blackstone;
    }
    if (stone && polishedBlackstone) {
        m_replacements[stone] = polishedBlackstone;
    }
    if (stoneBricks && polishedBlackstoneBricks) {
        m_replacements[stoneBricks] = polishedBlackstoneBricks;
    }
    if (mossyStoneBricks && polishedBlackstoneBricks) {
        m_replacements[mossyStoneBricks] = polishedBlackstoneBricks;
    }
    if (crackedStoneBricks && crackedPolishedBlackstoneBricks) {
        m_replacements[crackedStoneBricks] = crackedPolishedBlackstoneBricks;
    }
    if (chiseledStoneBricks && chiseledPolishedBlackstone) {
        m_replacements[chiseledStoneBricks] = chiseledPolishedBlackstone;
    }
    if (ironBars && chain) {
        m_replacements[ironBars] = chain;
    }

    // 楼梯替换
    u32 cobblestoneStairs = getBlockId("minecraft:cobblestone_stairs");
    u32 blackstoneStairs = getBlockId("minecraft:blackstone_stairs");
    u32 mossyCobblestoneStairs = getBlockId("minecraft:mossy_cobblestone_stairs");
    u32 stoneStairs = getBlockId("minecraft:stone_stairs");
    u32 polishedBlackstoneStairs = getBlockId("minecraft:polished_blackstone_stairs");
    u32 stoneBrickStairs = getBlockId("minecraft:stone_brick_stairs");
    u32 polishedBlackstoneBrickStairs = getBlockId("minecraft:polished_blackstone_brick_stairs");
    u32 mossyStoneBrickStairs = getBlockId("minecraft:mossy_stone_brick_stairs");

    if (cobblestoneStairs && blackstoneStairs) {
        m_replacements[cobblestoneStairs] = blackstoneStairs;
    }
    if (mossyCobblestoneStairs && blackstoneStairs) {
        m_replacements[mossyCobblestoneStairs] = blackstoneStairs;
    }
    if (stoneStairs && polishedBlackstoneStairs) {
        m_replacements[stoneStairs] = polishedBlackstoneStairs;
    }
    if (stoneBrickStairs && polishedBlackstoneBrickStairs) {
        m_replacements[stoneBrickStairs] = polishedBlackstoneBrickStairs;
    }
    if (mossyStoneBrickStairs && polishedBlackstoneBrickStairs) {
        m_replacements[mossyStoneBrickStairs] = polishedBlackstoneBrickStairs;
    }

    // 台阶替换
    u32 cobblestoneSlab = getBlockId("minecraft:cobblestone_slab");
    u32 blackstoneSlab = getBlockId("minecraft:blackstone_slab");
    u32 mossyCobblestoneSlab = getBlockId("minecraft:mossy_cobblestone_slab");
    u32 smoothStoneSlab = getBlockId("minecraft:smooth_stone_slab");
    u32 stoneSlab = getBlockId("minecraft:stone_slab");
    u32 polishedBlackstoneSlab = getBlockId("minecraft:polished_blackstone_slab");
    u32 stoneBrickSlab = getBlockId("minecraft:stone_brick_slab");
    u32 polishedBlackstoneBrickSlab = getBlockId("minecraft:polished_blackstone_brick_slab");
    u32 mossyStoneBrickSlab = getBlockId("minecraft:mossy_stone_brick_slab");

    if (cobblestoneSlab && blackstoneSlab) {
        m_replacements[cobblestoneSlab] = blackstoneSlab;
    }
    if (mossyCobblestoneSlab && blackstoneSlab) {
        m_replacements[mossyCobblestoneSlab] = blackstoneSlab;
    }
    if (smoothStoneSlab && polishedBlackstoneSlab) {
        m_replacements[smoothStoneSlab] = polishedBlackstoneSlab;
    }
    if (stoneSlab && polishedBlackstoneSlab) {
        m_replacements[stoneSlab] = polishedBlackstoneSlab;
    }
    if (stoneBrickSlab && polishedBlackstoneBrickSlab) {
        m_replacements[stoneBrickSlab] = polishedBlackstoneBrickSlab;
    }
    if (mossyStoneBrickSlab && polishedBlackstoneBrickSlab) {
        m_replacements[mossyStoneBrickSlab] = polishedBlackstoneBrickSlab;
    }

    // 墙替换
    u32 cobblestoneWall = getBlockId("minecraft:cobblestone_wall");
    u32 blackstoneWall = getBlockId("minecraft:blackstone_wall");
    u32 mossyCobblestoneWall = getBlockId("minecraft:mossy_cobblestone_wall");
    u32 stoneBrickWall = getBlockId("minecraft:stone_brick_wall");
    u32 polishedBlackstoneBrickWall = getBlockId("minecraft:polished_blackstone_brick_wall");
    u32 mossyStoneBrickWall = getBlockId("minecraft:mossy_stone_brick_wall");

    if (cobblestoneWall && blackstoneWall) {
        m_replacements[cobblestoneWall] = blackstoneWall;
    }
    if (mossyCobblestoneWall && blackstoneWall) {
        m_replacements[mossyCobblestoneWall] = blackstoneWall;
    }
    if (stoneBrickWall && polishedBlackstoneBrickWall) {
        m_replacements[stoneBrickWall] = polishedBlackstoneBrickWall;
    }
    if (mossyStoneBrickWall && polishedBlackstoneBrickWall) {
        m_replacements[mossyStoneBrickWall] = polishedBlackstoneBrickWall;
    }
}

std::optional<ProcessedBlockInfo> BlackstoneReplacementProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    u32 blockId = state->blockId();

    // 查找替换映射
    auto it = m_replacements.find(blockId);
    if (it != m_replacements.end()) {
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;

        // 获取目标方块
        Block* targetBlock = BlockRegistry::instance().getBlock(it->second);
        if (targetBlock) {
            // 获取目标方块的默认状态
            const BlockState* targetState = &targetBlock->defaultState();
            const Block& sourceBlock = state->getBlock();
            const auto& sourceContainer = sourceBlock.stateContainer();
            const auto& targetContainer = targetBlock->stateContainer();

            // 保持兼容的方块状态属性

            // 尝试复制 FACING 属性（用于楼梯、墙等）
            const IProperty* facingProp = sourceContainer.getProperty("facing");
            const IProperty* targetFacingProp = targetContainer.getProperty("facing");
            if (facingProp && targetFacingProp) {
                auto valueIndex = state->getValueIndex(*facingProp);
                if (valueIndex.has_value()) {
                    // 尝试在目标方块上设置相同属性值
                    size_t sourceIndex = *valueIndex;
                    std::string valueStr = facingProp->valueToString(sourceIndex);
                    auto parsedValue = targetFacingProp->parseValue(valueStr);
                    if (parsedValue) {
                        // 遍历目标方块的所有状态，找到具有目标属性值的状态
                        for (const auto& candidate : targetContainer.validStates()) {
                            if (!candidate) continue;
                            auto targetValueIndex = candidate->getValueIndex(*targetFacingProp);
                            if (targetValueIndex.has_value() && *targetValueIndex == *parsedValue) {
                                targetState = candidate.get();
                                break;
                            }
                        }
                    }
                }
            }

            // 尝试复制 HALF 属性（用于楼梯）
            const IProperty* halfProp = sourceContainer.getProperty("half");
            const IProperty* targetHalfProp = targetContainer.getProperty("half");
            if (halfProp && targetHalfProp && targetState) {
                auto valueIndex = state->getValueIndex(*halfProp);
                if (valueIndex.has_value()) {
                    size_t sourceIndex = *valueIndex;
                    std::string valueStr = halfProp->valueToString(sourceIndex);
                    auto parsedValue = targetHalfProp->parseValue(valueStr);
                    if (parsedValue) {
                        for (const auto& candidate : targetContainer.validStates()) {
                            if (!candidate) continue;
                            // 检查是否保持 FACING 值
                            auto targetFacingValueIndex =
                                targetFacingProp ? candidate->getValueIndex(*targetFacingProp) : std::nullopt;
                            auto sourceFacingValueIndex =
                                targetFacingProp ? targetState->getValueIndex(*targetFacingProp) : std::nullopt;
                            bool facingMatches = (targetFacingProp == nullptr) ||
                                (targetFacingValueIndex.has_value() && sourceFacingValueIndex.has_value() &&
                                    *targetFacingValueIndex == *sourceFacingValueIndex);
                            if (!facingMatches) continue;

                            auto targetValueIndex = candidate->getValueIndex(*targetHalfProp);
                            if (targetValueIndex.has_value() && *targetValueIndex == *parsedValue) {
                                targetState = candidate.get();
                                break;
                            }
                        }
                    }
                }
            }

            // 尝试复制 TYPE 属性（用于台阶 - top/bottom/double）
            const IProperty* typeProp = sourceContainer.getProperty("type");
            const IProperty* targetTypeProp = targetContainer.getProperty("type");
            if (typeProp && targetTypeProp && targetState) {
                auto valueIndex = state->getValueIndex(*typeProp);
                if (valueIndex.has_value()) {
                    size_t sourceIndex = *valueIndex;
                    std::string valueStr = typeProp->valueToString(sourceIndex);
                    auto parsedValue = targetTypeProp->parseValue(valueStr);
                    if (parsedValue) {
                        for (const auto& candidate : targetContainer.validStates()) {
                            if (!candidate) continue;
                            auto targetValueIndex = candidate->getValueIndex(*targetTypeProp);
                            if (targetValueIndex.has_value() && *targetValueIndex == *parsedValue) {
                                targetState = candidate.get();
                                break;
                            }
                        }
                    }
                }
            }

            result.blockStateId = targetState ? targetState->stateId() : targetBlock->defaultState().stateId();
        } else {
            result.blockStateId = blockInfo.blockStateId;
        }

        // 复制 NBT（如果有）
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }

        return result;
    }

    // 没有找到替换映射，保持原样
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
