#include <gtest/gtest.h>

#include "client/resource/ResourceManager.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/world/block/VanillaBlocks.hpp"

#include <unordered_map>

namespace mc::test {
namespace {

class InMemoryResourcePack final : public IResourcePack {
public:
    InMemoryResourcePack() = default;

    Result<void> initialize() override {
        return Result<void>::ok();
    }

    [[nodiscard]] const PackMetadata& metadata() const override {
        return m_metadata;
    }

    [[nodiscard]] bool hasResource(StringView resourcePath) const override {
        return m_resources.find(String(resourcePath)) != m_resources.end();
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(StringView resourcePath) const override {
        auto it = m_resources.find(String(resourcePath));
        if (it == m_resources.end()) {
            return Error(ErrorCode::NotFound, "Resource not found");
        }
        return it->second;
    }

    [[nodiscard]] Result<std::vector<String>> listResources(
        StringView directory,
        StringView extension) const override {
        std::vector<String> result;
        const String dirPrefix(directory);
        const String ext(extension);
        for (const auto& [path, _] : m_resources) {
            const bool inDir = dirPrefix.empty() || path.rfind(dirPrefix, 0) == 0;
            const bool extMatch = ext.empty() || (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext);
            if (inDir && extMatch) {
                result.push_back(path);
            }
        }
        return result;
    }

    [[nodiscard]] String name() const override {
        return "InMemoryResourcePack";
    }

    void add(String path, std::vector<u8> bytes) {
        m_resources.emplace(std::move(path), std::move(bytes));
    }

private:
    PackMetadata m_metadata{6, "test-pack"};
    std::unordered_map<String, std::vector<u8>> m_resources;
};

std::vector<u8> makeValid1x1Png() {
    return {
        137, 80, 78, 71, 13, 10, 26, 10,
        0, 0, 0, 13, 73, 72, 68, 82,
        0, 0, 0, 1, 0, 0, 0, 1,
        8, 4, 0, 0, 0, 181, 28, 12, 2,
        0, 0, 0, 11, 73, 68, 65, 84,
        120, 218, 99, 252, 255, 31, 0, 3,
        3, 2, 0, 239, 156, 7, 219,
        0, 0, 0, 0, 73, 69, 78, 68,
        174, 66, 96, 130
    };
}

std::vector<u8> toBytes(StringView content) {
    return std::vector<u8>(content.begin(), content.end());
}

} // namespace

TEST(ResourceManagerTextureDecodeTest, LoadCloudTextureFromResourcePack) {
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

TEST(ResourceManagerTextureDecodeTest, ReturnNotFoundWhenCloudTextureMissing) {
    ResourceManager manager;

    auto pack = std::make_shared<InMemoryResourcePack>();
    auto addResult = manager.addResourcePack(pack);
    ASSERT_TRUE(addResult.success());

    auto decodedResult = manager.loadTextureRGBA(ResourceLocation("minecraft:textures/environment/clouds"));
    ASSERT_TRUE(decodedResult.failed());
}

TEST(ResourceManagerTextureDecodeTest, WaterModelWithParticleTextureKeepsParticleOnlyAppearance) {
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
        ASSERT_TRUE(manager.buildTextureAtlas().success());

        const auto* appearance = manager.getBlockAppearance(ResourceLocation("minecraft:water"));
        ASSERT_NE(appearance, nullptr);
        EXPECT_TRUE(appearance->faceTextures.empty());
        EXPECT_TRUE(appearance->faceTextureLayers.empty());
}

} // namespace mc::test
