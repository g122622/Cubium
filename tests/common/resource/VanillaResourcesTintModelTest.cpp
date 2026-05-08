#include <gtest/gtest.h>

#include "common/resource/VanillaResources.hpp"

#include <nlohmann/json.hpp>

namespace mc::test {
namespace {

nlohmann::json loadJsonFromPack(const InMemoryResourcePack& pack, std::string_view path) {
    const auto readResult = pack.readResource(path);
    EXPECT_TRUE(readResult.success()) << "missing resource: " << path;
    if (readResult.failed()) {
        return nlohmann::json::object();
    }

    const auto& bytes = readResult.value();
    const std::string content(bytes.begin(), bytes.end());
    return nlohmann::json::parse(content);
}

} // namespace

TEST(VanillaResourcesTintModelTest, GrassBlockTopFaceHasTintIndex) {
    auto pack = VanillaResources::createResourcePack();
    ASSERT_NE(pack, nullptr);

    const nlohmann::json model = loadJsonFromPack(
        *pack,
        "assets/minecraft/models/block/grass_block.json");

    ASSERT_TRUE(model.contains("elements"));
    ASSERT_TRUE(model["elements"].is_array());
    ASSERT_FALSE(model["elements"].empty());

    const auto& faces = model["elements"][0]["faces"];
    ASSERT_TRUE(faces.contains("up"));
    ASSERT_TRUE(faces["up"].contains("tintindex"));
    EXPECT_EQ(faces["up"]["tintindex"].get<i32>(), 0);
}

TEST(VanillaResourcesTintModelTest, GrassBlockBottomUsesDirtTexture) {
    auto pack = VanillaResources::createResourcePack();
    ASSERT_NE(pack, nullptr);

    const nlohmann::json model = loadJsonFromPack(
        *pack,
        "assets/minecraft/models/block/grass_block.json");

    ASSERT_TRUE(model.contains("textures"));
    ASSERT_TRUE(model["textures"].contains("bottom"));
    EXPECT_EQ(model["textures"]["bottom"].get<std::string>(), "block/dirt");

    ASSERT_TRUE(model.contains("elements"));
    ASSERT_TRUE(model["elements"].is_array());
    ASSERT_FALSE(model["elements"].empty());

    const auto& faces = model["elements"][0]["faces"];
    ASSERT_TRUE(faces.contains("down"));
    ASSERT_TRUE(faces["down"].contains("texture"));
    EXPECT_EQ(faces["down"]["texture"].get<std::string>(), "#bottom");
}

TEST(VanillaResourcesTintModelTest, GrassBlockSideOverlayHasTintIndex) {
    auto pack = VanillaResources::createResourcePack();
    ASSERT_NE(pack, nullptr);

    const nlohmann::json model = loadJsonFromPack(
        *pack,
        "assets/minecraft/models/block/grass_block.json");

    ASSERT_TRUE(model.contains("textures"));
    ASSERT_TRUE(model["textures"].contains("overlay"));
    EXPECT_EQ(model["textures"]["overlay"].get<std::string>(), "block/grass_block_side_overlay");

    ASSERT_TRUE(model.contains("elements"));
    ASSERT_TRUE(model["elements"].is_array());
    ASSERT_GE(model["elements"].size(), 2u);

    const auto& overlayFaces = model["elements"][1]["faces"];
    for (const char* face : {"north", "south", "west", "east"}) {
        ASSERT_TRUE(overlayFaces.contains(face));
        ASSERT_TRUE(overlayFaces[face].contains("texture"));
        ASSERT_TRUE(overlayFaces[face].contains("tintindex"));
        EXPECT_EQ(overlayFaces[face]["texture"].get<std::string>(), "#overlay");
        EXPECT_EQ(overlayFaces[face]["tintindex"].get<i32>(), 0);
    }
}

TEST(VanillaResourcesTintModelTest, LeavesModelHasTintIndexOnAllFaces) {
    auto pack = VanillaResources::createResourcePack();
    ASSERT_NE(pack, nullptr);

    const nlohmann::json model = loadJsonFromPack(
        *pack,
        "assets/minecraft/models/block/leaves.json");

    ASSERT_TRUE(model.contains("elements"));
    ASSERT_TRUE(model["elements"].is_array());
    ASSERT_FALSE(model["elements"].empty());

    const auto& faces = model["elements"][0]["faces"];
    for (const char* face : {"down", "up", "north", "south", "west", "east"}) {
        ASSERT_TRUE(faces.contains(face));
        ASSERT_TRUE(faces[face].contains("tintindex"));
        EXPECT_EQ(faces[face]["tintindex"].get<i32>(), 0);
    }
}

TEST(VanillaResourcesTintModelTest, ShortGrassAndFernUseTintedCrossParent) {
    auto pack = VanillaResources::createResourcePack();
    ASSERT_NE(pack, nullptr);

    for (const char* block : {"short_grass", "fern"}) {
        const std::string path = "assets/minecraft/models/block/" + std::string(block) + ".json";
        const nlohmann::json model = loadJsonFromPack(*pack, path);

        ASSERT_TRUE(model.contains("parent"));
        EXPECT_EQ(model["parent"].get<std::string>(), "block/tinted_cross");
    }
}

TEST(VanillaResourcesTintModelTest, TallGrassUsesTintedCrossParent) {
    auto pack = VanillaResources::createResourcePack();
    ASSERT_NE(pack, nullptr);

    const nlohmann::json model = loadJsonFromPack(
        *pack,
        "assets/minecraft/models/block/tall_grass.json");

    ASSERT_TRUE(model.contains("parent"));
    EXPECT_EQ(model["parent"].get<std::string>(), "block/tinted_cross");
}

} // namespace mc::test
