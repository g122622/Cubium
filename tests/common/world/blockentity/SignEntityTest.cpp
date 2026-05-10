#include <gtest/gtest.h>
#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/block/BlockPos.hpp"
#include "util/text/StringTextComponent.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== SignEntityType 注册测试 ==========

class SignEntityRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 确保内置类型已注册
        BlockEntityRegistry::instance().registerBuiltinTypes();
    }
};

TEST_F(SignEntityRegistryTest, SignType_IsRegistered) {
    // 验证 Sign 类型已在注册表中注册
    EXPECT_TRUE(BlockEntityRegistry::instance().hasType(BlockEntityType::Sign));
}

TEST_F(SignEntityRegistryTest, CreateSignEntity_ReturnsValidEntity) {
    BlockPos pos(10, 64, -5);
    auto entity = BlockEntityRegistry::instance().create(BlockEntityType::Sign, pos);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(SignEntityRegistryTest, CreateSignEntity_CreatesSignEntity) {
    BlockPos pos(0, 0, 0);
    auto entity = BlockEntityRegistry::instance().create(BlockEntityType::Sign, pos);

    // 验证创建的是 SignEntity 类型
    auto* signEntity = dynamic_cast<SignEntity*>(entity.get());
    EXPECT_NE(signEntity, nullptr);
}

TEST_F(SignEntityRegistryTest, CreateFromJson_SignEntity) {
    nlohmann::json data;
    data["id"] = "minecraft:sign";
    data["x"] = 100;
    data["y"] = 64;
    data["z"] = -200;

    auto entity = BlockEntityRegistry::instance().createFromJson(data);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos().x, 100);
    EXPECT_EQ(entity->getPos().y, 64);
    EXPECT_EQ(entity->getPos().z, -200);
}

// ========== SignEntity 功能测试 ==========

class SignEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        signEntity = std::make_unique<SignEntity>(BlockPos(10, 20, 30));
    }

    std::unique_ptr<SignEntity> signEntity;
};

TEST_F(SignEntityTest, Constructor_InitializesEmptyLines) {
    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        const auto* line = signEntity->getLine(i);
        ASSERT_NE(line, nullptr);
        EXPECT_EQ(line->getUnformattedText(), "");
    }
}

TEST_F(SignEntityTest, Constructor_SetsCorrectType) {
    EXPECT_EQ(signEntity->getType(), BlockEntityType::Sign);
}

TEST_F(SignEntityTest, Constructor_SetsCorrectPosition) {
    EXPECT_EQ(signEntity->getPos(), BlockPos(10, 20, 30));
}

TEST_F(SignEntityTest, SetLine_ValidText) {
    auto text = std::make_unique<text::StringTextComponent>("Hello World");
    EXPECT_TRUE(signEntity->setLine(0, std::move(text)));

    EXPECT_EQ(signEntity->getLineText(0), "Hello World");
}

TEST_F(SignEntityTest, SetLine_InvalidLine_ReturnsFalse) {
    auto text = std::make_unique<text::StringTextComponent>("Test");
    EXPECT_FALSE(signEntity->setLine(-1, std::move(text)));
    EXPECT_FALSE(signEntity->setLine(4, std::move(text)));
}

TEST_F(SignEntityTest, SetLine_AllLines) {
    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        auto text = std::make_unique<text::StringTextComponent>("Line " + std::to_string(i));
        EXPECT_TRUE(signEntity->setLine(i, std::move(text)));
    }

    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        EXPECT_EQ(signEntity->getLineText(i), "Line " + std::to_string(i));
    }
}

TEST_F(SignEntityTest, SetLineFromLegacy_PlainText) {
    EXPECT_TRUE(signEntity->setLineFromLegacy(0, "Plain Text"));
    EXPECT_EQ(signEntity->getLineText(0), "Plain Text");
}

TEST_F(SignEntityTest, GetLine_OutOfRange_ReturnsNullptr) {
    EXPECT_EQ(signEntity->getLine(-1), nullptr);
    EXPECT_EQ(signEntity->getLine(4), nullptr);
}

TEST_F(SignEntityTest, ClearLines_ClearsAllLines) {
    signEntity->setLineFromLegacy(0, "Line 0");
    signEntity->setLineFromLegacy(1, "Line 1");
    signEntity->setLineFromLegacy(2, "Line 2");
    signEntity->setLineFromLegacy(3, "Line 3");

    signEntity->clearLines();

    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        EXPECT_EQ(signEntity->getLineText(i), "");
    }
}

TEST_F(SignEntityTest, Editable_InitiallyTrue) {
    EXPECT_TRUE(signEntity->isEditable());
}

TEST_F(SignEntityTest, SetEditable_ChangesState) {
    signEntity->setEditable(false);
    EXPECT_FALSE(signEntity->isEditable());

    signEntity->setEditable(true);
    EXPECT_TRUE(signEntity->isEditable());
}

TEST_F(SignEntityTest, Editor_InitiallyNullptr) {
    EXPECT_EQ(signEntity->getEditor(), nullptr);
}

TEST_F(SignEntityTest, TextColor_InitiallyZero) {
    EXPECT_EQ(signEntity->getTextColor(), 0);
}

TEST_F(SignEntityTest, SetTextColor_ChangesColor) {
    signEntity->setTextColor(14);  // Red dye color
    EXPECT_EQ(signEntity->getTextColor(), 14);
}

TEST_F(SignEntityTest, Glowing_InitiallyFalse) {
    EXPECT_FALSE(signEntity->isGlowing());
}

TEST_F(SignEntityTest, SetGlowing_ChangesState) {
    signEntity->setGlowing(true);
    EXPECT_TRUE(signEntity->isGlowing());

    signEntity->setGlowing(false);
    EXPECT_FALSE(signEntity->isGlowing());
}

TEST_F(SignEntityTest, OnlyOpsCanSetNbt_AlwaysTrue) {
    EXPECT_TRUE(signEntity->onlyOpsCanSetNbt());
}

TEST_F(SignEntityTest, Save_PreservesBasicInfo) {
    signEntity->setLineFromLegacy(0, "Hello");
    signEntity->setTextColor(5);
    signEntity->setGlowing(true);

    nlohmann::json data;
    signEntity->save(data);

    EXPECT_EQ(data["id"], "minecraft:sign");
    EXPECT_EQ(data["x"].get<i32>(), 10);
    EXPECT_EQ(data["y"].get<i32>(), 20);
    EXPECT_EQ(data["z"].get<i32>(), 30);
}

TEST_F(SignEntityTest, Load_PreservesTextLines) {
    // 创建原始数据
    signEntity->setLineFromLegacy(0, "Line 0");
    signEntity->setLineFromLegacy(1, "Line 1");
    signEntity->setLineFromLegacy(2, "Line 2");
    signEntity->setLineFromLegacy(3, "Line 3");
    signEntity->setTextColor(7);
    signEntity->setGlowing(true);

    nlohmann::json data;
    signEntity->save(data);

    // 加载到新实体
    auto loaded = std::make_unique<SignEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        EXPECT_EQ(loaded->getLineText(i), "Line " + std::to_string(i));
    }
    EXPECT_EQ(loaded->getTextColor(), 7);
    EXPECT_TRUE(loaded->isGlowing());
}

TEST_F(SignEntityTest, Clone_CreatesExactCopy) {
    signEntity->setLineFromLegacy(0, "Test Line");
    signEntity->setTextColor(12);
    signEntity->setGlowing(true);
    signEntity->setEditable(false);

    auto copy = signEntity->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Sign);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));

    auto* signCopy = dynamic_cast<SignEntity*>(copy.get());
    ASSERT_NE(signCopy, nullptr);
    EXPECT_EQ(signCopy->getLineText(0), "Test Line");
    EXPECT_EQ(signCopy->getTextColor(), 12);
    EXPECT_TRUE(signCopy->isGlowing());
    EXPECT_FALSE(signCopy->isEditable());
}

TEST_F(SignEntityTest, Constants_CorrectValues) {
    EXPECT_EQ(SignEntity::LINE_COUNT, 4);
    EXPECT_EQ(SignEntity::MAX_LINE_LENGTH, 15);
}
