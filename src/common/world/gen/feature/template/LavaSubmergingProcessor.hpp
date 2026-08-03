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
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <memory>
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 岩浆淹没结构处理器
 * 当结构放置在岩浆中时，如果方块是非固体（非不透明），则将其替换为岩浆。
 * 用于堡垒遗迹等在下界岩浆海中生成的结构。
 */
class LavaSubmergingProcessor : public StructureProcessor {
public:
    LavaSubmergingProcessor() = default;

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<LavaSubmergingProcessor>();
    }
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
