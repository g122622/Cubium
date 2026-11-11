#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "server/world/gen/feature/template/TemplateLoader.hpp"

#include <memory>
#include <string>

namespace mc::test {

/**
 * @brief TemplateLoader 私有解析函数的测试访问器
 *
 * 基岩 palette 项到方块状态 id 的解析是私有静态函数，但它承载了
 * "同一基岩属性名在不同方块上语义不同"的关键分派逻辑（如拉杆的
 * open_bit → powered），需要独立于世界/结构文件进行验证。
 */
class TemplateLoaderTestAccessor {
public:
    static u32 parseBedrockPaletteEntry(const nbt::tags::compound_tag& entry)
    {
        return world::gen::feature::template_::TemplateLoader::_parseBedrockBlockStateId(entry);
    }
};

} // namespace mc::test

namespace {

/// 构造一个基岩 block_palette 项：{ name: String, states: Compound }
std::unique_ptr<mc::nbt::tags::compound_tag> makeBedrockPaletteEntry(
    const std::string& name, const std::vector<std::pair<std::string, mc::i8>>& byteStates)
{
    auto entry = std::make_unique<mc::nbt::tags::compound_tag>();
    entry->put("name", name);

    auto states = std::make_unique<mc::nbt::tags::compound_tag>();
    for (const auto& [key, value] : byteStates) {
        states->put(key, value);
    }
    // compound_tag::put 的 tag_of<T> 不支持 unique_ptr<compound_tag>，子 Compound 直接挂到 value 映射上。
    entry->value["states"] = std::move(states);
    return entry;
}

class TemplateLoaderBedrockPaletteTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::VanillaBlocks::initialize();
    }
};

} // namespace

// 拉杆的 open_bit 必须映射到 Java 的 powered，而不是一个不存在的 open 属性。
// 这是本轮修复的核心：Java LeverBlock 的属性是 HORIZONTAL_FACING + ATTACH_FACE + POWERED，
// 没有 open，沿用通用映射会让通断状态被静默丢弃。
TEST_F(TemplateLoaderBedrockPaletteTest, LeverOpenBitMapsToPowered)
{
    ASSERT_NE(mc::VanillaBlocks::LEVER, nullptr) << "lever must be registered";

    // 通断位 = 1：必须落到 Java 的 POWERED=true。
    const auto onEntry = makeBedrockPaletteEntry("minecraft:lever", {{"open_bit", 1}});
    const mc::u32 onId = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*onEntry);
    const auto* onState = mc::BlockRegistry::instance().getBlockState(onId);
    ASSERT_NE(onState, nullptr);
    EXPECT_EQ(onState->get(mc::BlockStateProperties::POWERED()), true)
        << "open_bit=1 must map to powered=true on lever";

    // 通断位 = 0：POWERED=false。
    const auto offEntry = makeBedrockPaletteEntry("minecraft:lever", {{"open_bit", 0}});
    const mc::u32 offId = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*offEntry);
    const auto* offState = mc::BlockRegistry::instance().getBlockState(offId);
    ASSERT_NE(offState, nullptr);
    EXPECT_EQ(offState->get(mc::BlockStateProperties::POWERED()), false)
        << "open_bit=0 must map to powered=false on lever";
}

// 门仍然走 open 映射——确认按方块分派没有把通用情形改坏。
TEST_F(TemplateLoaderBedrockPaletteTest, DoorOpenBitStillMapsToOpen)
{
    ASSERT_NE(mc::VanillaBlocks::OAK_DOOR, nullptr) << "oak_door must be registered";

    const auto entry = makeBedrockPaletteEntry("minecraft:wooden_door", {{"open_bit", 1}});
    const mc::u32 id = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*entry);
    const auto* state = mc::BlockRegistry::instance().getBlockState(id);
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get(mc::BlockStateProperties::OPEN()), true) << "door open_bit must still map to open=true";
}
