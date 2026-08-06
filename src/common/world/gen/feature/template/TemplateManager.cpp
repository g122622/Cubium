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

#include "TemplateManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateLoader.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

TemplateManager::TemplateManager()
    : m_emptyTemplate(std::make_unique<Template>())
{}

TemplateManager::~TemplateManager() = default;

void TemplateManager::setResourcePack(const IResourcePack* pack)
{
    m_resourcePack = pack;
}

void TemplateManager::setDataPackRepository(const resource::DataPackRepository* dataPackList)
{
    m_dataPackList = dataPackList;
}

void TemplateManager::setStructurePackSource(const IStructurePackSource* source)
{
    m_structurePackSource = source;
}

const Template* TemplateManager::getTemplate(const ResourceLocation& location)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_templates.find(location);
    if (it != m_templates.end()) {
        return it->second.get();
    }

    auto templ = _loadTemplate(location);
    if (!templ) {
        return nullptr;
    }

    const Template* result = templ.get();
    m_templates[location] = std::move(templ);
    return result;
}

const Template& TemplateManager::getTemplateDefaulted(const ResourceLocation& location)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_templates.find(location);
    if (it != m_templates.end()) {
        return *it->second.get();
    }

    auto templ = _loadTemplate(location);
    if (templ) {
        const Template& result = *templ;
        m_templates[location] = std::move(templ);
        return result;
    }

    return *m_emptyTemplate;
}

bool TemplateManager::hasTemplate(const ResourceLocation& location) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_templates.find(location) != m_templates.end();
}

void TemplateManager::addTemplate(const ResourceLocation& location, std::unique_ptr<Template> templ)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_templates[location] = std::move(templ);
}

void TemplateManager::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_templates.clear();
}

std::unique_ptr<Template> TemplateManager::_loadTemplate(const ResourceLocation& location)
{
    // 优先从基岩版结构包资源源加载 .mcstructure（GameTest 场景，优先级最高）
    if (m_structurePackSource != nullptr) {
        auto result =
            m_structurePackSource->readStructure(std::string(location.namespace_()), std::string(location.path()));
        if (result.success() && !result.value().empty()) {
            auto templ = TemplateLoader::loadFromBedrockMcStructure(result.value());
            if (templ) {
                return templ;
            }
        }
    }

    // 优先从 DataPackRepository 加载（支持多数据包优先级）
    if (m_dataPackList) {
        std::string resourcePath =
            std::string(location.namespace_()) + "/structure/" + std::string(location.path()) + ".nbt";
        auto result = m_dataPackList->readResource(resourcePath);
        if (result.success() && !result.value().empty()) {
            auto templ = TemplateLoader::loadFromCompressedNbt(result.value());
            if (templ) {
                return templ;
            }
        }
    }

    // 尝试从单个资源包加载
    if (m_resourcePack) {
        auto templ = TemplateLoader::loadFromResourcePack(*m_resourcePack, location);
        if (templ) {
            return templ;
        }
    }

    // 从文件系统加载（支持开发期目录和存档 generated 目录）
    const std::vector<std::filesystem::path> baseDirs = {std::filesystem::current_path(),
        std::filesystem::current_path() / "generated",
        std::filesystem::current_path() / "data"};

    for (const auto& baseDir : baseDirs) {
        std::filesystem::path path = baseDir / location.namespace_() / "structures" / (location.path() + ".nbt");
        if (!std::filesystem::exists(path)) {
            continue;
        }

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            continue;
        }

        const std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) {
            continue;
        }

        file.seekg(0, std::ios::beg);
        std::vector<u8> data(static_cast<size_t>(fileSize));
        if (!file.read(reinterpret_cast<char*>(data.data()), fileSize)) {
            continue;
        }

        auto templ = TemplateLoader::loadFromCompressedNbt(data);
        if (templ) {
            return templ;
        }
    }

    return nullptr;
}

std::unique_ptr<Template> TemplateManager::createProceduralTemplate(
    const std::string& name, i32 width, i32 height, i32 depth)
{
    auto templ = std::make_unique<Template>();
    templ->setSize(BlockPos(width, height, depth));
    return templ;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
