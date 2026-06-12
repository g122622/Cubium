/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the new condition that the following
 * conditions are met:
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

#include "CopperBulbDegradationProcessor.hpp"

#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

CopperBulbDegradationProcessor::CopperBulbDegradationProcessor() = default;

i32 CopperBulbDegradationProcessor::getOxidationLevel(i32 x, i32 y, i32 z)
{
    // TODO: 当前实现使用位置哈希均匀映射到0-3四个氧化等级（各25%概率），
    // 而MC原版使用数据驱动的 trial_chambers_copper_bulb_degradation 结构处理器规则，
    // 通过JSON配置文件指定每个氧化等级的权重概率，而非均匀分布。
    // 当前实现仅匹配 WAXED_COPPER_BULB 一种基础方块，
    // 原版处理器可根据配置匹配任意铜灯变体并应用加权随机选择。
    // 应改为从数据包加载 trial_chambers_copper_bulb_degradation 配置，
    // 使用加权随机而非均匀哈希来决定氧化等级。
    u64 hash = math::hashBlockPos(x, y, z);
    return static_cast<i32>((hash >> 16) % 4);
}

std::optional<ProcessedBlockInfo> CopperBulbDegradationProcessor::process(const BlockPos& seedPos,
    const BlockPos& pos,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    MC_UNUSED(seedPos);
    MC_UNUSED(rawBlockInfo);
    MC_UNUSED(settings);

    // 通过 blockStateId 获取方块
    BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (state == nullptr) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    const Block& block = state->getBlock();

    // 检查是否为涂蜡铜灯 - 只对涂蜡铜灯进行降级
    // 在试炼密室中，结构模板使用的是涂蜡铜灯，
    // 处理器根据位置哈希将其替换为不同氧化等级的涂蜡铜灯
    Block* replacementBlock = nullptr;

    if (&block == VanillaBlocks::WAXED_COPPER_BULB) {
        i32 level = getOxidationLevel(pos.x, pos.y, pos.z);
        switch (level) {
            case 0:
                replacementBlock = VanillaBlocks::WAXED_COPPER_BULB;
                break;
            case 1:
                replacementBlock = VanillaBlocks::WAXED_EXPOSED_COPPER_BULB;
                break;
            case 2:
                replacementBlock = VanillaBlocks::WAXED_WEATHERED_COPPER_BULB;
                break;
            case 3:
                replacementBlock = VanillaBlocks::WAXED_OXIDIZED_COPPER_BULB;
                break;
            default:
                replacementBlock = VanillaBlocks::WAXED_COPPER_BULB;
                break;
        }
    }

    // 如果没有匹配到需要降级的方块，返回原始方块
    if (replacementBlock == nullptr) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 如果替换方块与原方块相同，无需替换
    if (replacementBlock == &block) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 使用替换方块的默认状态的 blockStateId
    const BlockState& replacementState = replacementBlock->defaultState();
    ProcessedBlockInfo result;
    result.pos = blockInfo.pos;
    result.blockStateId = replacementState.stateId();
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
