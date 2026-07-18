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

#include <gtest/gtest.h>

#include "client/resource/ResourceManager.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace mc::test {
namespace {

class InMemoryResourcePack final : public IResourcePack {
public:
    InMemoryResourcePack() = default;

    Result<void> initialize() override { return Result<void>::ok(); }

    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }

    [[nodiscard]] bool hasResource(resource::PackType type, std::string_view resourcePath) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        std::string full;
        if (hasTypePrefix) {
            full = path;
        } else {
            full = typeDir + "/" + path;
        }
        return m_resources.find(full) != m_resources.end();
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(
        resource::PackType type, std::string_view resourcePath) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        std::string full;
        if (hasTypePrefix) {
            full = path;
        } else {
            full = typeDir + "/" + path;
        }
        auto it = m_resources.find(full);
        if (it == m_resources.end()) {
            return Error(ErrorCode::NotFound, "Resource not found");
        }
        return it->second;
    }

    [[nodiscard]] Result<std::vector<std::string>> listResources(
        resource::PackType type, std::string_view directory, std::string_view extension) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string fullDirectory = typeDir + "/" + std::string(directory);
        std::vector<std::string> result;
        const std::string ext(extension);
        for (const auto& [path, _] : m_resources) {
            const bool inDir = fullDirectory.empty() || path.rfind(fullDirectory, 0) == 0;
            const bool extMatch =
                ext.empty() || (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext);
            if (inDir && extMatch) {
                result.push_back(path);
            }
        }
        return result;
    }

    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(resource::PackType type) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
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

    [[nodiscard]] std::string name() const override { return "InMemoryResourcePack"; }

    void add(std::string path, std::vector<u8> bytes) { m_resources.emplace(std::move(path), std::move(bytes)); }

private:
    PackMetadata m_metadata{6, "test-pack"};
    std::unordered_map<std::string, std::vector<u8>> m_resources;
};

std::vector<u8> makeValid1x1Png()
{
    return {137,
        80,
        78,
        71,
        13,
        10,
        26,
        10,
        0,
        0,
        0,
        13,
        73,
        72,
        68,
        82,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        1,
        8,
        4,
        0,
        0,
        0,
        181,
        28,
        12,
        2,
        0,
        0,
        0,
        11,
        73,
        68,
        65,
        84,
        120,
        218,
        99,
        252,
        255,
        31,
        0,
        3,
        3,
        2,
        0,
        239,
        156,
        7,
        219,
        0,
        0,
        0,
        0,
        73,
        69,
        78,
        68,
        174,
        66,
        96,
        130};
}

std::vector<u8> toBytes(std::string_view content)
{
    return std::vector<u8>(content.begin(), content.end());
}

} // namespace

TEST(ResourceManagerTextureDecodeTest, LoadCloudTextureFromResourcePack)
{
    ResourceManager manager;

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/textures/environment/clouds.png", makeValid1x1Png());

    auto addResult = manager.addResourcePack(pack);
    ASSERT_TRUE(addResult.success());

    auto decodedResult = manager.loadTextureRGBA(ResourceLocation("minecraft:textures/environment/clouds"));
    ASSERT_TRUE(decodedResult.success());

    const auto& decoded = decodedResult.value();
    EXPECT_EQ(decoded.width, 1u);
    EXPECT_EQ(decoded.height, 1u);
    EXPECT_EQ(decoded.pixels.size(), 4u);
}

TEST(ResourceManagerTextureDecodeTest, ReturnNotFoundWhenCloudTextureMissing)
{
    ResourceManager manager;

    auto pack = std::make_shared<InMemoryResourcePack>();
    auto addResult = manager.addResourcePack(pack);
    ASSERT_TRUE(addResult.success());

    auto decodedResult = manager.loadTextureRGBA(ResourceLocation("minecraft:textures/environment/clouds"));
    ASSERT_TRUE(decodedResult.failed());
}

TEST(ResourceManagerTextureDecodeTest, WaterModelWithParticleTextureKeepsParticleOnlyAppearance)
{
    VanillaBlocks::initialize();

    ResourceManager manager;
    auto pack = std::make_shared<InMemoryResourcePack>();

    pack->add("assets/minecraft/blockstates/water.json", toBytes(R"({
    "variants": {
        "": { "model": "minecraft:block/water" }
    }
})"));

    pack->add("assets/minecraft/models/block/water.json", toBytes(R"({
    "textures": {
        "particle": "block/water_still"
    }
})"));

    pack->add("assets/minecraft/textures/block/water_still.png", makeValid1x1Png());

    ASSERT_TRUE(manager.addResourcePack(pack).success());
    ASSERT_TRUE(manager.loadAllResources().success());

    // computeBlockAppearances 需要一个纹理区域查询回调（生产环境由 AtlasManager 提供）。
    // 本测试关注水面外观（无面纹理），传空回调即可——回调返回 nullptr 时面纹理为空，
    // particle 路径不参与区域查询。
    manager.computeBlockAppearances({});

    const auto* appearance = manager.getBlockAppearance(ResourceLocation("minecraft:water"), {});
    ASSERT_NE(appearance, nullptr);
    EXPECT_TRUE(appearance->faceTextures.empty());
    EXPECT_TRUE(appearance->faceTextureLayers.empty());
}

} // namespace mc::test
