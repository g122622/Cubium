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

#pragma once

#include "StructureProcessor.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 方块老化结构处理器（苔藓化处理器）
 * 根据苔藓概率随机将石砖相关方块替换为苔藓化或裂变版本。
 * 用于村庄等结构的老化效果。
 */
class BlockAgeProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造方块老化处理器
     * @param mossiness 苔藓化概率 (0.0 - 1.0)
     */
    explicit BlockAgeProcessor(f32 mossiness);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<BlockAgeProcessor>(m_mossiness);
    }

    [[nodiscard]] f32 mossiness() const { return m_mossiness; }

private:
    f32 m_mossiness;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
