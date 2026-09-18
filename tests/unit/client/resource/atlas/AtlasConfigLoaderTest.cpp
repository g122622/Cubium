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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <gtest/gtest.h>

#include "client/resource/atlas/AtlasConfigLoader.hpp"
#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/Sources.hpp"
#include "common/core/Result.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/PackMetadata.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace mc::client::resource::atlas {

namespace {

// 复用与 AtlasSourceTest 相同的内存资源包实现
class MemoryResourcePack final : public IResourcePack {
public:
    Result<void> initialize() override { return {}; }

    const PackMetadata& metadata() const override { return m_metadata; }

    bool hasResource(mc::resource::PackType type, std::string_view resourcePath) const override
    {
        std::string full = makeTypedPath(type, resourcePath);
        return m_resources.find(full) != m_resources.end();
    }

    Result<std::vector<u8>> readResource(mc::resource::PackType type, std::string_view resourcePath) const override
    {
        std::string full = makeTypedPath(type, resourcePath);
        auto it = m_resources.find(full);
        if (it == m_resources.end()) {
            return Error(ErrorCode::NotFound, "Resource not found");
        }
        return it->second;
    }

    Result<std::vector<std::string>> listResources(
        mc::resource::PackType type, std::string_view directory, std::string_view extension) const override
    {
        std::string typeDir(mc::resource::packTypeDirectoryName(type));
        std::string fullDirectory = typeDir + "/" + std::string(directory);
        if (!fullDirectory.empty() && fullDirectory.back() != '/') {
            fullDirectory += '/';
        }
        std::string typePrefix = typeDir + "/";
        std::vector<std::string> results;
        const std::string ext(extension);
        for (const auto& [path, _] : m_resources) {
            const bool prefixMatched = path.rfind(fullDirectory, 0) == 0;
            const bool extensionMatched =
                ext.empty() || (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext);
            if (prefixMatched && extensionMatched) {
                results.push_back(path.substr(typePrefix.size()));
            }
        }
        std::sort(results.begin(), results.end());
        return results;
    }

    Result<std::vector<std::string>> getResourceNamespaces(mc::resource::PackType type) const override
    {
        std::string typeDir(mc::resource::packTypeDirectoryName(type));
        std::string prefix = typeDir + "/";
        std::unordered_set<std::string> namespaces;
        for (const auto& [path, _] : m_resources) {
            if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
                std::string rest = path.substr(prefix.size());
                size_t slashPos = rest.find('/');
                if (slashPos != std::string::npos) {
                    namespaces.insert(rest.substr(0, slashPos));
                }
            }
        }
        std::vector<std::string> result(namespaces.begin(), namespaces.end());
        std::sort(result.begin(), result.end());
        return result;
    }

    std::string name() const override { return "AtlasConfigLoaderTestPack"; }

    void addResource(const std::string& path, const std::vector<u8>& data) { m_resources[path] = data; }

private:
    PackMetadata m_metadata{6, "test"};
    std::unordered_map<std::string, std::vector<u8>> m_resources;

    static std::string makeTypedPath(mc::resource::PackType type, std::string_view resourcePath)
    {
        std::string typeDir(mc::resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        std::string prefix = typeDir + "/";
        if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
            return path;
        }
        return prefix + path;
    }
};

ResourcePackPtr packPtr(MemoryResourcePack& pack)
{
    return std::shared_ptr<IResourcePack>(&pack, [](IResourcePack*) {});
}

} // namespace

// ============================================================================
// AtlasConfigLoader：多包 sources 拼接（addAll 不覆盖）
// ============================================================================

TEST(AtlasConfigLoaderTest, EmptyPacksReturnsEmpty)
{
    std::vector<ResourcePackPtr> packs;
    auto result = AtlasConfigLoader::load(packs, ResourceLocation("minecraft", "blocks"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 0u);
}

TEST(AtlasConfigLoaderTest, PackWithoutAtlasReturnsEmpty)
{
    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/block/stone.png", {0});

    std::vector<ResourcePackPtr> packs = {packPtr(pack)};
    auto result = AtlasConfigLoader::load(packs, ResourceLocation("minecraft", "blocks"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 0u);
}

TEST(AtlasConfigLoaderTest, SinglePackSourcesLoaded)
{
    MemoryResourcePack pack;
    const std::string atlasJson = R"({
        "sources": [
            { "type": "minecraft:single", "resource": "minecraft:block/stone" },
            { "type": "minecraft:directory", "source": "block", "prefix": "block/" }
        ]
    })";
    pack.addResource("assets/minecraft/atlases/blocks.json", std::vector<u8>(atlasJson.begin(), atlasJson.end()));

    std::vector<ResourcePackPtr> packs = {packPtr(pack)};
    auto result = AtlasConfigLoader::load(packs, ResourceLocation("minecraft", "blocks"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 2u);
}

TEST(AtlasConfigLoaderTest, MultiplePacksConcatenateSources)
{
    // 低优先级包提供 1 个 source，高优先级包提供 1 个 source，拼接后 2 个
    MemoryResourcePack lowPack;
    const std::string lowJson = R"({
        "sources": [
            { "type": "minecraft:single", "resource": "minecraft:block/stone" }
        ]
    })";
    lowPack.addResource("assets/minecraft/atlases/blocks.json", std::vector<u8>(lowJson.begin(), lowJson.end()));

    MemoryResourcePack highPack;
    const std::string highJson = R"({
        "sources": [
            { "type": "minecraft:directory", "source": "block", "prefix": "block/" }
        ]
    })";
    highPack.addResource("assets/minecraft/atlases/blocks.json", std::vector<u8>(highJson.begin(), highJson.end()));

    std::vector<ResourcePackPtr> packs = {packPtr(lowPack), packPtr(highPack)};
    auto result = AtlasConfigLoader::load(packs, ResourceLocation("minecraft", "blocks"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 2u);
    // 正序：低优先级 source 在前
    EXPECT_NE(result.value()[0]->describe().find("single"), std::string::npos);
    EXPECT_NE(result.value()[1]->describe().find("directory"), std::string::npos);
}

TEST(AtlasConfigLoaderTest, BadAtlasJsonInPackIsSkipped)
{
    // 单个包 atlas JSON 解析失败只记日志跳过，load 仍返回成功（空列表）
    // 对齐原版 SpriteSourceList.load 的容错语义
    MemoryResourcePack pack;
    const std::string badJson = "not json {";
    pack.addResource("assets/minecraft/atlases/blocks.json", std::vector<u8>(badJson.begin(), badJson.end()));

    std::vector<ResourcePackPtr> packs = {packPtr(pack)};
    auto result = AtlasConfigLoader::load(packs, ResourceLocation("minecraft", "blocks"));
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 0u);
}

TEST(AtlasConfigLoaderTest, LoadsFromNonMinecraftNamespace)
{
    MemoryResourcePack pack;
    const std::string atlasJson = R"({
        "sources": [
            { "type": "minecraft:single", "resource": "cubium:block/custom" }
        ]
    })";
    pack.addResource("assets/cubium/atlases/blocks.json", std::vector<u8>(atlasJson.begin(), atlasJson.end()));

    std::vector<ResourcePackPtr> packs = {packPtr(pack)};
    auto result = AtlasConfigLoader::load(packs, ResourceLocation("minecraft", "blocks"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 1u);
}

} // namespace mc::client::resource::atlas
