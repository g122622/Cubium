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

#include "GravityStructureProcessor.hpp"

#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

GravityStructureProcessor::GravityStructureProcessor(i32 heightmapType, i32 offset)
    : m_heightmapType(heightmapType)
    , m_offset(offset)
{}

std::optional<ProcessedBlockInfo> GravityStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // 根据高度图调整 Y 坐标
    // 如果有世界访问，则获取地面高度；否则仅应用偏移量
    const IWorld* world = settings.getWorld();

    ProcessedBlockInfo result = ProcessedBlockInfo::fromBlockInfo(blockInfo);

    if (world) {
        // 完整实现：使用高度图获取地面高度
        i32 surfaceY = world->getHeight(blockInfo.pos.x, blockInfo.pos.z);
        result.pos = BlockPos(blockInfo.pos.x, surfaceY + m_offset, blockInfo.pos.z);
    } else {
        // 简化实现：仅应用偏移量
        result.pos = BlockPos(blockInfo.pos.x, blockInfo.pos.y + m_offset, blockInfo.pos.z);
    }

    return result;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
