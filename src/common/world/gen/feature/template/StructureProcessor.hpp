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

#include "BlockInfo.hpp"
#include "PlacementSettings.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 结构处理器接口
 * 处理器可以修改或过滤模板中的方块
 */
class StructureProcessor {
public:
    virtual ~StructureProcessor() = default;

    /**
     * @brief 处理方块信息
     * @param seedPos 种子位置
     * @param pos 当前放置位置
     * @param rawBlockInfo 原始方块信息（未变换）
     * @param blockInfo 变换后的方块信息
     * @param settings 放置设置
     * @return 处理后的方块信息，或 nullopt 表示跳过此方块
     */
    [[nodiscard]] virtual std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings);

    /**
     * @brief 后处理阶段：在所有方块的 process() 调用完成后，对完整方块列表进行批量处理
     *
     * 此方法在所有方块逐一经过 process() 处理完毕后被调用，允许处理器对完整列表做
     * 跨方块批量操作（如 CappedStructureProcessor 限制替换次数）。
     * 默认实现直接返回 processedBlocks，不做任何修改。
     *
     * @param seedPos 结构放置的锚点位置
     * @param settings 放置设置
     * @param originalBlocks 原始方块信息列表（变换坐标前，对应模板内坐标和状态）
     * @param processedBlocks 经处理器链处理后的方块信息列表，可直接修改
     * @return 处理后的方块信息列表（通常返回修改后的 processedBlocks）
     */
    [[nodiscard]] virtual std::vector<ProcessedBlockInfo> finalizeProcessing(const BlockPos& seedPos,
        const PlacementSettings& settings,
        const std::vector<BlockInfo>& originalBlocks,
        std::vector<ProcessedBlockInfo> processedBlocks);

    /**
     * @brief 克隆处理器
     *
     * 用于在放置链路中组合多个处理器列表时深拷贝处理器。
     */
    [[nodiscard]] virtual std::unique_ptr<StructureProcessor> clone() const = 0;

    /**
     * @brief 获取处理器类型ID
     */
    [[nodiscard]] virtual u32 getProcessorType() const { return 0; }
};

/**
 * @brief 结构处理器列表
 */
class StructureProcessorList {
public:
    StructureProcessorList() = default;

    void addProcessor(std::unique_ptr<StructureProcessor> processor);
    [[nodiscard]] size_t size() const { return m_processors.size(); }
    [[nodiscard]] bool empty() const { return m_processors.empty(); }

    /**
     * @brief 深拷贝处理器列表
     *
     * 用于在放置链路中组合多个处理器列表时创建独立副本，
     * 避免修改原始注册表中的处理器。
     */
    [[nodiscard]] std::unique_ptr<StructureProcessorList> clone() const;

    /**
     * @brief 获取处理器列表的只读引用
     *
     * 用于遍历处理器进行克隆或查询。
     */
    [[nodiscard]] const std::vector<std::unique_ptr<StructureProcessor>>& getProcessors() const { return m_processors; }

    // 迭代器支持
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::const_iterator begin() const
    {
        return m_processors.begin();
    }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::const_iterator end() const
    {
        return m_processors.end();
    }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::iterator begin() { return m_processors.begin(); }
    [[nodiscard]] std::vector<std::unique_ptr<StructureProcessor>>::iterator end() { return m_processors.end(); }

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) const;

private:
    std::vector<std::unique_ptr<StructureProcessor>> m_processors;
};

/**
 * @brief 预定义处理器列表
 */
namespace ProcessorLists {
[[nodiscard]] const StructureProcessorList& empty();
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
