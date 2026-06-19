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

#include "StructureProcessor.hpp"
#include "Template.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// StructureProcessor
// ============================================================================

std::optional<ProcessedBlockInfo> StructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 默认实现：不修改，直接返回处理后的方块信息
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

// ============================================================================
// StructureProcessorList
// ============================================================================

void StructureProcessorList::addProcessor(std::unique_ptr<StructureProcessor> processor)
{
    m_processors.push_back(std::move(processor));
}

std::optional<ProcessedBlockInfo> StructureProcessorList::process(const BlockPos& seedPos,
    const BlockPos& pos,
    const BlockInfo& rawBlockInfo,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings) const
{
    // 如果没有处理器，直接返回原始信息
    if (m_processors.empty()) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 按顺序处理
    ProcessedBlockInfo current = ProcessedBlockInfo::fromBlockInfo(blockInfo);

    for (const auto& processor : m_processors) {
        if (!processor) continue;

        // 创建临时BlockInfo用于处理
        BlockInfo currentInfo;
        currentInfo.pos = current.pos;
        currentInfo.blockStateId = current.blockStateId;
        if (current.nbt) {
            currentInfo.nbt = std::make_unique<nbt::CompoundTag>(*current.nbt);
        }

        auto result = processor->process(seedPos, pos, rawBlockInfo, currentInfo, settings);
        if (!result) {
            // 处理器返回nullopt，跳过此方块
            return std::nullopt;
        }
        current = std::move(*result);
    }

    return current;
}

std::unique_ptr<StructureProcessorList> StructureProcessorList::clone() const
{
    auto list = std::make_unique<StructureProcessorList>();
    for (const auto& proc : m_processors) {
        if (proc) {
            list->addProcessor(proc->clone());
        }
    }
    return list;
}

// ============================================================================
// ProcessorLists
// ============================================================================

namespace ProcessorLists {

static std::unique_ptr<StructureProcessorList> s_emptyList;

const StructureProcessorList& empty()
{
    if (!s_emptyList) {
        s_emptyList = std::make_unique<StructureProcessorList>();
    }
    return *s_emptyList;
}

} // namespace ProcessorLists

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
