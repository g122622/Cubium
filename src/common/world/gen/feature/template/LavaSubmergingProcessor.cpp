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

#include "LavaSubmergingProcessor.hpp"

#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

std::optional<ProcessedBlockInfo> LavaSubmergingProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // 如果当前位置是岩浆，且模板方块不透明，则替换为岩浆
    const IWorld* world = settings.getWorld();
    if (!world) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 获取当前位置的方块状态
    const BlockState* worldState = world->getBlockState(blockInfo.pos);
    if (!worldState) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 检查当前位置是否是岩浆
    // 方块ID: 岩浆 (flowing_lava = 10, lava = 11)
    u32 worldBlockId = worldState->blockId();
    if (worldBlockId != 10 && worldBlockId != 11) {
        // 不是岩浆，保持原样
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 获取模板方块状态
    const BlockState* templateState = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!templateState) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 检查模板方块是否不透明
    // 如果不透明，则岩浆应该被替换为该方块；如果透明，则放置岩浆
    bool isOpaque = templateState->isOpaque();

    if (isOpaque) {
        // 不透明方块，岩浆应该被替换为该方块
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 透明方块（如栅栏、楼梯等），让岩浆保留
    // 返回岩浆状态
    ProcessedBlockInfo result;
    result.pos = blockInfo.pos;
    // 使用静止岩浆 (ID = 11)
    result.blockStateId = 11; // lava
    return result;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
