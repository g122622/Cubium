#include <gtest/gtest.h>
#include "../src/common/util/property/IProperty.hpp"
#include "../src/common/util/property/Property.hpp"
#include "../src/common/util/property/BooleanProperty.hpp"
#include "../src/common/util/property/IntegerProperty.hpp"
#include "../src/common/util/property/EnumProperty.hpp"
#include "../src/common/util/property/DirectionProperty.hpp"
#include "../src/common/util/property/Properties.hpp"
#include "../src/common/util/Direction.hpp"

using namespace mc;

// ============================================================================
// BooleanProperty 测试
// ============================================================================

TEST(BooleanPropertyTest, CreateAndBasicOperations) {
    auto prop = BooleanProperty::create("lit");

    EXPECT_EQ(prop->name(), "lit");
    EXPECT_EQ(prop->valueCount(), 2);
    EXPECT_STREQ(prop->typeName(), "BooleanProperty");
}

TEST(BooleanPropertyTest, AllowedValues) {
    auto prop = BooleanProperty::create("powered");

    const auto& values = prop->allowedValues();
    ASSERT_EQ(values.size(), 2);
    EXPECT_FALSE(values[0]);  // false comes first
    EXPECT_TRUE(values[1]);   // true comes second
}

TEST(BooleanPropertyTest, IndexOf) {
    auto prop = BooleanProperty::create("open");

    auto falseIdx = prop->indexOf(false);
    auto trueIdx = prop->indexOf(true);

    ASSERT_TRUE(falseIdx.has_value());
    ASSERT_TRUE(trueIdx.has_value());
    EXPECT_EQ(falseIdx.value(), 0);
    EXPECT_EQ(trueIdx.value(), 1);
}

TEST(BooleanPropertyTest, ValueAt) {
    auto prop = BooleanProperty::create("waterlogged");

    EXPECT_FALSE(prop->valueAt(0));
    EXPECT_TRUE(prop->valueAt(1));
}

TEST(BooleanPropertyTest, ValueToString) {
    auto prop = BooleanProperty::create("enabled");

    EXPECT_EQ(prop->valueToString(false), "false");
    EXPECT_EQ(prop->valueToString(true), "true");
    EXPECT_EQ(prop->valueToString(0), "false");
    EXPECT_EQ(prop->valueToString(1), "true");
}

TEST(BooleanPropertyTest, ParseValue) {
    auto prop = BooleanProperty::create("active");

    auto trueValue = prop->parse("true");
    auto falseValue = prop->parse("false");
    auto invalid = prop->parse("invalid");

    ASSERT_TRUE(trueValue.has_value());
    EXPECT_TRUE(trueValue.value());

    ASSERT_TRUE(falseValue.has_value());
    EXPECT_FALSE(falseValue.value());

    EXPECT_FALSE(invalid.has_value());
}

TEST(BooleanPropertyTest, ParseValueIndex) {
    auto prop = BooleanProperty::create("snowy");

    auto trueIdx = prop->parseValue(std::string_view("true"));
    auto falseIdx = prop->parseValue(std::string_view("false"));
    auto invalid = prop->parseValue(std::string_view("yes"));

    ASSERT_TRUE(trueIdx.has_value());
    EXPECT_EQ(trueIdx.value(), 1);

    ASSERT_TRUE(falseIdx.has_value());
    EXPECT_EQ(falseIdx.value(), 0);

    EXPECT_FALSE(invalid.has_value());
}

TEST(BooleanPropertyTest, Equals) {
    auto prop1 = BooleanProperty::create("lit");
    auto prop2 = BooleanProperty::create("lit");
    auto prop3 = BooleanProperty::create("powered");

    EXPECT_TRUE(prop1->equals(*prop2));
    EXPECT_FALSE(prop1->equals(*prop3));
    EXPECT_TRUE(prop1->equals(*prop1));
}

TEST(BooleanPropertyTest, HashCode) {
    auto prop1 = BooleanProperty::create("lit");
    auto prop2 = BooleanProperty::create("lit");

    EXPECT_EQ(prop1->hashCode(), prop2->hashCode());
}

// ============================================================================
// IntegerProperty 测试
// ============================================================================

TEST(IntegerPropertyTest, CreateAndBasicOperations) {
    auto prop = IntegerProperty::create("age", 0, 7);

    EXPECT_EQ(prop->name(), "age");
    EXPECT_EQ(prop->valueCount(), 8);
    EXPECT_EQ(prop->minValue(), 0);
    EXPECT_EQ(prop->maxValue(), 7);
    EXPECT_STREQ(prop->typeName(), "IntegerProperty");
}

TEST(IntegerPropertyTest, AllowedValues) {
    auto prop = IntegerProperty::create("power", 0, 15);

    const auto& values = prop->allowedValues();
    ASSERT_EQ(values.size(), 16);

    for (int i = 0; i <= 15; ++i) {
        EXPECT_EQ(values[i], i);
    }
}

TEST(IntegerPropertyTest, IndexOf) {
    auto prop = IntegerProperty::create("level", 0, 8);

    auto idx0 = prop->indexOf(0);
    auto idx4 = prop->indexOf(4);
    auto idx8 = prop->indexOf(8);
    auto invalid = prop->indexOf(9);
    auto invalid2 = prop->indexOf(-1);

    ASSERT_TRUE(idx0.has_value());
    EXPECT_EQ(idx0.value(), 0);

    ASSERT_TRUE(idx4.has_value());
    EXPECT_EQ(idx4.value(), 4);

    ASSERT_TRUE(idx8.has_value());
    EXPECT_EQ(idx8.value(), 8);

    EXPECT_FALSE(invalid.has_value());
    EXPECT_FALSE(invalid2.has_value());
}

TEST(IntegerPropertyTest, ValueToString) {
    auto prop = IntegerProperty::create("note", 0, 24);

    EXPECT_EQ(prop->valueToString(0), "0");
    EXPECT_EQ(prop->valueToString(12), "12");
    EXPECT_EQ(prop->valueToString(24), "24");
}

TEST(IntegerPropertyTest, ParseValue) {
    auto prop = IntegerProperty::create("delay", 1, 4);

    auto val1 = prop->parse("1");
    auto val4 = prop->parse("4");
    auto invalid1 = prop->parse("0");   // out of range
    auto invalid2 = prop->parse("5");   // out of range
    auto invalid3 = prop->parse("abc"); // not a number

    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(val1.value(), 1);

    ASSERT_TRUE(val4.has_value());
    EXPECT_EQ(val4.value(), 4);

    EXPECT_FALSE(invalid1.has_value());
    EXPECT_FALSE(invalid2.has_value());
    EXPECT_FALSE(invalid3.has_value());
}

TEST(IntegerPropertyTest, InvalidRange) {
    // min < 0 should throw
    EXPECT_THROW(IntegerProperty::create("invalid1", -1, 5), std::invalid_argument);

    // max <= min should throw
    EXPECT_THROW(IntegerProperty::create("invalid2", 5, 5), std::invalid_argument);
    EXPECT_THROW(IntegerProperty::create("invalid3", 5, 3), std::invalid_argument);
}

TEST(IntegerPropertyTest, Equals) {
    auto prop1 = IntegerProperty::create("age", 0, 7);
    auto prop2 = IntegerProperty::create("age", 0, 7);
    auto prop3 = IntegerProperty::create("age", 0, 15);
    auto prop4 = IntegerProperty::create("power", 0, 7);

    EXPECT_TRUE(prop1->equals(*prop2));
    EXPECT_FALSE(prop1->equals(*prop3));  // different range
    EXPECT_FALSE(prop1->equals(*prop4));  // different name
}

// ============================================================================
// EnumProperty<Direction> 测试
// ============================================================================

TEST(EnumPropertyDirectionTest, CreateAll) {
    auto prop = DirectionProperty::create("facing");

    EXPECT_EQ(prop->name(), "facing");
    EXPECT_EQ(prop->valueCount(), 6);
}

TEST(EnumPropertyDirectionTest, ValueToString) {
    auto prop = DirectionProperty::create("facing");

    EXPECT_EQ(prop->valueToString(Direction::Down), "down");
    EXPECT_EQ(prop->valueToString(Direction::Up), "up");
    EXPECT_EQ(prop->valueToString(Direction::North), "north");
    EXPECT_EQ(prop->valueToString(Direction::South), "south");
    EXPECT_EQ(prop->valueToString(Direction::West), "west");
    EXPECT_EQ(prop->valueToString(Direction::East), "east");
}

TEST(EnumPropertyDirectionTest, ParseValue) {
    auto prop = DirectionProperty::create("facing");

    auto north = prop->parse("north");
    auto south = prop->parse("south");
    auto invalid = prop->parse("invalid");

    ASSERT_TRUE(north.has_value());
    EXPECT_EQ(north.value(), Direction::North);

    ASSERT_TRUE(south.has_value());
    EXPECT_EQ(south.value(), Direction::South);

    EXPECT_FALSE(invalid.has_value());
}

TEST(EnumPropertyDirectionTest, Subset) {
    auto prop = DirectionProperty::create("horizontal_facing",
        {Direction::North, Direction::South, Direction::East, Direction::West});

    EXPECT_EQ(prop->valueCount(), 4);

    auto upIdx = prop->indexOf(Direction::Up);
    EXPECT_FALSE(upIdx.has_value());  // Up not in subset

    auto northIdx = prop->indexOf(Direction::North);
    ASSERT_TRUE(northIdx.has_value());
}

// ============================================================================
// EnumProperty<Axis> 测试
// ============================================================================

TEST(EnumPropertyAxisTest, CreateAll) {
    auto prop = AxisProperty::create("axis");

    EXPECT_EQ(prop->name(), "axis");
    EXPECT_EQ(prop->valueCount(), 3);
}

TEST(EnumPropertyAxisTest, ValueToString) {
    auto prop = AxisProperty::create("axis");

    EXPECT_EQ(prop->valueToString(Axis::X), "x");
    EXPECT_EQ(prop->valueToString(Axis::Y), "y");
    EXPECT_EQ(prop->valueToString(Axis::Z), "z");
}

TEST(EnumPropertyAxisTest, ParseValue) {
    auto prop = AxisProperty::create("axis");

    auto x = prop->parse("x");
    auto y = prop->parse("y");
    auto z = prop->parse("z");
    auto invalid = prop->parse("w");

    ASSERT_TRUE(x.has_value());
    EXPECT_EQ(x.value(), Axis::X);

    ASSERT_TRUE(y.has_value());
    EXPECT_EQ(y.value(), Axis::Y);

    ASSERT_TRUE(z.has_value());
    EXPECT_EQ(z.value(), Axis::Z);

    EXPECT_FALSE(invalid.has_value());
}

// ============================================================================
// DirectionProperty 测试
// ============================================================================

TEST(DirectionPropertyTest, CreateAll) {
    auto prop = DirectionProperty::create("facing");

    EXPECT_EQ(prop->name(), "facing");
    EXPECT_EQ(prop->valueCount(), 6);
    EXPECT_STREQ(prop->typeName(), "DirectionProperty");
}

TEST(DirectionPropertyTest, CreateHorizontal) {
    auto prop = DirectionProperty::createHorizontal("facing");

    EXPECT_EQ(prop->valueCount(), 4);

    // Should not contain Up or Down
    auto upIdx = prop->indexOf(Direction::Up);
    auto downIdx = prop->indexOf(Direction::Down);
    EXPECT_FALSE(upIdx.has_value());
    EXPECT_FALSE(downIdx.has_value());

    // Should contain horizontal directions
    auto northIdx = prop->indexOf(Direction::North);
    auto southIdx = prop->indexOf(Direction::South);
    auto eastIdx = prop->indexOf(Direction::East);
    auto westIdx = prop->indexOf(Direction::West);

    EXPECT_TRUE(northIdx.has_value());
    EXPECT_TRUE(southIdx.has_value());
    EXPECT_TRUE(eastIdx.has_value());
    EXPECT_TRUE(westIdx.has_value());
}

TEST(DirectionPropertyTest, CreateWithFilter) {
    auto prop = DirectionProperty::create("facing", [](Direction d) {
        return d != Direction::Up;
    });

    EXPECT_EQ(prop->valueCount(), 5);

    auto upIdx = prop->indexOf(Direction::Up);
    EXPECT_FALSE(upIdx.has_value());
}

// ============================================================================
// BlockStateProperties 测试
// ============================================================================

TEST(BlockStatePropertiesTest, BooleanProperties) {
    // 测试各种布尔属性
    EXPECT_EQ(BlockStateProperties::LIT().name(), "lit");
    EXPECT_EQ(BlockStateProperties::POWERED().name(), "powered");
    EXPECT_EQ(BlockStateProperties::OPEN().name(), "open");
    EXPECT_EQ(BlockStateProperties::WATERLOGGED().name(), "waterlogged");

    // 验证值数量
    EXPECT_EQ(BlockStateProperties::LIT().valueCount(), 2);
}

TEST(BlockStatePropertiesTest, DirectionProperties) {
    EXPECT_EQ(BlockStateProperties::FACING().name(), "facing");
    EXPECT_EQ(BlockStateProperties::FACING().valueCount(), 6);

    EXPECT_EQ(BlockStateProperties::HORIZONTAL_FACING().name(), "facing");
    EXPECT_EQ(BlockStateProperties::HORIZONTAL_FACING().valueCount(), 4);
}

TEST(BlockStatePropertiesTest, AxisProperties) {
    EXPECT_EQ(BlockStateProperties::AXIS().name(), "axis");
    EXPECT_EQ(BlockStateProperties::AXIS().valueCount(), 3);

    EXPECT_EQ(BlockStateProperties::HORIZONTAL_AXIS().name(), "axis");
    EXPECT_EQ(BlockStateProperties::HORIZONTAL_AXIS().valueCount(), 2);
}

TEST(BlockStatePropertiesTest, IntegerProperties) {
    EXPECT_EQ(BlockStateProperties::AGE_0_15().name(), "age");
    EXPECT_EQ(BlockStateProperties::AGE_0_15().valueCount(), 16);

    EXPECT_EQ(BlockStateProperties::POWER_0_15().name(), "power");
    EXPECT_EQ(BlockStateProperties::POWER_0_15().valueCount(), 16);

    EXPECT_EQ(BlockStateProperties::LEVEL_0_8().name(), "level");
    EXPECT_EQ(BlockStateProperties::LEVEL_0_8().valueCount(), 9);
}

// ============================================================================
// Direction 工具函数测试
// ============================================================================

TEST(DirectionUtilTest, AllDirections) {
    auto all = Directions::all();
    ASSERT_EQ(all.size(), 6);
}

TEST(DirectionUtilTest, HorizontalDirections) {
    auto horiz = Directions::horizontal();
    ASSERT_EQ(horiz.size(), 4);
}

TEST(DirectionUtilTest, Opposite) {
    EXPECT_EQ(Directions::opposite(Direction::Down), Direction::Up);
    EXPECT_EQ(Directions::opposite(Direction::Up), Direction::Down);
    EXPECT_EQ(Directions::opposite(Direction::North), Direction::South);
    EXPECT_EQ(Directions::opposite(Direction::South), Direction::North);
    EXPECT_EQ(Directions::opposite(Direction::West), Direction::East);
    EXPECT_EQ(Directions::opposite(Direction::East), Direction::West);
}

TEST(DirectionUtilTest, Offset) {
    EXPECT_EQ(Directions::xOffset(Direction::East), 1);
    EXPECT_EQ(Directions::xOffset(Direction::West), -1);
    EXPECT_EQ(Directions::xOffset(Direction::Up), 0);

    EXPECT_EQ(Directions::yOffset(Direction::Up), 1);
    EXPECT_EQ(Directions::yOffset(Direction::Down), -1);

    EXPECT_EQ(Directions::zOffset(Direction::South), 1);
    EXPECT_EQ(Directions::zOffset(Direction::North), -1);
}

TEST(DirectionUtilTest, Axis) {
    EXPECT_EQ(Directions::getAxis(Direction::Up), Axis::Y);
    EXPECT_EQ(Directions::getAxis(Direction::Down), Axis::Y);
    EXPECT_EQ(Directions::getAxis(Direction::East), Axis::X);
    EXPECT_EQ(Directions::getAxis(Direction::West), Axis::X);
    EXPECT_EQ(Directions::getAxis(Direction::North), Axis::Z);
    EXPECT_EQ(Directions::getAxis(Direction::South), Axis::Z);
}

TEST(DirectionUtilTest, RotateY) {
    EXPECT_EQ(Directions::rotateY(Direction::North), Direction::East);
    EXPECT_EQ(Directions::rotateY(Direction::East), Direction::South);
    EXPECT_EQ(Directions::rotateY(Direction::South), Direction::West);
    EXPECT_EQ(Directions::rotateY(Direction::West), Direction::North);
    // Vertical directions should not change
    EXPECT_EQ(Directions::rotateY(Direction::Up), Direction::Up);
    EXPECT_EQ(Directions::rotateY(Direction::Down), Direction::Down);
}

TEST(DirectionUtilTest, FromName) {
    EXPECT_EQ(Directions::fromName("down"), Direction::Down);
    EXPECT_EQ(Directions::fromName("up"), Direction::Up);
    EXPECT_EQ(Directions::fromName("north"), Direction::North);
    EXPECT_EQ(Directions::fromName("south"), Direction::South);
    EXPECT_EQ(Directions::fromName("east"), Direction::East);
    EXPECT_EQ(Directions::fromName("west"), Direction::West);
    EXPECT_FALSE(Directions::fromName("invalid").has_value());
}

TEST(DirectionUtilTest, ToString) {
    EXPECT_EQ(Directions::toString(Direction::Down), "down");
    EXPECT_EQ(Directions::toString(Direction::Up), "up");
    EXPECT_EQ(Directions::toString(Direction::North), "north");
}

// ============================================================================
// Axis 工具函数测试
// ============================================================================

TEST(AxisUtilTest, AllAxes) {
    auto all = Axes::all();
    ASSERT_EQ(all.size(), 3);
}

TEST(AxisUtilTest, FromName) {
    EXPECT_EQ(Axes::fromName("x"), Axis::X);
    EXPECT_EQ(Axes::fromName("y"), Axis::Y);
    EXPECT_EQ(Axes::fromName("z"), Axis::Z);
    EXPECT_FALSE(Axes::fromName("w").has_value());
}

TEST(AxisUtilTest, ToString) {
    EXPECT_EQ(Axes::toString(Axis::X), "x");
    EXPECT_EQ(Axes::toString(Axis::Y), "y");
    EXPECT_EQ(Axes::toString(Axis::Z), "z");
}

TEST(AxisUtilTest, IsHorizontalVertical) {
    EXPECT_TRUE(Axes::isHorizontal(Axis::X));
    EXPECT_TRUE(Axes::isHorizontal(Axis::Z));
    EXPECT_FALSE(Axes::isHorizontal(Axis::Y));

    EXPECT_FALSE(Axes::isVertical(Axis::X));
    EXPECT_FALSE(Axes::isVertical(Axis::Z));
    EXPECT_TRUE(Axes::isVertical(Axis::Y));
}

// ============================================================================
// StructureMode 枚举属性测试
// ============================================================================

TEST(StructureModeTest, PropertyCreation) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    EXPECT_EQ(prop.name(), "mode");
    EXPECT_EQ(prop.valueCount(), 4);
    EXPECT_STREQ(prop.typeName(), "EnumProperty");
}

TEST(StructureModeTest, AllowedValues) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    const auto& values = prop.allowedValues();
    ASSERT_EQ(values.size(), 4);

    // 验证所有枚举值都存在
    EXPECT_EQ(values[0], BlockStateProperties::StructureMode::Save);
    EXPECT_EQ(values[1], BlockStateProperties::StructureMode::Load);
    EXPECT_EQ(values[2], BlockStateProperties::StructureMode::Corner);
    EXPECT_EQ(values[3], BlockStateProperties::StructureMode::Data);
}

TEST(StructureModeTest, IndexOf) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    auto saveIdx = prop.indexOf(BlockStateProperties::StructureMode::Save);
    auto loadIdx = prop.indexOf(BlockStateProperties::StructureMode::Load);
    auto cornerIdx = prop.indexOf(BlockStateProperties::StructureMode::Corner);
    auto dataIdx = prop.indexOf(BlockStateProperties::StructureMode::Data);

    ASSERT_TRUE(saveIdx.has_value());
    ASSERT_TRUE(loadIdx.has_value());
    ASSERT_TRUE(cornerIdx.has_value());
    ASSERT_TRUE(dataIdx.has_value());

    EXPECT_EQ(saveIdx.value(), 0);
    EXPECT_EQ(loadIdx.value(), 1);
    EXPECT_EQ(cornerIdx.value(), 2);
    EXPECT_EQ(dataIdx.value(), 3);
}

TEST(StructureModeTest, ValueAt) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    EXPECT_EQ(prop.valueAt(0), BlockStateProperties::StructureMode::Save);
    EXPECT_EQ(prop.valueAt(1), BlockStateProperties::StructureMode::Load);
    EXPECT_EQ(prop.valueAt(2), BlockStateProperties::StructureMode::Corner);
    EXPECT_EQ(prop.valueAt(3), BlockStateProperties::StructureMode::Data);
}

TEST(StructureModeTest, ValueToString) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    EXPECT_EQ(prop.valueToString(BlockStateProperties::StructureMode::Save), "save");
    EXPECT_EQ(prop.valueToString(BlockStateProperties::StructureMode::Load), "load");
    EXPECT_EQ(prop.valueToString(BlockStateProperties::StructureMode::Corner), "corner");
    EXPECT_EQ(prop.valueToString(BlockStateProperties::StructureMode::Data), "data");
}

TEST(StructureModeTest, ParseValue) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    auto save = prop.parse("save");
    auto load = prop.parse("load");
    auto corner = prop.parse("corner");
    auto data = prop.parse("data");
    auto invalid = prop.parse("invalid");

    ASSERT_TRUE(save.has_value());
    EXPECT_EQ(save.value(), BlockStateProperties::StructureMode::Save);

    ASSERT_TRUE(load.has_value());
    EXPECT_EQ(load.value(), BlockStateProperties::StructureMode::Load);

    ASSERT_TRUE(corner.has_value());
    EXPECT_EQ(corner.value(), BlockStateProperties::StructureMode::Corner);

    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(data.value(), BlockStateProperties::StructureMode::Data);

    EXPECT_FALSE(invalid.has_value());
}

TEST(StructureModeTest, RoundTrip) {
    const auto& prop = BlockStateProperties::STRUCTURE_MODE();

    // 测试所有枚举值的往返转换
    for (const auto& mode : prop.allowedValues()) {
        std::string str = prop.valueToString(mode);
        auto parsed = prop.parse(str);

        ASSERT_TRUE(parsed.has_value()) << "Failed to parse: " << str;
        EXPECT_EQ(parsed.value(), mode) << "Round trip failed for mode: " << str;
    }
}
