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

#include "InstrumentTagLoader.hpp"

#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/item/instrument/InstrumentTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/tag/GenericTagLoader.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace mc::server::item {
namespace {

/// 乐器标签目录（相对 data/<namespace>/）
constexpr std::string_view INSTRUMENT_TAG_DIRECTORY = "tags/instrument";

/**
 * @brief 乐器成员索引池
 *
 * 通用标签加载器以 size_t 索引传递成员；本池把乐器资源名映射为稠密索引，
 * 使内置标签（直接以资源名登记）与数据包成员共用同一索引空间。
 */
class _InstrumentMemberPool {
public:
    std::size_t getOrCreate(const ResourceLocation& instrumentId)
    {
        const auto it = m_indexById.find(instrumentId);
        if (it != m_indexById.end()) {
            return it->second;
        }
        const std::size_t index = m_instruments.size();
        m_instruments.push_back(instrumentId);
        m_indexById.emplace(instrumentId, index);
        return index;
    }

    [[nodiscard]] const ResourceLocation& at(std::size_t index) const
    {
        return m_instruments.at(index);
    }

private:
    std::vector<ResourceLocation> m_instruments;
    std::unordered_map<ResourceLocation, std::size_t> m_indexById;
};

void _buildCallbacks(_InstrumentMemberPool& pool,
    resource::tag::TagMemberResolver& resolveMember,
    resource::tag::TagMemberReader& readTag,
    resource::tag::TagFiller& fillTag)
{
    resolveMember = [&pool](std::string_view memberId) -> std::size_t {
        // 成员必须是已注册的乐器（对应 vanilla RegistryFileCodec 解析 Holder 失败）
        const ResourceLocation instrumentId{std::string(memberId)};
        if (mc::item::instrument::InstrumentRegistry::instance().get(instrumentId) == nullptr) {
            return resource::tag::TAG_MEMBER_NOT_FOUND;
        }
        return pool.getOrCreate(instrumentId);
    };
    readTag = [&pool](const ResourceLocation& tagId) -> std::vector<std::size_t> {
        const auto* tag = mc::item::instrument::InstrumentTags::instance().getTag(tagId);
        if (tag == nullptr) {
            return {};
        }
        std::vector<std::size_t> members;
        members.reserve(tag->size());
        for (const auto& memberId : tag->members()) {
            members.push_back(pool.getOrCreate(memberId));
        }
        return members;
    };
    fillTag = [&pool](const ResourceLocation& tagId, const std::vector<std::size_t>& members, bool replace) {
        auto& tag = mc::item::instrument::InstrumentTags::instance().registerTag(tagId);
        if (replace) {
            tag.clear();
        }
        tag.setReplace(replace);
        for (const std::size_t index : members) {
            tag.add(pool.at(index));
        }
    };
}

} // namespace

Result<size_t> InstrumentTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    _InstrumentMemberPool pool;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromDataPackRepository(
        dataPackList, INSTRUMENT_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

Result<size_t> InstrumentTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    _InstrumentMemberPool pool;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromResourcePack(
        pack, INSTRUMENT_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

} // namespace mc::server::item