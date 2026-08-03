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

#include "IntegrityProcessor.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

IntegrityProcessor::IntegrityProcessor(f32 integrity)
    : m_integrity(integrity)
{}

std::optional<ProcessedBlockInfo> IntegrityProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 使用位置种子创建确定性随机数生成器
    // 关键：使用变换后的世界坐标 (blockInfo.pos)，而非模板内坐标
    u64 seed = math::getPositionRandom(blockInfo.pos.x, blockInfo.pos.y, blockInfo.pos.z);
    math::Random rng(seed);

    // 完整度 >= 1.0：保留所有方块
    if (m_integrity >= 1.0f) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 随机判断是否保留方块
    // nextFloat() 返回 [0.0, 1.0)，所以 <= integrity 的概率正好是 integrity
    if (rng.nextFloat() <= m_integrity) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 移除方块
    return std::nullopt;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
