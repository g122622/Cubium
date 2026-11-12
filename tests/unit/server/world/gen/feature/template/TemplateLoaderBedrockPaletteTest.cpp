#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "server/world/gen/feature/template/TemplateLoader.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

// 拉杆的 direction 是基岩 LeverDirection（0-7），一个值同时编码附着面与朝向，
// Java 侧要拆成 ATTACH_FACE + HORIZONTAL_FACING 两个属性。
TEST_F(TemplateLoaderBedrockPaletteTest, LeverDirectionSplitsIntoFaceAndFacing)
{
    ASSERT_NE(mc::VanillaBlocks::LEVER, nullptr) << "lever must be registered";

    struct Expectation {
        mc::i32 direction;
        mc::BlockStateProperties::AttachFace face;
        mc::Direction facing;
    };

    // 逐值对照基岩 LeverDirection 枚举（含相邻值语义相反的 UpEastWest / DownNorthSouth）。
    const std::array<Expectation, 8> expectations = {
        {{0, mc::BlockStateProperties::AttachFace::Ceiling, mc::Direction::North},
            {1, mc::BlockStateProperties::AttachFace::Wall, mc::Direction::East},
            {2, mc::BlockStateProperties::AttachFace::Wall, mc::Direction::West},
            {3, mc::BlockStateProperties::AttachFace::Wall, mc::Direction::South},
            {4, mc::BlockStateProperties::AttachFace::Wall, mc::Direction::North},
            {5, mc::BlockStateProperties::AttachFace::Floor, mc::Direction::North},
            {6, mc::BlockStateProperties::AttachFace::Floor, mc::Direction::East},
            {7, mc::BlockStateProperties::AttachFace::Ceiling, mc::Direction::North}}};

    for (const auto& expected : expectations) {
        auto entry = std::make_unique<mc::nbt::tags::compound_tag>();
        entry->put("name", std::string("minecraft:lever"));
        auto states = std::make_unique<mc::nbt::tags::compound_tag>();
        states->put("direction", expected.direction);
        entry->value["states"] = std::move(states);

        const mc::u32 id = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*entry);
        const auto* state = mc::BlockRegistry::instance().getBlockState(id);
        ASSERT_NE(state, nullptr) << "direction=" << expected.direction;
        EXPECT_EQ(state->get(mc::BlockStateProperties::ATTACH_FACE()), expected.face)
            << "direction=" << expected.direction;
        EXPECT_EQ(state->get(mc::BlockStateProperties::HORIZONTAL_FACING()), expected.facing)
            << "direction=" << expected.direction;
    }
}

// 非拉杆方块走原 CommonDirection 映射，方向的成绩不受拉杆分支影响。
TEST_F(TemplateLoaderBedrockPaletteTest, DoorDirectionKeepsCommonDirectionMapping)
{
    ASSERT_NE(mc::VanillaBlocks::OAK_DOOR, nullptr) << "oak_door must be registered";

    auto entry = std::make_unique<mc::nbt::tags::compound_tag>();
    entry->put("name", std::string("minecraft:wooden_door"));
    auto states = std::make_unique<mc::nbt::tags::compound_tag>();
    states->put("direction", 2);
    entry->value["states"] = std::move(states);

    const mc::u32 id = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*entry);
    const auto* state = mc::BlockRegistry::instance().getBlockState(id);
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->get(mc::BlockStateProperties::HORIZONTAL_FACING()), mc::Direction::South);
}

/// 构造带字符串状态值的基岩 block_palette 项
std::unique_ptr<mc::nbt::tags::compound_tag> makeBedrockPaletteEntryWithString(
    const std::string& name, const std::vector<std::pair<std::string, std::string>>& stringStates)
{
    auto entry = std::make_unique<mc::nbt::tags::compound_tag>();
    entry->put("name", name);

    auto states = std::make_unique<mc::nbt::tags::compound_tag>();
    for (const auto& [key, value] : stringStates) {
        states->put(key, value);
    }
    entry->value["states"] = std::move(states);
    return entry;
}

// 基岩把全部树苗收在 minecraft:sapling 里，树种由 sapling_type 决定；Java 每个树种一个方块。
TEST_F(TemplateLoaderBedrockPaletteTest, SaplingTypeMapsToPerSpeciesBlock)
{
    ASSERT_NE(mc::VanillaBlocks::OAK_SAPLING, nullptr);
    ASSERT_NE(mc::VanillaBlocks::BIRCH_SAPLING, nullptr);
    ASSERT_NE(mc::VanillaBlocks::DARK_OAK_SAPLING, nullptr);

    const std::vector<std::pair<std::string, mc::Block*>> cases = {
        {"oak", mc::VanillaBlocks::OAK_SAPLING},
        {"birch", mc::VanillaBlocks::BIRCH_SAPLING},
        {"dark_oak", mc::VanillaBlocks::DARK_OAK_SAPLING},
    };
    for (const auto& [bedrockType, expectedBlock] : cases) {
        const auto entry = makeBedrockPaletteEntryWithString("minecraft:sapling", {{"sapling_type", bedrockType}});
        const mc::u32 id = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*entry);
        const auto* state = mc::BlockRegistry::instance().getBlockState(id);
        ASSERT_NE(state, nullptr) << "sapling_type=" << bedrockType;
        EXPECT_EQ(&state->getBlock(), expectedBlock) << "sapling_type=" << bedrockType;
    }
}

// 未知树种不能静默变成某一种树苗：没有可解析的方块名时回退为空气。
TEST_F(TemplateLoaderBedrockPaletteTest, UnknownSaplingTypeFallsBackToAir)
{
    const auto entry = makeBedrockPaletteEntryWithString("minecraft:sapling", {{"sapling_type", "not_a_tree"}});
    const mc::u32 id = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*entry);
    const auto* state = mc::BlockRegistry::instance().getBlockState(id);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir()) << "unknown sapling_type must not resolve to a real block";
}

// 酿造台药水槽占用位：基岩的 brewing_stand_slot_a/b/c_bit 对应 Java 的 has_bottle_0/1/2。
TEST_F(TemplateLoaderBedrockPaletteTest, BrewingStandSlotBitsMapToHasBottle)
{
    ASSERT_NE(mc::VanillaBlocks::BREWING_STAND, nullptr) << "brewing_stand must be registered";

    struct Expectation {
        const char* bedrockKey;
        // 该位为 1 时 Java 侧应为 true 的属性
        bool expect0;
        bool expect1;
        bool expect2;
    };

    const std::array<Expectation, 3> expectations = {{{"brewing_stand_slot_a_bit", true, false, false},
        {"brewing_stand_slot_b_bit", false, true, false},
        {"brewing_stand_slot_c_bit", false, false, true}}};

    for (const auto& expected : expectations) {
        auto entry = std::make_unique<mc::nbt::tags::compound_tag>();
        entry->put("name", std::string("minecraft:brewing_stand"));
        auto states = std::make_unique<mc::nbt::tags::compound_tag>();
        states->put(expected.bedrockKey, static_cast<mc::i8>(1));
        entry->value["states"] = std::move(states);

        const mc::u32 id = mc::test::TemplateLoaderTestAccessor::parseBedrockPaletteEntry(*entry);
        const auto* state = mc::BlockRegistry::instance().getBlockState(id);
        ASSERT_NE(state, nullptr) << "key=" << expected.bedrockKey;
        EXPECT_EQ(state->get(mc::BlockStateProperties::HAS_BOTTLE_0()), expected.expect0)
            << "key=" << expected.bedrockKey;
        EXPECT_EQ(state->get(mc::BlockStateProperties::HAS_BOTTLE_1()), expected.expect1)
            << "key=" << expected.bedrockKey;
        EXPECT_EQ(state->get(mc::BlockStateProperties::HAS_BOTTLE_2()), expected.expect2)
            << "key=" << expected.bedrockKey;
    }
}

// 布尔属性的值索引必须自洽：valueToString(i) 解析回来的索引要等于 i。
// 这条约束是 applyPropertiesToState 按「字符串 → 索引」匹配候选状态的前提。
TEST_F(TemplateLoaderBedrockPaletteTest, BooleanPropertyIndexRoundTrips)
{
    const mc::IProperty& prop = mc::BlockStateProperties::HAS_BOTTLE_0();
    EXPECT_EQ(prop.valueCount(), 2u);
    EXPECT_EQ(prop.valueToString(0), "true");
    EXPECT_EQ(prop.valueToString(1), "false");

    const auto zero = prop.parseValue(prop.valueToString(0));
    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(*zero, 0u) << "valueToString(0) must parse back to index 0";

    const auto one = prop.parseValue(prop.valueToString(1));
    ASSERT_TRUE(one.has_value());
    EXPECT_EQ(*one, 1u) << "valueToString(1) must parse back to index 1";
}
