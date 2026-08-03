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

#pragma once

#include "StructureProcessor.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <memory>
#include <optional>
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 黑石替换结构处理器
 * 将普通石质方块替换为黑石变体，用于堡垒遗迹的结构生成。
 * 替换映射：
 * - 圆石 -> 黑石
 * - 石头 -> 磨制黑石
 * - 石砖 -> 磨制黑石砖
 * - 等...
 */
class BlackstoneReplacementProcessor : public StructureProcessor {
public:
    BlackstoneReplacementProcessor();

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<BlackstoneReplacementProcessor>();
    }

private:
    // 方块替换映射表：输入方块ID -> 输出方块ID
    std::unordered_map<u32, u32> m_replacements;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
