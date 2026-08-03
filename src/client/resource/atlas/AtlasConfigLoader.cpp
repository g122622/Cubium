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
 */

#include "client/resource/atlas/AtlasConfigLoader.hpp"

#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/AtlasSourceParser.hpp"
#include "common/core/Result.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::client::resource::atlas {

namespace {

constexpr std::string_view ATLAS_DIR = "atlases";
constexpr std::string_view JSON_EXT = ".json";

} // namespace

Result<std::vector<std::unique_ptr<AtlasSource>>> AtlasConfigLoader::load(
    const std::vector<ResourcePackPtr>& packs, const ResourceLocation& atlasId)
{
    std::vector<std::unique_ptr<AtlasSource>> combined;
    // 资源位置 path 段定位文件名：assets/<ns>/atlases/<atlasId.path()>.json
    const std::string& atlasName = atlasId.path();

    // 正序遍历：靠前=低优先级，靠后=高优先级。sources 拼接（addAll），
    // 覆盖语义延迟到 SpriteSourceOutput::build() 阶段处理。
    for (const auto& pack : packs) {
        if (!pack) {
            continue;
        }
        // 枚举该包所有命名空间，在 assets/<ns>/atlases/<atlasName>.json 探测
        auto nsResult = pack->getResourceNamespaces(mc::resource::PackType::ClientResources);
        if (nsResult.failed()) {
            continue;
        }
        for (const auto& ns : nsResult.value()) {
            const std::string relPath = ns + "/" + std::string(ATLAS_DIR) + "/" + atlasName + std::string(JSON_EXT);
            if (!pack->hasResource(mc::resource::PackType::ClientResources, relPath)) {
                continue;
            }
            auto readResult = pack->readTextResource(mc::resource::PackType::ClientResources, relPath);
            if (readResult.failed()) {
                spdlog::warn("atlas config: failed to read {} from pack '{}': {}",
                    relPath,
                    pack->name(),
                    readResult.error().message());
                continue;
            }
            auto parseResult = AtlasSourceParser::parseAtlasText(readResult.value());
            if (parseResult.failed()) {
                spdlog::warn("atlas config: failed to parse {} from pack '{}': {}",
                    relPath,
                    pack->name(),
                    parseResult.error().message());
                continue;
            }
            auto packSources = std::move(parseResult).value();
            for (auto& src : packSources) {
                combined.push_back(std::move(src));
            }
        }
    }
    return combined;
}

} // namespace mc::client::resource::atlas
