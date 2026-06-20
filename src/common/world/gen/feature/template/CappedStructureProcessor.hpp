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
 * LIABILITY, WHATSOEVER, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "Template.hpp"
#include <algorithm>
#include <numeric>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 限制次数的结构处理器（Capped Processor）
 *
 * 包装一个委托处理器（delegate），限制其在单次模板放置中最多被成功应用的次数。
 * 设计原理：
 * - process() 阶段不参与处理（直接透传），委托处理器也不被调用
 * - finalizeProcessing() 阶段介入：随机选取方块位置，对这些位置调用
 *   delegate.process()，并严格限制成功替换的次数不超过 limit
 *
 * JSON 格式：
 *   { "processor_type": "minecraft:capped", "delegate": {...}, "limit": 4 }
 *
 * 其中 delegate 是嵌套的处理器定义，limit 是最大应用次数。
 * TODO: MC 原版支持 IntProvider 随机范围作为 limit（如 uniform 分布），
 *       当前实现仅支持固定整数值，未来需要实现 IntProvider 支持。
 *
 * 典型用例：远古遗迹（Ocean Ruins）、古迹废墟（Trail Ruins）等结构中，
 * 限制某种替换规则（如苔藓化、箱子替换等）最多只生效 N 次，
 * 避免整个结构被过度修改。
 */
class CappedStructureProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造 CappedStructureProcessor
     * @param delegate 被包装的委托处理器，实际执行方块替换逻辑
     * @param limit 最大成功应用次数（>= 0，0 表示不替换任何方块）
     */
    CappedStructureProcessor(std::unique_ptr<StructureProcessor> delegate, i32 limit);

    /**
     * @brief process 阶段不参与处理，直接透传
     *
     * CappedProcessor 在逐方块 process 阶段不做任何修改，
     * 而是在 finalizeProcessing 阶段统一处理。
     */
    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    /**
     * @brief 后处理阶段：随机选取方块位置，限制委托处理器的成功替换次数
     *
     * 算法流程：
     * 1. 使用确定性随机源（基于结构放置位置的哈希）打乱方块索引顺序
     * 2. 对每个随机选取的索引，调用 delegate.process() 尝试替换
     * 3. 只有当 delegate 返回的结果与当前处理后方块不同时，才计为一次成功替换
     * 4. 成功替换次数达到 limit 后停止
     */
    [[nodiscard]] std::vector<ProcessedBlockInfo> finalizeProcessing(const BlockPos& seedPos,
        const PlacementSettings& settings,
        const std::vector<BlockInfo>& originalBlocks,
        std::vector<ProcessedBlockInfo> processedBlocks) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override;

    [[nodiscard]] const StructureProcessor* getDelegate() const { return m_delegate.get(); }
    [[nodiscard]] i32 getLimit() const { return m_limit; }

private:
    std::unique_ptr<StructureProcessor> m_delegate;
    i32 m_limit;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
