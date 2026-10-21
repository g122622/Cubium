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

// 结构方块实体测试（对齐 vanilla StructureBlockEntity）。
//
// 覆盖：
//   - 默认值（坐标 0/1/0、尺寸 0、ignoreEntities=true、showboundingbox=true、integrity=1）
//   - 结构名 / 偏移 / 尺寸钳制范围（-48..48、0..48）
//   - NBT 往返（saveToNBT → loadFromNBT）
//   - JSON 往返（save → 字段）
//   - 实体注册（BlockEntityRegistry 能创建 StructureBlockEntity）
//
// Ref: net.minecraft.world.level.block.entity.StructureBlockEntity

#include "common/TestWorldHelper.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/blockentity/special/StructureBlockEntity.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

class StructureBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_entity = std::make_unique<StructureBlockEntity>(BlockPos(10, 20, 30)); }

    std::unique_ptr<StructureBlockEntity> m_entity;
};

TEST_F(StructureBlockEntityTest, Create_HasCorrectTypeAndPos)
{
    EXPECT_EQ(m_entity->getType(), BlockEntityType::StructureBlock);
    EXPECT_EQ(m_entity->getPos(), BlockPos(10, 20, 30));
}

TEST_F(StructureBlockEntityTest, Create_DefaultValuesMatchVanilla)
{
    // 对齐 vanilla 默认值。
    EXPECT_FALSE(m_entity->hasStructureName());
    EXPECT_EQ(m_entity->getStructurePos(), BlockPos(0, 1, 0));
    EXPECT_EQ(m_entity->getStructureSize(), BlockPos(0, 0, 0));
    EXPECT_EQ(m_entity->getRotation(), Rotation::None);
    EXPECT_EQ(m_entity->getMirror(), Mirror::None);
    EXPECT_TRUE(m_entity->isIgnoreEntities());
    EXPECT_FALSE(m_entity->isStrict());
    EXPECT_FALSE(m_entity->isShowAir());
    EXPECT_TRUE(m_entity->isShowBoundingBox());
    EXPECT_FLOAT_EQ(m_entity->getIntegrity(), 1.0f);
    EXPECT_EQ(m_entity->getSeed(), 0);
}

TEST_F(StructureBlockEntityTest, SetStructurePos_ClampedTo48)
{
    m_entity->setStructurePos(BlockPos(100, -100, 7));
    EXPECT_EQ(m_entity->getStructurePos(), BlockPos(48, -48, 7));
}

TEST_F(StructureBlockEntityTest, SetStructureSize_ClampedToZeroFortyEight)
{
    m_entity->setStructureSize(BlockPos(100, -5, 12));
    EXPECT_EQ(m_entity->getStructureSize(), BlockPos(48, 0, 12));
}

TEST_F(StructureBlockEntityTest, SetIntegrity_ClampedToUnitRange)
{
    m_entity->setIntegrity(2.0f);
    EXPECT_FLOAT_EQ(m_entity->getIntegrity(), 1.0f);
    m_entity->setIntegrity(-1.0f);
    EXPECT_FLOAT_EQ(m_entity->getIntegrity(), 0.0f);
}

TEST_F(StructureBlockEntityTest, NbtRoundTrip_PreservesAllFields)
{
    m_entity->setStructureName("minecraft:test_house");
    m_entity->setAuthor("Tester");
    m_entity->setMetaData("meta");
    m_entity->setStructurePos(BlockPos(1, 2, 3));
    m_entity->setStructureSize(BlockPos(4, 5, 6));
    m_entity->setMirror(Mirror::LeftRight);
    m_entity->setRotation(Rotation::Clockwise90);
    m_entity->setIgnoreEntities(false);
    m_entity->setStrict(true);
    m_entity->setShowAir(true);
    m_entity->setShowBoundingBox(false);
    m_entity->setIntegrity(0.5f);
    m_entity->setSeed(1234567890123LL);

    const nbt::CompoundTag tag = m_entity->getUpdateTag();
    StructureBlockEntity loaded(BlockPos(10, 20, 30));
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    EXPECT_EQ(loaded.getStructureName(), "minecraft:test_house");
    EXPECT_EQ(loaded.getAuthor(), "Tester");
    EXPECT_EQ(loaded.getMetaData(), "meta");
    EXPECT_EQ(loaded.getStructurePos(), BlockPos(1, 2, 3));
    EXPECT_EQ(loaded.getStructureSize(), BlockPos(4, 5, 6));
    EXPECT_EQ(loaded.getMirror(), Mirror::LeftRight);
    EXPECT_EQ(loaded.getRotation(), Rotation::Clockwise90);
    EXPECT_FALSE(loaded.isIgnoreEntities());
    EXPECT_TRUE(loaded.isStrict());
    EXPECT_TRUE(loaded.isShowAir());
    EXPECT_FALSE(loaded.isShowBoundingBox());
    EXPECT_FLOAT_EQ(loaded.getIntegrity(), 0.5f);
    EXPECT_EQ(loaded.getSeed(), 1234567890123LL);
}

TEST_F(StructureBlockEntityTest, Save_WritesVanillaFieldNames)
{
    m_entity->setStructureName("minecraft:foo");
    m_entity->setStructureSize(BlockPos(3, 4, 5));

    nlohmann::json data;
    m_entity->save(data);

    EXPECT_EQ(data["name"], "minecraft:foo");
    EXPECT_EQ(data["sizeX"], 3);
    EXPECT_EQ(data["sizeY"], 4);
    EXPECT_EQ(data["sizeZ"], 5);
    EXPECT_EQ(data["mode"], "DATA"); // 无世界上下文时模式回退 DATA（对齐 vanilla 默认）
    EXPECT_EQ(data["rotation"], "none");
    EXPECT_EQ(data["mirror"], "none");
}

TEST_F(StructureBlockEntityTest, Clone_CopiesEditableState)
{
    m_entity->setStructureName("minecraft:bar");
    m_entity->setSeed(42);

    auto copy = m_entity->clone();
    ASSERT_NE(copy, nullptr);
    auto* structureCopy = dynamic_cast<StructureBlockEntity*>(copy.get());
    ASSERT_NE(structureCopy, nullptr);
    EXPECT_EQ(structureCopy->getStructureName(), "minecraft:bar");
    EXPECT_EQ(structureCopy->getSeed(), 42);
}

TEST_F(StructureBlockEntityTest, Registry_CreatesStructureBlockEntity)
{
    auto& registry = BlockEntityRegistry::instance();
    registry.registerBuiltinTypes();
    ASSERT_TRUE(registry.hasType(BlockEntityType::StructureBlock));

    auto entity = registry.create(BlockEntityType::StructureBlock, BlockPos(1, 2, 3));
    ASSERT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<StructureBlockEntity*>(entity.get()), nullptr);
}