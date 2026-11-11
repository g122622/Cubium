#include <gtest/gtest.h>

#include "common/core/GameDirectory.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "server/world/gen/feature/FeatureTypeRegistry.hpp"
#include "server/world/gen/feature/tree/TreeFeature.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <memory>

namespace {

class MangroveRootPlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::VanillaBlocks::initialize();
        // configured_feature 的 type 工厂需要先注册，否则 create() 直接报未注册。
        mc::world::gen::feature::initializeBuiltinFeatureTypes();
    }
};

/// 读取原版数据包里的 mangrove.json（不手写 JSON，避免与数据包漂移）
nlohmann::json readMangroveConfiguredFeature()
{
    // 用项目的 GameDirectory 取数据包目录，而不是猜路径。
    const auto path = mc::GameDirectory::defaultDirectory().dataPacksDir() / "Vanilla" / "data" / "minecraft" /
        "worldgen" / "configured_feature" / "mangrove.json";
    std::ifstream input(path);
    if (!input.is_open()) {
        return nlohmann::json();
    }
    return nlohmann::json::parse(input);
}

} // namespace

// 数据包以方块列表给出 muddy_roots_in 时，必须被登记成可查询的标签，
// 否则 TreeFeature 的泥根替换永远不生效（旧实现靠硬编码 MUD/MUDDY_MANGROVE_ROOTS 兜底）。
TEST_F(MangroveRootPlacementTest, BlockListFormPopulatesMuddyRootsTag)
{
    const auto datapackJson = readMangroveConfiguredFeature();
    if (datapackJson.is_null() || !datapackJson.contains("config")) {
        GTEST_SKIP() << "vanilla datapack mangrove.json is not available on this machine";
    }

    auto result =
        mc::world::gen::feature::FeatureTypeRegistry::instance().create("minecraft:tree", datapackJson["config"]);
    ASSERT_TRUE(result.success()) << result.error().message();

    auto* configured = dynamic_cast<mc::ConfiguredTreeFeature*>(result.value().get());
    ASSERT_NE(configured, nullptr);
    const auto& config = configured->getConfig();
    ASSERT_NE(config.rootPlacer, nullptr) << "mangrove must carry a root placer";
    ASSERT_NE(config.rootPlacer->mangroveRootPlacement, nullptr);

    const auto* muddyTag = config.rootPlacer->mangroveRootPlacement->muddyRootsIn;
    ASSERT_NE(muddyTag, nullptr) << "muddy_roots_in block list must be registered as a queryable tag";

    // 数据包列表里的两个方块都必须命中
    ASSERT_NE(mc::VanillaBlocks::MUD, nullptr);
    ASSERT_NE(mc::VanillaBlocks::MUDDY_MANGROVE_ROOTS, nullptr);
    EXPECT_TRUE(muddyTag->contains(mc::VanillaBlocks::MUD->defaultState()))
        << "mud must be contained in the muddy_roots_in tag";
    EXPECT_TRUE(muddyTag->contains(mc::VanillaBlocks::MUDDY_MANGROVE_ROOTS->defaultState()))
        << "muddy_mangrove_roots must be contained in the muddy_roots_in tag";

    // 反向：不在列表里的方块不应命中，防止标签被过度填充
    ASSERT_NE(mc::VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(muddyTag->contains(mc::VanillaBlocks::STONE->defaultState()));
}
