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

#include "CappedStructureProcessor.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/valueprovider/IntProvider.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

CappedStructureProcessor::CappedStructureProcessor(
    std::unique_ptr<StructureProcessor> delegate, std::unique_ptr<valueprovider::IntProvider> limitProvider)
    : m_delegate(std::move(delegate))
    , m_limitProvider(std::move(limitProvider))
{
    // limitProvider 为空时使用 ConstantInt(0)，确保安全
    if (!m_limitProvider) {
        m_limitProvider = std::make_unique<valueprovider::ConstantInt>(0);
    }
}

CappedStructureProcessor::CappedStructureProcessor(std::unique_ptr<StructureProcessor> delegate, i32 limit)
    : m_delegate(std::move(delegate))
    , m_limitProvider(std::make_unique<valueprovider::ConstantInt>(limit < 0 ? 0 : limit))
{}

std::optional<ProcessedBlockInfo> CappedStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // CappedProcessor 在逐方块 process 阶段不做任何修改，直接透传。
    // 实际的限制替换逻辑在 finalizeProcessing 中执行。
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

std::vector<ProcessedBlockInfo> CappedStructureProcessor::finalizeProcessing(const BlockPos& seedPos,
    const PlacementSettings& settings,
    const std::vector<BlockInfo>& originalBlocks,
    std::vector<ProcessedBlockInfo> processedBlocks)
{
    // 快速退出：delegate 为空、limitProvider 最大值为 0 或列表为空
    if (!m_delegate || m_limitProvider->getMaxValue() == 0 || processedBlocks.empty()) {
        return processedBlocks;
    }

    // 安全校验：原始列表和处理后列表大小必须一致
    if (originalBlocks.size() != processedBlocks.size()) {
        return processedBlocks;
    }

    // 随机源口径：`RandomSource.create(level.getSeed()).forkPositional().at(模板原点)`。
    // 它决定「哪几个命中方块被转换」，与结构随机源、位置哈希都不同；换成别的随机源会让
    // 转换落点整体漂移（海洋废墟的可疑沙砾即由此对齐）。世界缺失只可能出现在单元测试里，
    // 此时种子退化到 0。
    const IWorld* level = settings.getWorld();
    math::JavaLegacyRandom levelRandom(level != nullptr ? level->seed() : 0);
    math::PositionalRandomFactory factory = levelRandom.forkPositional();
    std::unique_ptr<math::IRandom> rng = factory.at(seedPos.x, seedPos.y, seedPos.z);

    // 采样 IntProvider 得到实际限制次数
    i32 effectiveLimit = std::min(m_limitProvider->sample(*rng), static_cast<i32>(processedBlocks.size()));
    if (effectiveLimit < 1) {
        return processedBlocks;
    }

    // 生成打乱的索引序列：反向 Fisher-Yates（`j` 从 size 递减到 2，`nextInt(j)` 与
    // `j-1` 交换），与 `Util.toShuffledList(IntStream, RandomSource)` 完全一致。
    std::vector<i32> indices(processedBlocks.size());
    std::iota(indices.begin(), indices.end(), 0);
    for (i32 j = static_cast<i32>(indices.size()); j > 1; --j) {
        const i32 k = rng->nextInt(j);
        std::swap(indices[static_cast<size_t>(j - 1)], indices[static_cast<size_t>(k)]);
    }

    // 遍历打乱后的索引，调用 delegate.process() 尝试替换
    i32 successCount = 0;
    for (i32 idx : indices) {
        if (successCount >= effectiveLimit) {
            break;
        }

        const BlockInfo& rawInfo = originalBlocks[static_cast<size_t>(idx)];
        const ProcessedBlockInfo& currentProcessed = processedBlocks[static_cast<size_t>(idx)];

        // 构造 BlockInfo 用于传递给 delegate.process()
        BlockInfo currentInfo(currentProcessed.pos, currentProcessed.blockStateId);
        if (currentProcessed.nbt) {
            currentInfo.nbt = std::make_unique<nbt::CompoundTag>(*currentProcessed.nbt);
        }

        // 调用委托处理器的 process 方法
        auto result = m_delegate->process(seedPos, currentProcessed.pos, rawInfo, currentInfo, settings);

        // 检查委托处理器是否实际改变了方块
        // 只有当 delegate 返回的结果与当前处理后方块不等时，才计为成功替换
        if (result.has_value()) {
            bool changed = false;
            if (result->blockStateId != currentProcessed.blockStateId) {
                changed = true;
            } else if (result->pos != currentProcessed.pos) {
                changed = true;
            } else if (result->nbt && !currentProcessed.nbt) {
                changed = true;
            } else if (!result->nbt && currentProcessed.nbt) {
                changed = true;
            } else if (result->nbt && currentProcessed.nbt) {
                // 两者都有 NBT，深度比较 NBT 内容是否不同
                if (!result->nbt->equals(*currentProcessed.nbt)) {
                    changed = true;
                }
            }

            if (changed) {
                ++successCount;
                // 更新处理后的方块信息
                processedBlocks[static_cast<size_t>(idx)] = std::move(*result);
            }
        }
        // 如果 delegate 返回 nullopt，表示方块应被移除，不计入成功替换次数
        // 但按照 MC 原版逻辑，nullopt 不增加计数器，索引已消耗
    }

    return processedBlocks;
}

std::unique_ptr<StructureProcessor> CappedStructureProcessor::clone() const
{
    auto clonedDelegate = m_delegate ? m_delegate->clone() : nullptr;
    auto clonedLimit = m_limitProvider ? m_limitProvider->clone() : nullptr;
    return std::make_unique<CappedStructureProcessor>(std::move(clonedDelegate), std::move(clonedLimit));
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
