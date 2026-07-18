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

#include "BlockStateLoader.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

Result<void> BlockStateLoader::loadFromResourcePack(IResourcePack& resourcePack)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "BlockStateLoader::loadFromResourcePack");

    // 列出所有方块状态文件
    auto result = resourcePack.listResources(resource::PackType::ClientResources, "minecraft/blockstates", "json");

    if (result.failed()) {
        // 目录可能不存在
        return Result<void>::ok();
    }

    auto files = result.value();
    size_t loaded = 0;

    for (const auto& file : files) {
        // 提取方块名称
        // assets/minecraft/blockstates/stone.json -> stone
        std::string blockName;
        size_t lastSlash = file.find_last_of("/\\");
        size_t dotPos = file.find_last_of('.');

        if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
            blockName = file.substr(lastSlash + 1, dotPos - lastSlash - 1);
        } else {
            continue;
        }

        // 读取并解析
        auto readResult = resourcePack.readTextResource(resource::PackType::ClientResources, file);
        if (readResult.failed()) {
            continue;
        }

        auto parseResult = BlockStateDefinition::parse(readResult.value());
        if (parseResult.failed()) {
            continue;
        }

        ResourceLocation blockId("minecraft", blockName);
        m_blockStates[blockId] = parseResult.value();
        loaded++;
    }

    if (loaded > 0) {
        spdlog::info("BlockStateLoader: Loaded {} block states from '{}'", loaded, resourcePack.name());
    }

    return Result<void>::ok();
}

const BlockStateDefinition* BlockStateLoader::getBlockState(const ResourceLocation& blockId) const
{
    auto it = m_blockStates.find(blockId);
    if (it != m_blockStates.end()) {
        return &it->second;
    }
    return nullptr;
}

void BlockStateLoader::clearCache()
{
    m_blockStates.clear();
}

std::vector<ResourceLocation> BlockStateLoader::getLoadedBlockStates() const
{
    std::vector<ResourceLocation> result;
    result.reserve(m_blockStates.size());

    for (const auto& [loc, def] : m_blockStates) {
        result.push_back(loc);
    }

    return result;
}

} // namespace mc
