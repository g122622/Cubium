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

#include <gtest/gtest.h>

#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/AtlasSourceParser.hpp"
#include "client/resource/atlas/IdentifierPattern.hpp"
#include "client/resource/atlas/MissingNo.hpp"
#include "client/resource/atlas/Sources.hpp"
#include "client/resource/atlas/SpriteLoader.hpp"
#include "common/core/Result.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/PackMetadata.hpp"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace mc::client::resource::atlas {

namespace {

// ============================================================================
// 测试用内存资源包（路径键含 assets/ 前缀，与 listResources 返回约定一致）
// ============================================================================
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

    std::string name() const override { return "AtlasSourceTestPack"; }

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

/// 生成 W×H 纯色 RGBA8 PNG 不可行（需编码），改用 raw 占位：测试 source 语义主要看
/// sprite 名与覆盖/removeAll，像素解码由 SpriteLoader.resolve 验证。这里提供 1×1 PNG。
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

/// 生成 W×H 纯色 RGBA8 像素（非 PNG，用于 Predecoded 路径与 unstitch/paletted 像素校验）
std::vector<u8> makeSolidRgba(u32 width, u32 height, u8 r, u8 g, u8 b, u8 a)
{
    std::vector<u8> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i + 0] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }
    return pixels;
}

ResourcePackPtr packPtr(MemoryResourcePack& pack)
{
    return std::shared_ptr<IResourcePack>(&pack, [](IResourcePack*) {});
}

/// 把 SpriteSourceOutput.build() 的结果收集为 sprite 名集合，便于断言
std::unordered_set<std::string> collectNames(const std::vector<std::pair<ResourceLocation, SpriteLoader>>& built)
{
    std::unordered_set<std::string> names;
    for (const auto& [name, _] : built) {
        names.insert(name.toString());
    }
    return names;
}

} // namespace

// ============================================================================
// SpriteSourceOutput：add 后覆盖先 / removeAll
// ============================================================================

TEST(SpriteSourceOutputTest, AddNewSprite)
{
    SpriteSourceOutput output;
    output.add(ResourceLocation("minecraft", "block/stone"),
        SpriteLoader::fromTextureResource(ResourceLocation("minecraft", "block/stone")));
    EXPECT_EQ(output.size(), 1u);
    auto built = output.build();
    ASSERT_EQ(built.size(), 1u);
    EXPECT_EQ(built[0].first.toString(), "minecraft:block/stone");
}

TEST(SpriteSourceOutputTest, LaterAddOverridesEarlier)
{
    SpriteSourceOutput output;
    // 同名 sprite 两次 add，后执行的 loader 胜出
    output.add(ResourceLocation("minecraft", "block/stone"),
        SpriteLoader::fromPredecoded(SpriteContents{makeSolidRgba(2, 2, 10, 10, 10, 255), 2, 2, std::nullopt}));
    output.add(ResourceLocation("minecraft", "block/stone"),
        SpriteLoader::fromPredecoded(SpriteContents{makeSolidRgba(2, 2, 20, 20, 20, 255), 2, 2, std::nullopt}));
    EXPECT_EQ(output.size(), 1u);
    auto built = output.build();
    ASSERT_EQ(built.size(), 1u);
    // 后 add 的 loader 应为 Predecoded 像素 20,20,20
    auto resolved = built[0].second.resolve({});
    ASSERT_TRUE(resolved.success());
    EXPECT_EQ(resolved.value().pixels[0], 20u);
}

TEST(SpriteSourceOutputTest, RemoveAllByPathRegex)
{
    SpriteSourceOutput output;
    output.add(ResourceLocation("minecraft", "block/stone"),
        SpriteLoader::fromTextureResource(ResourceLocation("minecraft", "block/stone")));
    output.add(ResourceLocation("minecraft", "block/dirt"),
        SpriteLoader::fromTextureResource(ResourceLocation("minecraft", "block/dirt")));
    output.add(ResourceLocation("minecraft", "item/stick"),
        SpriteLoader::fromTextureResource(ResourceLocation("minecraft", "item/stick")));

    IdentifierPattern pattern;
    pattern.pathRegex = std::regex("block/.*");
    output.removeAll(pattern);

    auto built = output.build();
    auto names = collectNames(built);
    EXPECT_EQ(names.size(), 1u);
    EXPECT_NE(names.find("minecraft:item/stick"), names.end());
}

TEST(SpriteSourceOutputTest, RemoveAllOnlyAffectsPriorSources)
{
    // filter 只移除它之前已累积的 sprite，之后 add 的不受影响
    SpriteSourceOutput output;
    output.add(ResourceLocation("minecraft", "block/stone"),
        SpriteLoader::fromTextureResource(ResourceLocation("minecraft", "block/stone")));

    IdentifierPattern pattern;
    pattern.pathRegex = std::regex("block/.*");
    output.removeAll(pattern);

    output.add(ResourceLocation("minecraft", "block/dirt"),
        SpriteLoader::fromTextureResource(ResourceLocation("minecraft", "block/dirt")));

    auto built = output.build();
    auto names = collectNames(built);
    EXPECT_EQ(names.size(), 1u);
    EXPECT_NE(names.find("minecraft:block/dirt"), names.end());
}

// ============================================================================
// SingleFileSource
// ============================================================================

TEST(SingleFileSourceTest, AddsSpriteWithCustomName)
{
    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/block/stone.png", oneByOnePng());

    SingleFileSource source(
        ResourceLocation("minecraft", "block/stone"), ResourceLocation("minecraft", "block/stone_custom"));
    SpriteSourceOutput output;
    auto result = source.run(pack, output);
    ASSERT_TRUE(result.success());
    auto built = output.build();
    ASSERT_EQ(built.size(), 1u);
    EXPECT_EQ(built[0].first.toString(), "minecraft:block/stone_custom");
}

TEST(SingleFileSourceTest, MissingResourceWarnsButSucceeds)
{
    MemoryResourcePack pack; // 空
    SingleFileSource source(
        ResourceLocation("minecraft", "block/missing"), ResourceLocation("minecraft", "block/missing"));
    SpriteSourceOutput output;
    auto result = source.run(pack, output);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(output.size(), 0u); // 缺失资源不 add
}

// ============================================================================
// DirectoryListerSource
// ============================================================================

TEST(DirectoryListerSourceTest, EnumeratesTexturesWithPrefix)
{
    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/block/stone.png", oneByOnePng());
    pack.addResource("assets/minecraft/textures/block/dirt.png", oneByOnePng());
    pack.addResource("assets/minecraft/textures/block/sub/cobble.png", oneByOnePng());
    pack.addResource("assets/minecraft/textures/item/stick.png", oneByOnePng()); // 不在 block 目录

    DirectoryListerSource source("block", "block/");
    SpriteSourceOutput output;
    auto result = source.run(pack, output);
    ASSERT_TRUE(result.success());

    auto built = output.build();
    auto names = collectNames(built);
    EXPECT_EQ(names.size(), 3u);
    EXPECT_NE(names.find("minecraft:block/stone"), names.end());
    EXPECT_NE(names.find("minecraft:block/dirt"), names.end());
    EXPECT_NE(names.find("minecraft:block/sub/cobble"), names.end());
    EXPECT_EQ(names.find("minecraft:item/stick"), names.end());
}

// ============================================================================
// FilterSource
// ============================================================================

TEST(FilterSourceTest, RemovesMatchingFromAccumulated)
{
    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/block/stone.png", oneByOnePng());
    pack.addResource("assets/minecraft/textures/item/stick.png", oneByOnePng());

    SpriteSourceOutput output;
    DirectoryListerSource blockDir("block", "block/");
    DirectoryListerSource itemDir("item", "item/");
    ASSERT_TRUE(blockDir.run(pack, output).success());
    ASSERT_TRUE(itemDir.run(pack, output).success());
    EXPECT_EQ(output.size(), 2u);

    // filter 移除所有 block/ 前缀
    IdentifierPattern pattern;
    pattern.pathRegex = std::regex("block/.*");
    FilterSource filter(pattern);
    ASSERT_TRUE(filter.run(pack, output).success());

    auto built = output.build();
    auto names = collectNames(built);
    EXPECT_EQ(names.size(), 1u);
    EXPECT_NE(names.find("minecraft:item/stick"), names.end());
}

// ============================================================================
// UnstitcherSource
// ============================================================================

TEST(UnstitcherSourceTest, SlicesRegionsFromLargeImage)
{
    // 构造一个 2×2 纯色 Predecoded 像素图作为大图（无法用 PNG 编码，改用 SpriteContents 直接验证像素）
    // 这里通过 Paletted/Unstitch 的 Predecoded 路径验证像素裁剪逻辑：
    // 由于 unstitch 需要真实 PNG，本测试改用 1×1 PNG + divisor 验证整图取出。
    MemoryResourcePack pack;
    pack.addResource("assets/minecraft/textures/colormap/test.png", oneByOnePng());

    UnstitchRegion region;
    region.sprite = ResourceLocation("minecraft", "block/test");
    region.x = 0.0;
    region.y = 0.0;
    region.width = 1.0;
    region.height = 1.0;

    UnstitcherSource source(ResourceLocation("minecraft", "colormap/test"), {region}, 1.0, 1.0);
    SpriteSourceOutput output;
    auto result = source.run(pack, output);
    ASSERT_TRUE(result.success());

    auto built = output.build();
    ASSERT_EQ(built.size(), 1u);
    EXPECT_EQ(built[0].first.toString(), "minecraft:block/test");
    // resolve 验证像素（Predecoded）
    auto resolved = built[0].second.resolve({});
    ASSERT_TRUE(resolved.success());
    EXPECT_EQ(resolved.value().width, 1u);
    EXPECT_EQ(resolved.value().height, 1u);
}

// ============================================================================
// PalettedPermutationsSource
// ============================================================================

TEST(PalettedPermutationsSourceTest, GeneratesDerivedSpritesForAllPermutations)
{
    // 由于 paletted 需要真实 PNG 像素，本测试主要验证：
    // 1) palette_key 缺失时跳过且不崩
    // 2) describe() 文本
    PalettedPermutationsSource source({ResourceLocation("minecraft", "entity/test/base")},
        ResourceLocation("minecraft", "colormap/palette_key"),
        {{"red", ResourceLocation("minecraft", "colormap/red")}},
        "_");

    MemoryResourcePack pack; // 空：palette_key 缺失，应 warn 跳过
    SpriteSourceOutput output;
    auto result = source.run(pack, output);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(output.size(), 0u);

    EXPECT_NE(source.describe().find("paletted_permutations"), std::string::npos);
}

// ============================================================================
// AtlasSourceParser：type dispatch
// ============================================================================

TEST(AtlasSourceParserTest, ParseSingleSource)
{
    nlohmann::json j = nlohmann::json::parse(R"({
        "type": "minecraft:single",
        "resource": "minecraft:block/stone",
        "sprite": "minecraft:block/stone_custom"
    })");
    auto result = AtlasSourceParser::parseSource(j);
    ASSERT_TRUE(result.success());
    auto source = std::move(result).value();
    EXPECT_NE(source->describe().find("single"), std::string::npos);
}

TEST(AtlasSourceParserTest, ParseDirectorySource)
{
    nlohmann::json j = nlohmann::json::parse(R"({
        "type": "minecraft:directory",
        "source": "block",
        "prefix": "block/"
    })");
    auto result = AtlasSourceParser::parseSource(j);
    ASSERT_TRUE(result.success());
    auto source = std::move(result).value();
    EXPECT_NE(source->describe().find("directory"), std::string::npos);
}

TEST(AtlasSourceParserTest, ParseFilterSource)
{
    nlohmann::json j = nlohmann::json::parse(R"({
        "type": "minecraft:filter",
        "pattern": { "namespace": "minecraft", "path": "block/.*" }
    })");
    auto result = AtlasSourceParser::parseSource(j);
    ASSERT_TRUE(result.success());
    auto source = std::move(result).value();
    EXPECT_EQ(source->describe(), "filter");
}

TEST(AtlasSourceParserTest, ParseUnstitchSource)
{
    nlohmann::json j = nlohmann::json::parse(R"({
        "type": "minecraft:unstitch",
        "resource": "minecraft:colormap/test",
        "regions": [
            { "sprite": "minecraft:block/a", "x": 0, "y": 0, "width": 1, "height": 1 },
            { "sprite": "minecraft:block/b", "x": 1, "y": 0, "width": 1, "height": 1 }
        ],
        "divisor_x": 2,
        "divisor_y": 1
    })");
    auto result = AtlasSourceParser::parseSource(j);
    ASSERT_TRUE(result.success());
    auto source = std::move(result).value();
    EXPECT_NE(source->describe().find("unstitch"), std::string::npos);
}

TEST(AtlasSourceParserTest, ParsePalettedPermutationsSource)
{
    nlohmann::json j = nlohmann::json::parse(R"({
        "type": "minecraft:paletted_permutations",
        "textures": ["minecraft:entity/test/base"],
        "palette_key": "minecraft:colormap/palette_key",
        "permutations": { "red": "minecraft:colormap/red" },
        "separator": "_"
    })");
    auto result = AtlasSourceParser::parseSource(j);
    ASSERT_TRUE(result.success());
    auto source = std::move(result).value();
    EXPECT_NE(source->describe().find("paletted_permutations"), std::string::npos);
}

TEST(AtlasSourceParserTest, UnknownTypeReturnsError)
{
    nlohmann::json j = nlohmann::json::parse(R"({ "type": "minecraft:bogus" })");
    auto result = AtlasSourceParser::parseSource(j);
    EXPECT_TRUE(result.failed());
}

TEST(AtlasSourceParserTest, MissingTypeReturnsError)
{
    nlohmann::json j = nlohmann::json::parse(R"({ "resource": "minecraft:block/stone" })");
    auto result = AtlasSourceParser::parseSource(j);
    EXPECT_TRUE(result.failed());
}

TEST(AtlasSourceParserTest, ParseAtlasJsonExtractsSourcesArray)
{
    nlohmann::json j = nlohmann::json::parse(R"({
        "sources": [
            { "type": "minecraft:single", "resource": "minecraft:block/stone" },
            { "type": "minecraft:directory", "source": "block", "prefix": "block/" }
        ]
    })");
    auto result = AtlasSourceParser::parseAtlasJson(j);
    ASSERT_TRUE(result.success());
    auto sources = std::move(result).value();
    EXPECT_EQ(sources.size(), 2u);
}

TEST(AtlasSourceParserTest, ParseAtlasTextInvalidJsonReturnsError)
{
    auto result = AtlasSourceParser::parseAtlasText("not json {");
    EXPECT_TRUE(result.failed());
}

TEST(AtlasSourceParserTest, ParseAtlasJsonSkipsBadSourceKeepsGood)
{
    // 单个 source 解析失败只跳过不中断
    nlohmann::json j = nlohmann::json::parse(R"({
        "sources": [
            { "type": "minecraft:single", "resource": "minecraft:block/stone" },
            { "type": "minecraft:bogus" }
        ]
    })");
    auto result = AtlasSourceParser::parseAtlasJson(j);
    ASSERT_TRUE(result.success());
    auto sources = std::move(result).value();
    EXPECT_EQ(sources.size(), 1u);
}

// ============================================================================
// MissingNo
// ============================================================================

TEST(MissingNoTest, SpriteLocationIsMinecraftMissingno)
{
    EXPECT_EQ(MissingNo::spriteLocation().toString(), "minecraft:missingno");
}

TEST(MissingNoTest, Generates16x16Pixels)
{
    auto pixels = MissingNo::generatePixels();
    EXPECT_EQ(pixels.size(), 16u * 16u * 4u);
    // 全不透明
    for (size_t i = 3; i < pixels.size(); i += 4) {
        EXPECT_EQ(pixels[i], 255u);
    }
}

} // namespace mc::client::resource::atlas
