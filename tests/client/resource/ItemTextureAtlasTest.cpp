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

#include "client/resource/ItemTextureAtlas.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace mc::client {

namespace {

class MemoryResourcePack final : public IResourcePack {
public:
    Result<void> initialize() override { return {}; }

    const PackMetadata& metadata() const override { return m_metadata; }

    bool hasResource(resource::PackType type, std::string_view resourcePath) const override
    {
        std::string full = makeTypedPath(type, resourcePath);
        return m_resources.find(full) != m_resources.end();
    }

    Result<std::vector<u8>> readResource(resource::PackType type, std::string_view resourcePath) const override
    {
        std::string full = makeTypedPath(type, resourcePath);
        auto it = m_resources.find(full);
        if (it == m_resources.end()) {
            return Error(ErrorCode::NotFound, "Resource not found");
        }
        return it->second;
    }

    Result<std::vector<std::string>> listResources(
        resource::PackType type, std::string_view directory, std::string_view extension) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
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
                // 返回相对于类型目录根的路径，与 FolderResourcePack 一致
                results.push_back(path.substr(typePrefix.size()));
            }
        }

        return results;
    }

    Result<std::vector<std::string>> getResourceNamespaces(resource::PackType type) const override
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

    std::string name() const override { return "MemoryResourcePack"; }

    void addResource(const std::string& path, const std::vector<u8>& data) { m_resources[path] = data; }

private:
    PackMetadata m_metadata{6, "test"};
    std::unordered_map<std::string, std::vector<u8>> m_resources;

    static std::string makeTypedPath(resource::PackType type, std::string_view resourcePath)
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        std::string prefix = typeDir + "/";
        if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
            return path;
        }
        return prefix + path;
    }
};

const std::vector<u8>& oneByOnePng()
{
    static const std::vector<u8> bytes = {137,
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
    return bytes;
}

Item* getOrRegisterSimpleItem(const ResourceLocation& id)
{
    Item* existing = ItemRegistry::instance().getItem(id);
    if (existing != nullptr) {
        return existing;
    }

    return &ItemRegistry::instance().registerItem(id, ItemProperties());
}

Item* getOrRegisterBlockItem(const ResourceLocation& id, const Block& block)
{
    Item* existing = ItemRegistry::instance().getItem(id);
    if (existing != nullptr) {
        return existing;
    }

    return &ItemRegistry::instance().registerItem<BlockItem>(id, block, ItemProperties());
}

} // namespace

TEST(ItemTextureAtlasTest, LoadItemTextureWithoutPngSuffixInLocation)
{
    auto* item = getOrRegisterSimpleItem(ResourceLocation("minecraft:copilot_test_item"));
    ASSERT_NE(item, nullptr);

    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/item/copilot_test_item.png", oneByOnePng());

    ItemTextureAtlas atlas;
    std::vector<ResourcePackPtr> packs = {std::shared_ptr<IResourcePack>(&pack, [](IResourcePack*) {})};
    auto result = atlas.loadFromResourcePacks(packs);

    ASSERT_TRUE(result.success());
    EXPECT_NE(atlas.getItemTexture(item->itemId()), nullptr);
    EXPECT_NE(atlas.getItemTexture(ResourceLocation("minecraft", "textures/item/copilot_test_item")), nullptr);
    EXPECT_NE(atlas.getItemTexture(ResourceLocation("minecraft", "item/copilot_test_item")), nullptr);
}

TEST(ItemTextureAtlasTest, BlockItemCanLoadFromItemTexturePath)
{
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    auto* blockItem =
        getOrRegisterBlockItem(ResourceLocation("minecraft:copilot_test_block_item"), *VanillaBlocks::STONE);
    ASSERT_NE(blockItem, nullptr);

    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/item/copilot_test_block_item.png", oneByOnePng());

    ItemTextureAtlas atlas;
    std::vector<ResourcePackPtr> packs = {std::shared_ptr<IResourcePack>(&pack, [](IResourcePack*) {})};
    auto result = atlas.loadFromResourcePacks(packs);

    ASSERT_TRUE(result.success());
    EXPECT_NE(atlas.getItemTexture(blockItem->itemId()), nullptr);
    EXPECT_NE(atlas.getItemTexture(ResourceLocation("minecraft", "item/copilot_test_block_item")), nullptr);
}

TEST(ItemTextureAtlasTest, BlockItemFallsBackToBlockTexturePath)
{
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    auto* blockItem =
        getOrRegisterBlockItem(ResourceLocation("minecraft:copilot_test_block_fallback"), *VanillaBlocks::STONE);
    ASSERT_NE(blockItem, nullptr);

    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/block/stone.png", oneByOnePng());

    ItemTextureAtlas atlas;
    std::vector<ResourcePackPtr> packs = {std::shared_ptr<IResourcePack>(&pack, [](IResourcePack*) {})};
    auto result = atlas.loadFromResourcePacks(packs);

    ASSERT_TRUE(result.success());
    EXPECT_NE(atlas.getItemTexture(blockItem->itemId()), nullptr);
    EXPECT_NE(atlas.getItemTexture(ResourceLocation("minecraft", "block/stone")), nullptr);
    EXPECT_NE(atlas.getItemTexture(ResourceLocation("minecraft", "textures/block/stone")), nullptr);
}

} // namespace mc::client
