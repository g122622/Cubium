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

#include "StructureManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/world/gen/jigsaw/TemplatePoolLoader.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

// StructureRegistry 实现
// 【无硬编码兜底】注册表只由 StructureDefinitionLoader 从数据包装配，故不持有初始化标志。
// 详见头文件类注释。

std::unordered_map<ResourceLocation, std::unique_ptr<Structure>>& StructureRegistry::getStructures()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<Structure>> structures;
    return structures;
}

std::vector<const Structure*>& StructureRegistry::getStructureList()
{
    static std::vector<const Structure*> structureList;
    return structureList;
}

void StructureRegistry::clear()
{
    getStructures().clear();
    getStructureList().clear();
}

void StructureRegistry::registerStructure(std::unique_ptr<Structure> structure)
{
    if (!structure) {
        return;
    }

    const ResourceLocation& id = structure->id();
    auto& structures = getStructures();
    auto& list = getStructureList();

    if (structures.find(id) == structures.end()) {
        list.push_back(structure.get());
        structures[id] = std::move(structure);
    } else {
        spdlog::warn("StructureRegistry: Re registering structure {}", id.toString());
    }
}

const Structure* StructureRegistry::get(const ResourceLocation& id)
{
    auto& structures = getStructures();
    auto it = structures.find(id);
    return it != structures.end() ? it->second.get() : nullptr;
}

const Structure* StructureRegistry::get(const std::string& name)
{
    // 兼容旧接口：将字符串名称转换为 ResourceLocation
    ResourceLocation id = ResourceLocation::parse(name);
    return get(id);
}

const std::vector<const Structure*>& StructureRegistry::getAll()
{
    return getStructureList();
}

// StructureManager 实现
StructureManager::StructureManager(i64 seed)
    : m_seed(seed)
{}

void StructureManager::clearCache()
{
    m_structureCheck.clearCache();
}

} // namespace mc::world::gen::structure

// StructureRegistry loadTemplatePoolsFromDataPacks 实现
size_t mc::world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(
    const resource::DataPackRepository& dataPackList)
{
    auto result = jigsaw::TemplatePoolLoader::loadFromDataPackRepository(dataPackList);
    if (result.success()) {
        return result.value();
    }
    return 0;
}
