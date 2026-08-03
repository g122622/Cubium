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

#include "ProtectedBlocksProcessor.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include <optional>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

ProtectedBlocksProcessor::ProtectedBlocksProcessor(ResourceLocation tagId)
    : m_tagId(std::move(tagId))
{}

std::optional<ProcessedBlockInfo> ProtectedBlocksProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& settings)
{
    // 边界情况1：未设置世界读取器，无法判断保护标签，直接透传
    const IWorld* world = settings.getWorld();
    if (world == nullptr) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 边界情况2：标签不存在（可能尚未注册），视为空标签，直接透传
    // BlockTags::getTag 在标签未注册时返回 nullptr，与 MC 中 TagKey 解析失败的语义一致
    BlockTag* tag = BlockTags::getTag(m_tagId);
    if (tag == nullptr) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 读取目标位置当前世界方块状态
    // 若当前位置区块未加载或坐标无效，getBlockState 返回 nullptr，直接透传
    const BlockState* worldState = world->getBlockState(blockInfo.pos);
    if (worldState == nullptr) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 核心逻辑（对应 MC Java 1.21.11 ProtectedBlockProcessor#processBlock）：
    //   Feature.isReplaceable(cannotReplace).test(worldState) ? blockInfo : null
    // 其中 isReplaceable(tag) = !state.is(tag)
    // 即：若世界方块在保护标签内 → 返回 nullopt（跳过放置，保留世界方块）
    //     若世界方块不在保护标签内 → 返回 blockInfo（正常放置）
    if (tag->contains(*worldState)) {
        return std::nullopt;
    }

    return ProcessedBlockInfo::fromBlockInfo(blockInfo);
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
