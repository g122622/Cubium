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

#include "RuleStructureProcessor.hpp"

#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

RuleStructureProcessor::RuleStructureProcessor(std::vector<std::unique_ptr<ruletest::RuleEntry>> rules)
    : m_rules(std::move(rules))
{}

std::optional<ProcessedBlockInfo> RuleStructureProcessor::process(const BlockPos& seedPos,
    const BlockPos& /*pos*/,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // 遍历所有规则，找到第一个匹配的规则
    // 使用位置种子创建确定性随机数生成器
    u64 seed = math::getPositionRandom(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(seed);

    // 获取输入方块状态
    const BlockState* inputStatePtr = BlockRegistry::instance().getBlockState(rawBlockInfo.blockStateId);
    if (!inputStatePtr) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 获取世界位置方块状态（通过 PlacementSettings 中的世界访问）
    // locationState 可以为空（世界中该位置没有方块）
    const BlockState* locationStatePtr = nullptr;
    const IWorld* world = settings.getWorld();
    if (world) {
        locationStatePtr = world->getBlockState(blockInfo.pos);
    }

    // 需要为 locationState 提供有效的引用
    // 如果世界中没有方块状态，使用空气作为默认值
    static const BlockState* airState = BlockRegistry::instance().getBlockState(0);
    const BlockState& locationState = locationStatePtr ? *locationStatePtr : (airState ? *airState : *inputStatePtr);

    for (const auto& rule : m_rules) {
        if (rule && rule->matches(*inputStatePtr, locationState, rawBlockInfo.pos, blockInfo.pos, seedPos, rng)) {
            // 找到匹配的规则，返回输出方块状态
            ProcessedBlockInfo result;
            result.pos = blockInfo.pos;
            result.blockStateId = rule->outputStateId();
            // 不复制 NBT（规则输出不保留原 NBT）
            return result;
        }
    }

    // 没有规则匹配，保持原样
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

std::unique_ptr<StructureProcessor> RuleStructureProcessor::clone() const
{
    std::vector<std::unique_ptr<ruletest::RuleEntry>> clonedRules;
    clonedRules.reserve(m_rules.size());
    for (const auto& rule : m_rules) {
        if (rule) {
            clonedRules.push_back(rule->clone());
        }
    }
    return std::make_unique<RuleStructureProcessor>(std::move(clonedRules));
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
