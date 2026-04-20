#include <gtest/gtest.h>
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/PackMetadata.hpp"
#include "common/resource/FolderResourcePack.hpp"
#include "common/resource/ZipResourcePack.hpp"
#include "common/resource/ResourcePackList.hpp"
#include "common/core/settings/ResourcePackListOption.hpp"

using namespace mc;

// ResourceLocation测试
TEST(ResourceLocationTest, DefaultConstructor) {
    ResourceLocation loc;
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_TRUE(loc.path().empty());
}

TEST(ResourceLocationTest, ParseWithoutNamespace) {
    ResourceLocation loc("textures/blocks/stone");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/blocks/stone");
}

TEST(ResourceLocationTest, ParseWithNamespace) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/blocks/stone");
}

TEST(ResourceLocationTest, ParseCustomNamespace) {
    ResourceLocation loc("mymod:blocks/custom_block");
    EXPECT_EQ(loc.namespace_(), "mymod");
    EXPECT_EQ(loc.path(), "blocks/custom_block");
}

TEST(ResourceLocationTest, ToString) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toString(), "minecraft:textures/blocks/stone");
}

TEST(ResourceLocationTest, ToFilePath) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toFilePath(), "assets/minecraft/textures/blocks/stone");
}

TEST(ResourceLocationTest, ToFilePathWithExtension) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toFilePath("png"), "assets/minecraft/textures/blocks/stone.png");
}

TEST(ResourceLocationTest, Comparison) {
    ResourceLocation loc1("minecraft:stone");
    ResourceLocation loc2("minecraft:stone");
    ResourceLocation loc3("minecraft:dirt");

    EXPECT_EQ(loc1, loc2);
    EXPECT_NE(loc1, loc3);
    EXPECT_LT(loc3, loc1); // dirt < stone alphabetically
}

// PackMetadata测试
TEST(PackMetadataTest, ParseValidJson) {
    const char* json = R"({"pack": {"pack_format": 3, "description": "Test Pack"}})";
    auto result = PackMetadata::parse(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().packFormat(), 3);
    EXPECT_EQ(result.value().description(), "Test Pack");
}

TEST(PackMetadataTest, ParseEmptyJson) {
    const char* json = "{}";
    auto result = PackMetadata::parse(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().packFormat(), 0);
    EXPECT_TRUE(result.value().description().empty());
}

TEST(PackMetadataTest, IsCompatible) {
    PackMetadata meta;
    // 需要通过parse设置值，这里只测试isCompatible方法
    auto result = PackMetadata::parse(R"({"pack": {"pack_format": 3}})");
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(result.value().isCompatible(1, 5));
    EXPECT_TRUE(result.value().isCompatible(3, 3));
    EXPECT_FALSE(result.value().isCompatible(4, 6));
}

// FolderResourcePack测试 - 使用测试资源包
TEST(FolderResourcePackTest, LoadPackMetadata) {
    // 使用实际的资源包路径
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        EXPECT_EQ(pack.metadata().packFormat(), 3);
        EXPECT_FALSE(pack.metadata().description().empty());
    }
}

TEST(FolderResourcePackTest, HasResource) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        EXPECT_TRUE(pack.hasResource("pack.mcmeta"));
        EXPECT_TRUE(pack.hasResource("assets/minecraft/blockstates/oak_log.json"));
        EXPECT_FALSE(pack.hasResource("nonexistent/file.json"));
    }
}

TEST(FolderResourcePackTest, ReadResource) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        auto readResult = pack.readResource("pack.mcmeta");
        if (readResult.success()) {
            EXPECT_FALSE(readResult.value().empty());
        }
    }
}

TEST(FolderResourcePackTest, ReadTextResource) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        auto readResult = pack.readTextResource("pack.mcmeta");
        if (readResult.success()) {
            EXPECT_TRUE(readResult.value().find("pack") != String::npos);
        }
    }
}

TEST(FolderResourcePackTest, ListResources) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        auto listResult = pack.listResources("assets/minecraft/blockstates", "json");
        if (listResult.success()) {
            EXPECT_FALSE(listResult.value().empty());

            // 检查是否包含oak_log.json
            bool foundOakLog = false;
            for (const auto& file : listResult.value()) {
                if (file.find("oak_log.json") != String::npos) {
                    foundOakLog = true;
                    break;
                }
            }
            EXPECT_TRUE(foundOakLog);
        }
    }
}

// ResourcePackListOption测试
TEST(ResourcePackListOptionTest, DefaultConstructor) {
    ResourcePackListOption option("resourcePacks");
    EXPECT_TRUE(option.empty());
}

TEST(ResourcePackListOptionTest, SetEntries) {
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

TEST(ResourcePackListOptionTest, SortedEntries) {
    ResourcePackListOption option("resourcePacks");

    std::vector<ResourcePackEntry> entries;
    entries.emplace_back("packs/a.zip", true, 2);  // 高优先级
    entries.emplace_back("packs/b.zip", true, 0);  // 低优先级
    entries.emplace_back("packs/c.zip", true, 1);  // 中优先级

    option.setEntries(std::move(entries));

    auto sorted = option.getSortedEnabledEntries();
    ASSERT_EQ(sorted.size(), static_cast<size_t>(3));
    // 高优先级在前
    EXPECT_EQ(sorted[0].path, "packs/a.zip");
    EXPECT_EQ(sorted[1].path, "packs/c.zip");
    EXPECT_EQ(sorted[2].path, "packs/b.zip");
}

TEST(ResourcePackListOptionTest, JsonSerialization) {
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

// ResourcePackList测试
TEST(ResourcePackListTest, EmptyList) {
    ResourcePackList list;
    EXPECT_EQ(list.packCount(), static_cast<size_t>(0));
    EXPECT_EQ(list.enabledPackCount(), static_cast<size_t>(0));
    EXPECT_TRUE(list.getEnabledPacks().empty());
}

TEST(ResourcePackListTest, HasResourceEmpty) {
    ResourcePackList list;
    EXPECT_FALSE(list.hasResource("test.json"));
}

TEST(ResourcePackListTest, ReadResourceEmpty) {
    ResourcePackList list;
    auto result = list.readResource("test.json");
    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceNotFound);
}

TEST(ResourcePackListTest, SetEnabled) {
    ResourcePackList list;

    // 测试空列表
    EXPECT_FALSE(list.setEnabled("test", true));
    EXPECT_FALSE(list.setEnabled("test", false));
}

TEST(ResourcePackListTest, SetPriority) {
    ResourcePackList list;

    // 测试空列表
    EXPECT_FALSE(list.setPriority("test", 5));
}

TEST(ResourcePackListTest, MoveUp) {
    ResourcePackList list;

    // 测试空列表
    EXPECT_FALSE(list.moveUp("test"));
}

TEST(ResourcePackListTest, MoveDown) {
    ResourcePackList list;

    // 测试空列表
    EXPECT_FALSE(list.moveDown("test"));
}

TEST(ResourcePackListTest, Clear) {
    ResourcePackList list;
    list.clear();  // 不应崩溃
    EXPECT_EQ(list.packCount(), static_cast<size_t>(0));
}

TEST(ResourcePackListTest, FindPackEmpty) {
    ResourcePackList list;
    EXPECT_FALSE(list.getPackInfo("test").has_value());
}
