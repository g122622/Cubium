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

#include "common/core/settings/ResourcePackListOption.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/resource/pack/PackMetadata.hpp"
#include "common/resource/pack/ZipResourcePack.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace mc;

namespace {

std::filesystem::path makeTempPackDir()
{
    const auto dir = std::filesystem::temp_directory_path() / "mc_resource_location_test_pack";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir / "assets/minecraft/blockstates");

    std::ofstream mcmeta(dir / "pack.mcmeta", std::ios::binary);
    // pack_format = 75：MC 1.21.11 客户端资源包版本（resource_pack_version，
    // 见 resourcepacks/Vanilla/version.json）。原值为 6（1.16.x 遗留），与断言不一致。
    mcmeta << R"({"pack":{"pack_format":75,"description":"test"}})";
    mcmeta.close();

    std::ofstream blockstate(dir / "assets/minecraft/blockstates/oak_log.json", std::ios::binary);
    blockstate << R"({"variants":{"axis=y":{"model":"minecraft:block/oak_log"}}})";
    blockstate.close();

    return dir;
}

} // namespace

// ResourceLocation测试
TEST(ResourceLocationTest, DefaultConstructor)
{
    ResourceLocation loc;
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_TRUE(loc.path().empty());
}

TEST(ResourceLocationTest, ParseWithoutNamespace)
{
    ResourceLocation loc("textures/blocks/stone");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/blocks/stone");
}

TEST(ResourceLocationTest, ParseWithNamespace)
{
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/blocks/stone");
}

TEST(ResourceLocationTest, ParseCustomNamespace)
{
    ResourceLocation loc("mymod:blocks/custom_block");
    EXPECT_EQ(loc.namespace_(), "mymod");
    EXPECT_EQ(loc.path(), "blocks/custom_block");
}

TEST(ResourceLocationTest, ToString)
{
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toString(), "minecraft:textures/blocks/stone");
}

TEST(ResourceLocationTest, ToFilePath)
{
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toFilePath(resource::PackType::ClientResources), "assets/minecraft/textures/blocks/stone");
}

TEST(ResourceLocationTest, ToFilePathWithExtension)
{
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toFilePath(resource::PackType::ClientResources, "png"), "assets/minecraft/textures/blocks/stone.png");
}

TEST(ResourceLocationTest, Comparison)
{
    ResourceLocation loc1("minecraft:stone");
    ResourceLocation loc2("minecraft:stone");
    ResourceLocation loc3("minecraft:dirt");

    EXPECT_EQ(loc1, loc2);
    EXPECT_NE(loc1, loc3);
    EXPECT_LT(loc3, loc1); // dirt < stone alphabetically
}

// PackMetadata测试
TEST(PackMetadataTest, ParseValidJson)
{
    const char* json = R"({"pack": {"pack_format": 3, "description": "Test Pack"}})";
    auto result = PackMetadata::parse(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().packFormat(), 3);
    EXPECT_EQ(result.value().description(), "Test Pack");
}

TEST(PackMetadataTest, ParseEmptyJson)
{
    const char* json = "{}";
    auto result = PackMetadata::parse(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().packFormat(), 0);
    EXPECT_TRUE(result.value().description().empty());
}

TEST(PackMetadataTest, IsCompatible)
{
    PackMetadata meta;
    // 需要通过parse设置值，这里只测试isCompatible方法
    auto result = PackMetadata::parse(R"({"pack": {"pack_format": 3}})");
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(result.value().isCompatible(1, 5));
    EXPECT_TRUE(result.value().isCompatible(3, 3));
    EXPECT_FALSE(result.value().isCompatible(4, 6));
}

// FolderResourcePack测试 - 使用测试资源包
TEST(FolderResourcePackTest, LoadPackMetadata)
{
    const auto packDir = makeTempPackDir();
    FolderResourcePack pack(packDir.string());

    auto result = pack.initialize();
    if (result.success()) {
        // pack_format = 75：MC 1.21.11 客户端资源包版本（resource_pack_version）。
        // 原断言为 3（1.16.x 遗留），与 fixture 写入值不一致——纯测试代码 bug。
        EXPECT_EQ(pack.metadata().packFormat(), 75);
        EXPECT_FALSE(pack.metadata().description().empty());
    }
}

TEST(FolderResourcePackTest, HasResource)
{
    const auto packDir = makeTempPackDir();
    FolderResourcePack pack(packDir.string());

    auto result = pack.initialize();
    if (result.success()) {
        EXPECT_TRUE(pack.hasResource(resource::PackType::ClientResources, "../pack.mcmeta"));
        EXPECT_TRUE(pack.hasResource(resource::PackType::ClientResources, "minecraft/blockstates/oak_log.json"));
        EXPECT_FALSE(pack.hasResource(resource::PackType::ClientResources, "nonexistent/file.json"));
    }
}

TEST(FolderResourcePackTest, ReadResource)
{
    const auto packDir = makeTempPackDir();
    FolderResourcePack pack(packDir.string());

    auto result = pack.initialize();
    if (result.success()) {
        auto readResult = pack.readResource(resource::PackType::ClientResources, "../pack.mcmeta");
        if (readResult.success()) {
            EXPECT_FALSE(readResult.value().empty());
        }
    }
}

TEST(FolderResourcePackTest, ReadTextResource)
{
    const auto packDir = makeTempPackDir();
    FolderResourcePack pack(packDir.string());

    auto result = pack.initialize();
    if (result.success()) {
        auto readResult = pack.readTextResource(resource::PackType::ClientResources, "../pack.mcmeta");
        if (readResult.success()) {
            EXPECT_TRUE(readResult.value().find("pack") != std::string::npos);
        }
    }
}

TEST(FolderResourcePackTest, ListResources)
{
    const auto packDir = makeTempPackDir();
    FolderResourcePack pack(packDir.string());

    auto result = pack.initialize();
    if (result.success()) {
        auto listResult = pack.listResources(resource::PackType::ClientResources, "minecraft/blockstates", "json");
        if (listResult.success()) {
            EXPECT_FALSE(listResult.value().empty());

            // 检查是否包含oak_log.json
            bool foundOakLog = false;
            for (const auto& file : listResult.value()) {
                if (file.find("oak_log.json") != std::string::npos) {
                    foundOakLog = true;
                    break;
                }
            }
            EXPECT_TRUE(foundOakLog);
        }
    }
}

// ResourcePackListOption测试
TEST(ResourcePackListOptionTest, DefaultConstructor)
{
    ResourcePackListOption option("resourcePacks");
    EXPECT_TRUE(option.empty());
}

TEST(ResourcePackListOptionTest, SetEntries)
{
    ResourcePackListOption option("resourcePacks");

    std::vector<ResourcePackEntry> entries;
    entries.emplace_back("packs/test.zip", true, 0);
    entries.emplace_back("packs/other.zip", false, 1);

    option.setEntries(std::move(entries));

    const auto& loaded = option.entries();
    EXPECT_EQ(loaded.size(), static_cast<size_t>(2));
    EXPECT_EQ(loaded[0].path, "packs/test.zip");
    EXPECT_TRUE(loaded[0].enabled);
    EXPECT_EQ(loaded[0].priority, 0);
}

TEST(ResourcePackListOptionTest, SortedEntries)
{
    ResourcePackListOption option("resourcePacks");

    std::vector<ResourcePackEntry> entries;
    entries.emplace_back("packs/a.zip", true, 2); // 高优先级
    entries.emplace_back("packs/b.zip", true, 0); // 低优先级
    entries.emplace_back("packs/c.zip", true, 1); // 中优先级

    option.setEntries(std::move(entries));

    auto sorted = option.getSortedEnabledEntries();
    ASSERT_EQ(sorted.size(), static_cast<size_t>(3));
    // 高优先级在前
    EXPECT_EQ(sorted[0].path, "packs/a.zip");
    EXPECT_EQ(sorted[1].path, "packs/c.zip");
    EXPECT_EQ(sorted[2].path, "packs/b.zip");
}

TEST(ResourcePackListOptionTest, JsonSerialization)
{
    ResourcePackListOption option("resourcePacks");

    std::vector<ResourcePackEntry> entries;
    entries.emplace_back("packs/test.zip", true, 5);
    option.setEntries(std::move(entries));

    // 序列化到JSON
    nlohmann::json j;
    option.serialize(j);

    // 从JSON反序列化
    ResourcePackListOption option2("resourcePacks");
    option2.deserialize(j);

    const auto& loaded = option2.entries();
    ASSERT_EQ(loaded.size(), static_cast<size_t>(1));
    EXPECT_EQ(loaded[0].path, "packs/test.zip");
    EXPECT_TRUE(loaded[0].enabled);
    EXPECT_EQ(loaded[0].priority, 5);
}

// PackRepository测试
TEST(PackRepositoryTest, EmptyList)
{
    PackRepository list;
    EXPECT_EQ(list.packCount(), static_cast<size_t>(0));
    EXPECT_EQ(list.enabledPackCount(), static_cast<size_t>(0));
    EXPECT_TRUE(list.getEnabledPacks().empty());
}

TEST(PackRepositoryTest, HasResourceEmpty)
{
    PackRepository list;
    EXPECT_FALSE(list.hasResource("test.json"));
}

TEST(PackRepositoryTest, ReadResourceEmpty)
{
    PackRepository list;
    auto result = list.readResource("test.json");
    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceNotFound);
}

TEST(PackRepositoryTest, SetEnabled)
{
    PackRepository list;

    // 测试空列表
    EXPECT_FALSE(list.setEnabled("test", true));
    EXPECT_FALSE(list.setEnabled("test", false));
}

TEST(PackRepositoryTest, SetPriority)
{
    PackRepository list;

    // 测试空列表
    EXPECT_FALSE(list.setPriority("test", 5));
}

TEST(PackRepositoryTest, MoveUp)
{
    PackRepository list;

    // 测试空列表
    EXPECT_FALSE(list.moveUp("test"));
}

TEST(PackRepositoryTest, MoveDown)
{
    PackRepository list;

    // 测试空列表
    EXPECT_FALSE(list.moveDown("test"));
}

TEST(PackRepositoryTest, Clear)
{
    PackRepository list;
    list.clear(); // 不应崩溃
    EXPECT_EQ(list.packCount(), static_cast<size_t>(0));
}

TEST(PackRepositoryTest, FindPackEmpty)
{
    PackRepository list;
    EXPECT_FALSE(list.getPackInfo("test").has_value());
}
