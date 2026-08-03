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
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief Jigsaw 替换结构处理器
 *
 * 当遇到 Jigsaw 方块时，读取其 final_state NBT 字段，
 * 解析并替换为对应的方块状态。如果 final_state 是 structure_void，则跳过此方块。
 */
class JigsawReplacementStructureProcessor : public StructureProcessor {
public:
    JigsawReplacementStructureProcessor();

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<JigsawReplacementStructureProcessor>();
    }

private:
    /**
     * @brief 解析方块状态字符串
     * @param stateStr 方块状态字符串，如 "minecraft:stone[axis=y]"
     * @return 方块状态 ID，失败返回 0（空气）
     */
    [[nodiscard]] static u32 parseBlockStateString(const std::string& stateStr);
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
