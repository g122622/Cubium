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

#include "BlockIgnoreStructureProcessor.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

BlockIgnoreStructureProcessor::BlockIgnoreStructureProcessor(const std::vector<u32>& blocksToIgnore)
    : m_blocksToIgnore(blocksToIgnore.begin(), blocksToIgnore.end())
{}

std::optional<ProcessedBlockInfo> BlockIgnoreStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 使用 unordered_set 进行 O(1) 查找
    if (m_blocksToIgnore.count(blockInfo.blockStateId) > 0) {
        return std::nullopt; // 跳过此方块
    }

    // 保留方块
    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
