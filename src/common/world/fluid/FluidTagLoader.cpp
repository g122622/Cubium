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

#include "FluidTagLoader.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/tag/GenericTagLoader.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace mc {

namespace {

/// 流体标签目录（相对 data/<namespace>/）
constexpr std::string_view FLUID_TAG_DIRECTORY = "tags/fluid";

/**
 * @brief 流体资源位置池
 *
 * 通用加载器以 size_t 索引传递成员。本池把 ResourceLocation 映射为稠密索引，
 * 使内置标签成员（由 FluidTags::initialize 直接以 ResourceLocation 注册）与
 * 数据包成员共用同一索引空间。
 */
class _FluidMemberPool {
public:
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

    [[nodiscard]] const ResourceLocation& at(std::size_t index) const { return m_locations.at(index); }

private:
    std::vector<ResourceLocation> m_locations;
    std::unordered_map<ResourceLocation, std::size_t> m_indexByLocation;
};

void _buildCallbacks(_FluidMemberPool& pool,
    resource::tag::TagMemberResolver& resolveMember,
    resource::tag::TagMemberReader& readTag,
    resource::tag::TagFiller& fillTag)
{
    resolveMember = [&pool](std::string_view memberId) -> std::size_t {
        ResourceLocation location = ResourceLocation::parse(memberId);
        if (fluid::FluidRegistry::instance().getFluid(location) == nullptr) {
            return resource::tag::TAG_MEMBER_NOT_FOUND;
        }
        return pool.getOrCreate(location);
    };

    readTag = [&pool](const ResourceLocation& tagId) -> std::vector<std::size_t> {
        std::vector<std::size_t> members;
        auto* tag = fluid::FluidTags::getTag(tagId);
        if (tag == nullptr) {
            return members;
        }
        for (const auto& fluidId : tag->fluids()) {
            members.push_back(pool.getOrCreate(fluidId));
        }
        return members;
    };

    fillTag = [&pool](const ResourceLocation& tagId, const std::vector<std::size_t>& members, bool replace) {
        fluid::FluidTag& tag = fluid::FluidTags::registerTag(tagId);
        if (replace) {
            tag.clear();
        }
        for (std::size_t index : members) {
            tag.add(pool.at(index));
        }
    };
}

} // namespace

Result<size_t> FluidTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    _FluidMemberPool pool;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromDataPackRepository(
        dataPackList, FLUID_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

Result<size_t> FluidTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    _FluidMemberPool pool;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromResourcePack(
        pack, FLUID_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

} // namespace mc
