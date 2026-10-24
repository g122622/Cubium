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

#include "BlockTagLoader.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/tag/GenericTagLoader.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mc {

namespace {

/// 方块标签目录（相对 data/<namespace>/）
constexpr std::string_view BLOCK_TAG_DIRECTORY = "tags/block";

/**
 * @brief 方块资源位置池
 *
 * 通用加载器以 size_t 索引传递成员。本池把 ResourceLocation 映射为稠密索引，
 * 使内置标签成员（由 BlockTags::initialize 直接以 ResourceLocation 注册）与
 * 数据包成员共用同一索引空间。
 */
class _BlockMemberPool {
public:
    /**
     * @brief 获取或创建资源位置对应的索引
     *
     * 内置标签成员不经 resolveMember 加入，故 readTag 也须经此函数按需登记，
     * 保证任意已存在成员都能取到索引。
     */
    std::size_t getOrCreate(const ResourceLocation& location)
    {
        auto it = m_indexByLocation.find(location);
        if (it != m_indexByLocation.end()) {
            return it->second;
        }
        std::size_t index = m_locations.size();
        m_locations.push_back(location);
        m_indexByLocation.emplace(location, index);
        return index;
    }

    [[nodiscard]] const ResourceLocation& at(std::size_t index) const
    {
        return m_locations.at(index);
    }

private:
    std::vector<ResourceLocation> m_locations;
    std::unordered_map<ResourceLocation, std::size_t> m_indexByLocation;
};

/// 记录 fillTag 期间被显式引用的数据包标签 ID（解析时 lastFilledTag 指向当前标签）
struct _ReferenceCollector {
    std::optional<ResourceLocation> lastFilledTag;
    std::vector<ResourceLocation> referencedTags;
};

/**
 * @brief 显式保留标签间引用（#tag 形式）
 *
 * GenericTagLoader 会把 #tag 引用展开为成员并写入 fillTag，导致原始引用关系丢失
 * （minecraft:tool 等按 HolderSet 判断"某个标签是否被包含"时无法判断，
 * 例如剑标签包含 minecraft:swords 而非其全部成员）。在填充完成后按记录恢复引用。
 */
void _restoreTagReferences(const _ReferenceCollector& collector)
{
    for (const auto& tagId : collector.referencedTags) {
        BlockTag* source = BlockTags::getTag(tagId);
        BlockTag* target = collector.lastFilledTag.has_value() ? BlockTags::getTag(*collector.lastFilledTag) : nullptr;
        if (source != nullptr && target != nullptr && source != target) {
            target->addTagReference(tagId);
        }
    }
}

/// 构建三个回调，绑定到同一资源位置池
void _buildCallbacks(_BlockMemberPool& pool,
    _ReferenceCollector& collector,
    resource::tag::TagMemberResolver& resolveMember,
    resource::tag::TagMemberReader& readTag,
    resource::tag::TagFiller& fillTag)
{
    resolveMember = [&pool](std::string_view memberId) -> std::size_t {
        ResourceLocation location = ResourceLocation::parse(memberId);
        if (BlockRegistry::instance().getBlock(location) == nullptr) {
            return resource::tag::TAG_MEMBER_NOT_FOUND;
        }
        return pool.getOrCreate(location);
    };

    readTag = [&pool, &collector](const ResourceLocation& tagId) -> std::vector<std::size_t> {
        std::vector<std::size_t> members;
        auto* tag = BlockTags::getTag(tagId);
        if (tag == nullptr) {
            return members;
        }
        for (const auto& blockId : tag->getBlockIds()) {
            members.push_back(pool.getOrCreate(blockId));
        }
        collector.referencedTags.push_back(tagId);
        return members;
    };

    fillTag = [&pool, &collector](
                  const ResourceLocation& tagId, const std::vector<std::size_t>& members, bool replace) {
        BlockTag& tag = BlockTags::registerTag(tagId);
        if (replace) {
            tag.clear();
        }
        for (std::size_t index : members) {
            tag.add(pool.at(index));
        }
        collector.lastFilledTag = tagId;
        _restoreTagReferences(collector);
        collector.referencedTags.clear();
    };
}

} // namespace

Result<size_t> BlockTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    _BlockMemberPool pool;
    _ReferenceCollector collector;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, collector, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromDataPackRepository(
        dataPackList, BLOCK_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

Result<size_t> BlockTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    _BlockMemberPool pool;
    _ReferenceCollector collector;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, collector, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromResourcePack(
        pack, BLOCK_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

} // namespace mc
