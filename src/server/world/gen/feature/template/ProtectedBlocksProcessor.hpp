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

#include "Template.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 受保护方块结构处理器
 *
 * 基于 BlockTag 标签保护世界中原有的方块不被模板结构覆盖。
 *
 * 处理逻辑（对应 MC Java 1.21.11 ProtectedBlockProcessor）：
 * - 读取目标位置当前世界方块状态
 * - 若该方块在保护标签内 → 返回 nullopt（跳过放置，保留世界方块）
 * - 若该方块不在保护标签内 → 返回模板方块信息（正常放置）
 *
 * 等价于 MC 中：
 *   Feature.isReplaceable(cannotReplace).test(world.getBlockState(pos)) ? blockInfo : null
 * 其中 isReplaceable(tag) = !state.is(tag)
 *
 * JSON 格式：
 *   { "processor_type": "minecraft:protected_blocks", "value": "#minecraft:features_cannot_replace" }
 *
 * 典型用例：远古城市、堡垒遗迹等结构生成时，保护基岩、刷怪笼、箱子等关键方块
 * 不被结构模板覆盖。对应数据包标签 #minecraft:features_cannot_replace。
 *
 * 注意：本处理器依赖 PlacementSettings::getWorld() 提供的世界读取器。
 * 若未设置世界（getWorld() == nullptr），则不做任何过滤，直接透传模板方块。
 */
class ProtectedBlocksProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造受保护方块处理器
     *
     * @param tagId 保护标签的资源位置（如 "minecraft:features_cannot_replace"）
     *
     * 注意：构造时不校验标签是否存在（标签可能在本处理器构造之后才注册），
     * 标签查找延迟到 process() 调用时进行。若运行时标签不存在，
     * 则视为空标签（不保护任何方块），所有模板方块均正常放置。
     */
    explicit ProtectedBlocksProcessor(ResourceLocation tagId);

    /**
     * @brief 处理方块信息
     *
     * 读取 world->getBlockState(blockInfo.pos) 当前世界方块状态：
     * - 若世界方块在保护标签内 → 返回 nullopt（跳过放置）
     * - 否则 → 返回原始 blockInfo（正常放置）
     *
     * 边界情况：
     * - 若 settings.getWorld() == nullptr → 直接透传 blockInfo
     * - 若 world->getBlockState() 返回 nullptr → 直接透传 blockInfo
     * - 若保护标签不存在（BlockTags::getTag 返回 nullptr）→ 直接透传 blockInfo
     */
    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    /**
     * @brief 克隆处理器
     *
     * 用于在放置链路中组合多个处理器列表时深拷贝处理器。
     */
    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<ProtectedBlocksProcessor>(m_tagId);
    }

    /**
     * @brief 获取保护标签的资源位置
     */
    [[nodiscard]] const ResourceLocation& getTagId() const noexcept { return m_tagId; }

private:
    /// 保护标签的资源位置（如 "minecraft:features_cannot_replace"）
    ResourceLocation m_tagId;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
