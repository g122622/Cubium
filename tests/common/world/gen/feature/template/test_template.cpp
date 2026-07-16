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

#include <cmath>
#include <gtest/gtest.h>

#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/RuleTest.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateLoader.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"

using namespace mc;
using namespace mc::world::gen::feature::template_;

// ============================================================================
// 测试夹具
// ============================================================================

class TemplateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保原版方块已注册
        VanillaBlocks::initialize();
    }
};

class RuleTestTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// BlockInfo 测试
// ============================================================================

TEST_F(TemplateTest, BlockInfo_DefaultConstruction)
{
    BlockInfo info;
    EXPECT_EQ(info.pos.x, 0);
    EXPECT_EQ(info.pos.y, 0);
    EXPECT_EQ(info.pos.z, 0);
    EXPECT_EQ(info.blockStateId, 0u);
    EXPECT_EQ(info.nbt, nullptr);
}

TEST_F(TemplateTest, BlockInfo_ParameterizedConstruction)
{
    BlockPos pos(10, 20, 30);
    BlockInfo info(pos, 42);

    EXPECT_EQ(info.pos.x, 10);
    EXPECT_EQ(info.pos.y, 20);
    EXPECT_EQ(info.pos.z, 30);
    EXPECT_EQ(info.blockStateId, 42u);
    EXPECT_EQ(info.nbt, nullptr);
}

TEST_F(TemplateTest, BlockInfo_CopyConstruction)
{
    BlockPos pos(5, 10, 15);
    BlockInfo original(pos, 100);
    original.nbt = std::make_unique<nbt::CompoundTag>();
    original.nbt->value["test"] = std::make_unique<nbt::IntTag>(42);

    BlockInfo copy(original);

    EXPECT_EQ(copy.pos.x, 5);
    EXPECT_EQ(copy.pos.y, 10);
    EXPECT_EQ(copy.pos.z, 15);
    EXPECT_EQ(copy.blockStateId, 100u);
    ASSERT_NE(copy.nbt, nullptr);

    auto* intTag = dynamic_cast<nbt::IntTag*>(copy.nbt->value["test"].get());
    ASSERT_NE(intTag, nullptr);
    EXPECT_EQ(intTag->value, 42);
}

TEST_F(TemplateTest, BlockInfo_MoveConstruction)
{
    BlockPos pos(1, 2, 3);
    BlockInfo original(pos, 50);
    original.nbt = std::make_unique<nbt::CompoundTag>();

    BlockInfo moved(std::move(original));

    EXPECT_EQ(moved.pos.x, 1);
    EXPECT_EQ(moved.pos.y, 2);
    EXPECT_EQ(moved.pos.z, 3);
    EXPECT_EQ(moved.blockStateId, 50u);
    EXPECT_NE(moved.nbt, nullptr);
}

TEST_F(TemplateTest, BlockInfo_CopyAssignment)
{
    BlockInfo original(BlockPos(1, 2, 3), 10);
    original.nbt = std::make_unique<nbt::CompoundTag>();

    BlockInfo assigned;
    assigned = original;

    EXPECT_EQ(assigned.pos.x, 1);
    EXPECT_EQ(assigned.blockStateId, 10u);
    EXPECT_NE(assigned.nbt, nullptr);
}

TEST_F(TemplateTest, BlockInfo_MoveAssignment)
{
    BlockInfo original(BlockPos(1, 2, 3), 10);
    original.nbt = std::make_unique<nbt::CompoundTag>();

    BlockInfo assigned;
    assigned = std::move(original);

    EXPECT_EQ(assigned.pos.x, 1);
    EXPECT_EQ(assigned.blockStateId, 10u);
    EXPECT_NE(assigned.nbt, nullptr);
}

// ============================================================================
// TemplateEntityInfo 测试
// ============================================================================

TEST_F(TemplateTest, EntityInfo_DefaultConstruction)
{
    TemplateEntityInfo info;
    EXPECT_EQ(info.typeId, "");
    EXPECT_EQ(info.posx, 0.0);
    EXPECT_EQ(info.posy, 0.0);
    EXPECT_EQ(info.posz, 0.0);
    EXPECT_EQ(info.blockPos.x, 0);
    EXPECT_EQ(info.blockPos.y, 0);
    EXPECT_EQ(info.blockPos.z, 0);
    EXPECT_EQ(info.nbt, nullptr);
}

TEST_F(TemplateTest, EntityInfo_CopyConstruction)
{
    TemplateEntityInfo original;
    original.typeId = "minecraft:zombie";
    original.posx = 1.5;
    original.posy = 2.0;
    original.posz = 3.5;
    original.blockPos = BlockPos(1, 2, 3);
    original.nbt = std::make_unique<nbt::CompoundTag>();
    original.nbt->value["Health"] = std::make_unique<nbt::FloatTag>(20.0f);

    TemplateEntityInfo copy(original);

    EXPECT_EQ(copy.typeId, "minecraft:zombie");
    EXPECT_DOUBLE_EQ(copy.posx, 1.5);
    EXPECT_DOUBLE_EQ(copy.posy, 2.0);
    EXPECT_DOUBLE_EQ(copy.posz, 3.5);
    EXPECT_EQ(copy.blockPos.x, 1);
    EXPECT_NE(copy.nbt, nullptr);
}

TEST_F(TemplateTest, EntityInfo_MoveConstruction)
{
    TemplateEntityInfo original;
    original.typeId = "minecraft:skeleton";
    original.posx = 10.0;
    original.posy = 20.0;
    original.posz = 30.0;
    original.nbt = std::make_unique<nbt::CompoundTag>();

    TemplateEntityInfo moved(std::move(original));

    EXPECT_EQ(moved.typeId, "minecraft:skeleton");
    EXPECT_DOUBLE_EQ(moved.posx, 10.0);
    EXPECT_DOUBLE_EQ(moved.posy, 20.0);
    EXPECT_DOUBLE_EQ(moved.posz, 30.0);
    EXPECT_NE(moved.nbt, nullptr);
}

// ============================================================================
// PlacementSettings 测试
// ============================================================================

TEST_F(TemplateTest, PlacementSettings_DefaultValues)
{
    PlacementSettings settings;

    EXPECT_EQ(settings.getRotation(), Rotation::None);
    EXPECT_EQ(settings.getMirror(), Mirror::None);
    EXPECT_EQ(settings.ignoreEntities(), false);
    EXPECT_EQ(settings.getBoundingBox(), nullptr);
    EXPECT_EQ(settings.getCenterOffset().x, 0);
    EXPECT_EQ(settings.getCenterOffset().y, 0);
    EXPECT_EQ(settings.getCenterOffset().z, 0);
    EXPECT_EQ(settings.getBlockUpdateFlags(), 18u);
    EXPECT_EQ(settings.keepLiquids(), false);
    EXPECT_EQ(settings.getProcessors(), nullptr);
}

TEST_F(TemplateTest, PlacementSettings_ChainedSetters)
{
    PlacementSettings settings;
    BlockPos offset(10, 20, 30);
    StructureProcessorList processorList;

    auto& result = settings.setRotation(Rotation::Clockwise90)
                       .setMirror(Mirror::LeftRight)
                       .setIgnoreEntities(true)
                       .setCenterOffset(offset)
                       .setBlockUpdateFlags(2)
                       .setKeepLiquids(true)
                       .setProcessors(&processorList);

    EXPECT_EQ(&result, &settings); // 链式调用返回自身
    EXPECT_EQ(settings.getRotation(), Rotation::Clockwise90);
    EXPECT_EQ(settings.getMirror(), Mirror::LeftRight);
    EXPECT_EQ(settings.ignoreEntities(), true);
    EXPECT_EQ(settings.getCenterOffset().x, 10);
    EXPECT_EQ(settings.getCenterOffset().y, 20);
    EXPECT_EQ(settings.getCenterOffset().z, 30);
    EXPECT_EQ(settings.getBlockUpdateFlags(), 2u);
    EXPECT_EQ(settings.keepLiquids(), true);
    EXPECT_EQ(settings.getProcessors(), &processorList);
}

TEST_F(TemplateTest, PlacementSettings_Copy)
{
    PlacementSettings original;
    original.setRotation(Rotation::Clockwise180)
        .setMirror(Mirror::FrontBack)
        .setIgnoreEntities(true)
        .setBlockUpdateFlags(5);

    PlacementSettings copy = original.copy();

    EXPECT_EQ(copy.getRotation(), Rotation::Clockwise180);
    EXPECT_EQ(copy.getMirror(), Mirror::FrontBack);
    EXPECT_EQ(copy.ignoreEntities(), true);
    EXPECT_EQ(copy.getBlockUpdateFlags(), 5u);
}

// ============================================================================
// Template 坐标变换测试
// ============================================================================

TEST_F(TemplateTest, TransformBlockPos_NoRotationNoMirror)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    BlockPos result = Template::transformBlockPos(pos, Mirror::None, Rotation::None, center);

    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, 15);
}

TEST_F(TemplateTest, TransformBlockPos_Rotation90)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    // 90度顺时针: (x, y, z) -> (-z, y, x)
    BlockPos result = Template::transformBlockPos(pos, Mirror::None, Rotation::Clockwise90, center);

    EXPECT_EQ(result.x, -15);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, 5);
}

TEST_F(TemplateTest, TransformBlockPos_Rotation180)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    // 180度: (x, y, z) -> (-x, y, -z)
    BlockPos result = Template::transformBlockPos(pos, Mirror::None, Rotation::Clockwise180, center);

    EXPECT_EQ(result.x, -5);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, -15);
}

TEST_F(TemplateTest, TransformBlockPos_Rotation270)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    // 270度顺时针 (90度逆时针): (x, y, z) -> (z, y, -x)
    BlockPos result = Template::transformBlockPos(pos, Mirror::None, Rotation::CounterClockwise90, center);

    EXPECT_EQ(result.x, 15);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, -5);
}

TEST_F(TemplateTest, TransformBlockPos_MirrorLeftRight)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    // Z轴镜像: (x, y, z) -> (x, y, -z)
    BlockPos result = Template::transformBlockPos(pos, Mirror::LeftRight, Rotation::None, center);

    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, -15);
}

TEST_F(TemplateTest, TransformBlockPos_MirrorFrontBack)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    // X轴镜像: (x, y, z) -> (-x, y, z)
    BlockPos result = Template::transformBlockPos(pos, Mirror::FrontBack, Rotation::None, center);

    EXPECT_EQ(result.x, -5);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, 15);
}

TEST_F(TemplateTest, TransformBlockPos_MirrorAndRotation)
{
    BlockPos pos(5, 10, 15);
    BlockPos center(0, 0, 0);

    // 先镜像后旋转
    // Z轴镜像: (5, 10, 15) -> (5, 10, -15)
    // 90度旋转: (5, 10, -15) -> (15, 10, 5)
    BlockPos result = Template::transformBlockPos(pos, Mirror::LeftRight, Rotation::Clockwise90, center);

    EXPECT_EQ(result.x, 15);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, 5);
}

TEST_F(TemplateTest, TransformBlockPos_WithCenter)
{
    BlockPos pos(10, 5, 10);
    BlockPos center(5, 0, 5);

    // 相对于中心: (10-5, 5, 10-5) = (5, 5, 5)
    // 无变换后: (5, 5, 5)
    // 加回中心: (5+5, 5, 5+5) = (10, 5, 10)
    BlockPos result = Template::transformBlockPos(pos, Mirror::None, Rotation::None, center);

    EXPECT_EQ(result.x, 10);
    EXPECT_EQ(result.y, 5);
    EXPECT_EQ(result.z, 10);
}

// ============================================================================
// Template transformEntityPos 测试
// ============================================================================
// 对应 MC 1.21.11 StructureTemplate#transform(Vec3, Mirror, Rotation, BlockPos)
// 实体位置使用 f64 精度，镜像 `1.0 - coord`，旋转含 +1 偏移（block-corner 坐标系）
// 注：pivot 为 BlockPos(0,0,0) 时公式简化，与 MC Java 默认行为一致

TEST_F(TemplateTest, TransformEntityPos_NoRotationNoMirror)
{
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::None, Rotation::None, pivot);

    EXPECT_DOUBLE_EQ(result.x, 0.3);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_DOUBLE_EQ(result.z, 0.7);
}

TEST_F(TemplateTest, TransformEntityPos_MirrorLeftRight)
{
    // LeftRight 镜像: d2 = 1.0 - d2
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::LeftRight, Rotation::None, pivot);

    EXPECT_DOUBLE_EQ(result.x, 0.3);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_DOUBLE_EQ(result.z, 0.3); // 1.0 - 0.7 = 0.3
}

TEST_F(TemplateTest, TransformEntityPos_MirrorFrontBack)
{
    // FrontBack 镜像: d0 = 1.0 - d0
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::FrontBack, Rotation::None, pivot);

    EXPECT_DOUBLE_EQ(result.x, 0.7); // 1.0 - 0.3 = 0.7
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_DOUBLE_EQ(result.z, 0.7);
}

TEST_F(TemplateTest, TransformEntityPos_RotationClockwise90)
{
    // pivot=(0,0,0): i=0, j=0
    // CLOCKWISE_90: (i + j + 1 - d2, d1, j - i + d0) = (1 - d2, d1, d0)
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::None, Rotation::Clockwise90, pivot);

    EXPECT_NEAR(result.x, 0.3, 1e-10); // 1 - 0.7 = 0.3
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_NEAR(result.z, 0.3, 1e-10); // 0.3
}

TEST_F(TemplateTest, TransformEntityPos_Rotation180)
{
    // pivot=(0,0,0): i=0, j=0
    // CLOCKWISE_180: (i + i + 1 - d0, d1, j + j + 1 - d2) = (1 - d0, d1, 1 - d2)
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::None, Rotation::Clockwise180, pivot);

    EXPECT_NEAR(result.x, 0.7, 1e-10); // 1 - 0.3 = 0.7
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_NEAR(result.z, 0.3, 1e-10); // 1 - 0.7 = 0.3
}

TEST_F(TemplateTest, TransformEntityPos_RotationCounterClockwise90)
{
    // pivot=(0,0,0): i=0, j=0
    // COUNTERCLOCKWISE_90: (i - j + d2, d1, i + j + 1 - d0) = (d2, d1, 1 - d0)
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::None, Rotation::CounterClockwise90, pivot);

    EXPECT_NEAR(result.x, 0.7, 1e-10); // d2 = 0.7
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_NEAR(result.z, 0.7, 1e-10); // 1 - 0.3 = 0.7
}

TEST_F(TemplateTest, TransformEntityPos_MirrorAndRotation)
{
    // 先镜像 (LeftRight): d2 = 1.0 - 0.7 = 0.3, d0 = 0.3
    // 再旋转 (Clockwise90): (1 - d2, d1, d0) = (1 - 0.3, 0.5, 0.3) = (0.7, 0.5, 0.3)
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(0, 0, 0);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::LeftRight, Rotation::Clockwise90, pivot);

    EXPECT_NEAR(result.x, 0.7, 1e-10);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_NEAR(result.z, 0.3, 1e-10);
}

TEST_F(TemplateTest, TransformEntityPos_WithPivot)
{
    // pivot=(2,0,3): i=2, j=3
    // 无镜像无旋转 -> 返回原 pos
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(2, 0, 3);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::None, Rotation::None, pivot);

    EXPECT_DOUBLE_EQ(result.x, 0.3);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_DOUBLE_EQ(result.z, 0.7);
}

TEST_F(TemplateTest, TransformEntityPos_WithPivotRotation)
{
    // pivot=(2,0,3): i=2, j=3
    // FrontBack 镜像: d0 = 1.0 - 0.3 = 0.7
    // CounterClockwise90: (i - j + d2, d1, i + j + 1 - d0)
    //   = (2 - 3 + 0.7, 0.5, 2 + 3 + 1 - 0.7) = (-0.3, 0.5, 5.3)
    math::Vector3d pos(0.3, 0.5, 0.7);
    BlockPos pivot(2, 0, 3);

    math::Vector3d result = Template::transformEntityPos(pos, Mirror::FrontBack, Rotation::CounterClockwise90, pivot);

    EXPECT_NEAR(result.x, -0.3, 1e-10);
    EXPECT_DOUBLE_EQ(result.y, 0.5);
    EXPECT_NEAR(result.z, 5.3, 1e-10);
}

TEST_F(TemplateTest, TransformEntityPos_EntityVsBlockComparison)
{
    // 同一个输入位置（用方块坐标 5,10,15），变换应该不同：
    // - transformBlockPos 用 -coord（整数镜像）
    // - transformEntityPos 用 1.0 - coord（block-corner 镜像）
    // 这测试确认两者不混淆
    BlockPos blockPos(5, 10, 15);
    math::Vector3d entityPos(5.0, 10.0, 15.0);
    BlockPos pivot(0, 0, 0);

    // LeftRight 镜像
    BlockPos blockResult = Template::transformBlockPos(blockPos, Mirror::LeftRight, Rotation::None, pivot);
    math::Vector3d entityResult = Template::transformEntityPos(entityPos, Mirror::LeftRight, Rotation::None, pivot);

    // 方块: (5, 10, -15)（z 取负）
    EXPECT_EQ(blockResult.x, 5);
    EXPECT_EQ(blockResult.y, 10);
    EXPECT_EQ(blockResult.z, -15);

    // 实体: (5.0, 10.0, 1.0 - 15.0 = -14.0)（z 用 1 - coord）
    EXPECT_DOUBLE_EQ(entityResult.x, 5.0);
    EXPECT_DOUBLE_EQ(entityResult.y, 10.0);
    EXPECT_DOUBLE_EQ(entityResult.z, -14.0);
}

TEST_F(TemplateTest, GetTransformedPosition_NoRotation)
{
    BlockPos pos(5, 10, 15);
    BlockPos size(20, 10, 20);

    BlockPos result = Template::getTransformedPosition(pos, Rotation::None, size);

    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 10);
    EXPECT_EQ(result.z, 15);
}

TEST_F(TemplateTest, GetTransformedPosition_Rotation90)
{
    BlockPos size(10, 10, 20); // x=10, z=20
    BlockPos pos(2, 5, 5);

    // 90度顺时针: (x, z) -> (size_z - 1 - z, x)
    // (2, 5) -> (20 - 1 - 5, 2) = (14, 2)
    BlockPos result = Template::getTransformedPosition(pos, Rotation::Clockwise90, size);

    EXPECT_EQ(result.x, 14);
    EXPECT_EQ(result.y, 5);
    EXPECT_EQ(result.z, 2);
}

// ============================================================================
// Template 基本操作测试
// ============================================================================

TEST_F(TemplateTest, Template_DefaultConstruction)
{
    Template templ;

    EXPECT_EQ(templ.getSize().x, 0);
    EXPECT_EQ(templ.getSize().y, 0);
    EXPECT_EQ(templ.getSize().z, 0);
    EXPECT_EQ(templ.getBlockCount(), 0u);
    EXPECT_EQ(templ.getJigsawBlockCount(), 0u);
    EXPECT_EQ(templ.getEntities().size(), 0u);
}

TEST_F(TemplateTest, Template_SetSize)
{
    Template templ;
    templ.setSize(BlockPos(10, 5, 15));

    EXPECT_EQ(templ.getSize().x, 10);
    EXPECT_EQ(templ.getSize().y, 5);
    EXPECT_EQ(templ.getSize().z, 15);
}

TEST_F(TemplateTest, Template_AddBlock)
{
    Template templ;

    // 使用 Palette 添加方块
    std::vector<BlockInfo> blocks;
    blocks.push_back(BlockInfo(BlockPos(0, 0, 0), 1));
    blocks.push_back(BlockInfo(BlockPos(1, 0, 0), 2));
    blocks.push_back(BlockInfo(BlockPos(0, 1, 0), 3));
    templ.addPalette(Palette(std::move(blocks)));

    EXPECT_EQ(templ.getBlockCount(), 3u);
    EXPECT_EQ(templ.getPaletteCount(), 1u);

    const auto& paletteBlocks = templ.getBlocks();
    EXPECT_EQ(paletteBlocks[0].pos.x, 0);
    EXPECT_EQ(paletteBlocks[1].blockStateId, 2u);
    EXPECT_EQ(paletteBlocks[2].pos.y, 1);
}

TEST_F(TemplateTest, Template_AddJigsawBlock)
{
    Template templ;

    templ.addJigsawBlock(TemplateJigsawBlockInfo(
        BlockPos(0, 0, 0), "minecraft:bottom", "minecraft:village/street", "minecraft:empty", 0));
    templ.addJigsawBlock(
        TemplateJigsawBlockInfo(BlockPos(5, 10, 5), "minecraft:top", "minecraft:village/houses", "minecraft:house", 1));

    EXPECT_EQ(templ.getJigsawBlockCount(), 2u);

    const auto& jigsawBlocks = templ.getJigsawBlocks();
    EXPECT_EQ(jigsawBlocks[0].name, "minecraft:bottom");
    EXPECT_EQ(jigsawBlocks[0].jointType, 0);
    EXPECT_EQ(jigsawBlocks[1].targetPool, "minecraft:village/houses");
    EXPECT_EQ(jigsawBlocks[1].jointType, 1);
}

TEST_F(TemplateTest, Template_AddEntity)
{
    Template templ;

    TemplateEntityInfo entity1;
    entity1.typeId = "minecraft:zombie";
    entity1.posx = 1.5;
    entity1.posy = 2.0;
    entity1.posz = 3.5;
    entity1.blockPos = BlockPos(1, 2, 3);

    TemplateEntityInfo entity2;
    entity2.typeId = "minecraft:skeleton";
    entity2.posx = 10.0;
    entity2.posy = 20.0;
    entity2.posz = 30.0;

    templ.addEntity(entity1);
    templ.addEntity(entity2);

    EXPECT_EQ(templ.getEntities().size(), 2u);

    const auto& entities = templ.getEntities();
    EXPECT_EQ(entities[0].typeId, "minecraft:zombie");
    EXPECT_DOUBLE_EQ(entities[0].posx, 1.5);
    EXPECT_EQ(entities[1].typeId, "minecraft:skeleton");
}

TEST_F(TemplateTest, Template_GetBoundingBox)
{
    Template templ;
    templ.setSize(BlockPos(10, 5, 15));

    PlacementSettings settings;
    BlockPos pos(100, 50, 200);

    auto bounds = templ.getBoundingBox(settings, pos);

    EXPECT_EQ(bounds.minX(), 100);
    EXPECT_EQ(bounds.minY(), 50);
    EXPECT_EQ(bounds.minZ(), 200);
    EXPECT_EQ(bounds.maxX(), 109); // 100 + 10 - 1
    EXPECT_EQ(bounds.maxY(), 54);  // 50 + 5 - 1
    EXPECT_EQ(bounds.maxZ(), 214); // 200 + 15 - 1
}

TEST_F(TemplateTest, Template_GetBoundingBox_WithRotation)
{
    Template templ;
    templ.setSize(BlockPos(10, 5, 15));

    PlacementSettings settings;
    settings.setRotation(Rotation::Clockwise90);
    BlockPos pos(0, 0, 0);

    // 90度旋转后尺寸: (15, 5, 10)
    auto bounds = templ.getBoundingBox(settings, pos);

    EXPECT_EQ(bounds.minX(), 0);
    EXPECT_EQ(bounds.minY(), 0);
    EXPECT_EQ(bounds.minZ(), 0);
    EXPECT_EQ(bounds.maxX(), 14); // 0 + 15 - 1
    EXPECT_EQ(bounds.maxY(), 4);  // 0 + 5 - 1
    EXPECT_EQ(bounds.maxZ(), 9);  // 0 + 10 - 1
}

// ============================================================================
// StructureProcessor 测试
// ============================================================================

TEST_F(TemplateTest, StructureProcessor_DefaultProcess)
{
    // StructureProcessor is now abstract (has clone() = 0),
    // use NopStructureProcessor for testing the default pass-through behavior
    NopStructureProcessor processor;
    BlockInfo blockInfo(BlockPos(10, 20, 30), 42);
    PlacementSettings settings;
    math::Random rng(12345);

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pos.x, 10);
    EXPECT_EQ(result->pos.y, 20);
    EXPECT_EQ(result->pos.z, 30);
    EXPECT_EQ(result->blockStateId, 42u);
}

TEST_F(TemplateTest, BlockIgnoreStructureProcessor_IgnoreBlock)
{
    std::vector<u32> ignoredBlocks = {1, 2, 3};
    BlockIgnoreStructureProcessor processor(ignoredBlocks);

    BlockInfo ignoredBlock(BlockPos(0, 0, 0), 2);
    BlockInfo keptBlock(BlockPos(0, 0, 0), 5);
    PlacementSettings settings;
    math::Random rng(12345);

    // 忽略的方块
    auto result1 = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), ignoredBlock, ignoredBlock, settings);
    EXPECT_FALSE(result1.has_value());

    // 不忽略的方块
    auto result2 = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), keptBlock, keptBlock, settings);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2->blockStateId, 5u);
}

TEST_F(TemplateTest, IntegrityProcessor_FullIntegrity)
{
    IntegrityProcessor processor(1.0f); // 100% 完整度

    BlockInfo block(BlockPos(100, 100, 100), 42);
    PlacementSettings settings;
    math::Random rng(12345);

    // 100% 完整度应该保留所有方块
    for (int i = 0; i < 10; ++i) {
        BlockInfo testBlock(BlockPos(i, i * 10, i * 100), 42);
        auto result =
            processor.process(BlockPos(0, 0, 0), BlockPos(i, i * 10, i * 100), testBlock, testBlock, settings);
        EXPECT_TRUE(result.has_value());
    }
}

TEST_F(TemplateTest, IntegrityProcessor_ZeroIntegrity)
{
    IntegrityProcessor processor(0.0f); // 0% 完整度

    BlockInfo block(BlockPos(0, 0, 0), 42);
    PlacementSettings settings;
    math::Random rng(12345);

    // 0% 完整度应该移除所有方块
    for (int i = 0; i < 10; ++i) {
        BlockInfo testBlock(BlockPos(i, i, i), 42);
        auto result = processor.process(BlockPos(0, 0, 0), BlockPos(i, i, i), testBlock, testBlock, settings);
        EXPECT_FALSE(result.has_value());
    }
}

TEST_F(TemplateTest, IntegrityProcessor_Deterministic)
{
    IntegrityProcessor processor(0.5f);

    PlacementSettings settings;

    // 相同位置应该产生相同的结果（确定性）
    for (int i = 0; i < 5; ++i) {
        BlockInfo block1(BlockPos(10, 20, 30), 42);
        BlockInfo block2(BlockPos(10, 20, 30), 42);

        auto result1 = processor.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), block1, block1, settings);
        auto result2 = processor.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), block2, block2, settings);

        EXPECT_EQ(result1.has_value(), result2.has_value());
    }
}

// ============================================================================
// IntegrityProcessor MC 1.16.5 算法对齐测试
// ============================================================================

TEST_F(TemplateTest, IntegrityProcessor_PositionSeededRandom)
{
    // MC 1.16.5: IntegrityProcessor 使用位置种子随机
    // 算法: seed = hashBlockPos(x, y, z); Random rng(seed); rng.nextFloat() <= integrity
    // 这确保同一位置的结构完整度在不同生成中保持一致

    IntegrityProcessor processor(0.5f);
    PlacementSettings settings;

    // 测试位置确定性：同一位置多次处理应该产生相同结果
    BlockInfo block(BlockPos(100, 64, -200), 1);
    auto result1 = processor.process(BlockPos(0, 0, 0), BlockPos(100, 64, -200), block, block, settings);
    auto result2 = processor.process(BlockPos(0, 0, 0), BlockPos(100, 64, -200), block, block, settings);
    auto result3 = processor.process(BlockPos(0, 0, 0), BlockPos(100, 64, -200), block, block, settings);

    // 所得结果必须一致
    EXPECT_EQ(result1.has_value(), result2.has_value());
    EXPECT_EQ(result1.has_value(), result3.has_value());
}

TEST_F(TemplateTest, IntegrityProcessor_ProbabilityDistribution)
{
    // MC 1.16.5: 测试完整度概率分布
    // 使用 0.5 完整度，在大量方块中约 50% 应被保留

    IntegrityProcessor processor(0.5f);
    PlacementSettings settings;

    int totalBlocks = 1000;
    int keptBlocks = 0;

    // 使用不同位置测试
    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo block(BlockPos(i * 7, i % 64, i * 13), 1);
        auto result = processor.process(BlockPos(0, 0, 0), block.pos, block, block, settings);
        if (result.has_value()) {
            ++keptBlocks;
        }
    }

    // 验证保留率接近 50% (允许 ±10% 的误差)
    f32 keepRate = static_cast<f32>(keptBlocks) / static_cast<f32>(totalBlocks);
    EXPECT_GT(keepRate, 0.35f) << "Keep rate too low: " << keepRate;
    EXPECT_LT(keepRate, 0.65f) << "Keep rate too high: " << keepRate;
}

TEST_F(TemplateTest, IntegrityProcessor_DifferentIntegrityValues)
{
    // MC 1.16.5: 测试不同完整度值
    PlacementSettings settings;

    // 测试 0.25 完整度
    {
        IntegrityProcessor processor(0.25f);
        int kept = 0;
        for (int i = 0; i < 200; ++i) {
            BlockInfo block(BlockPos(i, i, i), 1);
            if (processor.process(BlockPos(0, 0, 0), block.pos, block, block, settings)) {
                ++kept;
            }
        }
        f32 rate = static_cast<f32>(kept) / 200.0f;
        EXPECT_GT(rate, 0.1f);
        EXPECT_LT(rate, 0.4f);
    }

    // 测试 0.75 完整度
    {
        IntegrityProcessor processor(0.75f);
        int kept = 0;
        for (int i = 0; i < 200; ++i) {
            BlockInfo block(BlockPos(i + 1000, i, i), 1);
            if (processor.process(BlockPos(0, 0, 0), block.pos, block, block, settings)) {
                ++kept;
            }
        }
        f32 rate = static_cast<f32>(kept) / 200.0f;
        EXPECT_GT(rate, 0.6f);
        EXPECT_LT(rate, 0.9f);
    }
}

TEST_F(TemplateTest, IntegrityProcessor_PositionSensitivity)
{
    // MC 1.16.5: 测试位置敏感性
    // 不同位置应该产生不同的结果（至少在某些情况下）

    IntegrityProcessor processor(0.5f);
    PlacementSettings settings;

    int sameResult = 0;
    int differentResult = 0;

    // 测试相邻位置
    for (int i = 0; i < 100; ++i) {
        BlockPos pos1(i * 2, 0, 0);
        BlockPos pos2(i * 2 + 1, 0, 0);

        BlockInfo block1(pos1, 1);
        BlockInfo block2(pos2, 1);

        auto result1 = processor.process(BlockPos(0, 0, 0), pos1, block1, block1, settings);
        auto result2 = processor.process(BlockPos(0, 0, 0), pos2, block2, block2, settings);

        if (result1.has_value() == result2.has_value()) {
            ++sameResult;
        } else {
            ++differentResult;
        }
    }

    // 至少应该有一些不同的结果
    EXPECT_GT(differentResult, 0) << "Position sensitivity test failed: all positions gave same result";
}

TEST_F(TemplateTest, IntegrityProcessor_MultipleProcessorsChained)
{
    // MC 1.16.5: 测试处理器链中的 IntegrityProcessor
    StructureProcessorList list;

    // 先添加空气忽略处理器
    list.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(std::vector<u32>{0}));

    // 再添加完整度处理器
    list.addProcessor(std::make_unique<IntegrityProcessor>(0.7f));

    PlacementSettings settings;
    settings.setProcessors(&list);

    // 测试非空气方块
    BlockInfo stoneInfo(BlockPos(10, 20, 30), 1); // 假设石头 ID != 0
    auto result = list.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), stoneInfo, stoneInfo, settings);

    // 结果可能是保留或移除，取决于随机
    // 但不应该因为空气忽略处理器而移除
    // 验证结果一致
    auto result2 = list.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), stoneInfo, stoneInfo, settings);
    EXPECT_EQ(result.has_value(), result2.has_value());
}

// ============================================================================
// StructureProcessorList 测试
// ============================================================================

TEST_F(TemplateTest, ProcessorList_Empty)
{
    StructureProcessorList list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0u);

    BlockInfo block(BlockPos(0, 0, 0), 42);
    PlacementSettings settings;

    auto result = list.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), block, block, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, 42u);
}

TEST_F(TemplateTest, ProcessorList_AddProcessor)
{
    StructureProcessorList list;

    list.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(std::vector<u32>{1}));

    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 1u);
}

TEST_F(TemplateTest, ProcessorList_Chain)
{
    StructureProcessorList list;

    // 第一个处理器忽略方块ID为1的方块
    list.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(std::vector<u32>{1}));

    // 方块ID为1的方块应该被过滤
    BlockInfo ignoredBlock(BlockPos(0, 0, 0), 1);
    PlacementSettings settings;

    auto result = list.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), ignoredBlock, ignoredBlock, settings);
    EXPECT_FALSE(result.has_value());

    // 方块ID为2的方块应该保留
    BlockInfo keptBlock(BlockPos(0, 0, 0), 2);
    result = list.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), keptBlock, keptBlock, settings);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, 2u);
}

// ============================================================================
// RuleTest 测试
//
// 方块谓词已并入 mc::RuleTest（引用风格 test(const BlockState&, Random&)），
// template_:: 下的同名类为 mc:: 的别名。位置谓词 PosRuleTest 仍属本命名空间。
// ============================================================================

TEST_F(RuleTestTest, AlwaysTrueRuleTest)
{
    AlwaysTrueRuleTest test;
    math::Random rng(12345);

    const BlockState* state = BlockRegistry::instance().getBlockState(0);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(test.test(*state, rng));
    EXPECT_EQ(test.name(), std::string("always_true"));
}

TEST_F(RuleTestTest, AlwaysTrueRuleTest_Clone)
{
    AlwaysTrueRuleTest original;
    auto clone = original.clone();

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(std::string(clone->name()), "always_true");
    math::Random rng(12345);
    const BlockState* state = BlockRegistry::instance().getBlockState(0);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(clone->test(*state, rng));
}

TEST_F(RuleTestTest, BlockMatchRuleTest)
{
    auto& registry = BlockRegistry::instance();

    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stoneBlock, nullptr);

    BlockMatchRuleTest test(stoneBlock);
    math::Random rng(12345);

    const BlockState* stoneState = &stoneBlock->defaultState();
    EXPECT_TRUE(test.test(*stoneState, rng));

    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));
    if (dirtBlock) {
        const BlockState* dirtState = &dirtBlock->defaultState();
        EXPECT_FALSE(test.test(*dirtState, rng));
    }

    EXPECT_EQ(test.getBlock(), stoneBlock);
}

TEST_F(RuleTestTest, BlockMatchRuleTest_Clone)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stoneBlock, nullptr);

    BlockMatchRuleTest original(stoneBlock);
    auto clone = original.clone();

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(std::string(clone->name()), "block_match");
    auto* cloned = dynamic_cast<BlockMatchRuleTest*>(clone.get());
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getBlock(), stoneBlock);
}

TEST_F(RuleTestTest, BlockStateMatchRuleTest)
{
    auto& registry = BlockRegistry::instance();

    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stoneBlock, nullptr);

    const BlockState* state = &stoneBlock->defaultState();
    BlockStateMatchRuleTest test(state);
    math::Random rng(12345);

    EXPECT_TRUE(test.test(*state, rng));

    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));
    if (dirtBlock) {
        const BlockState* dirtState = &dirtBlock->defaultState();
        EXPECT_FALSE(test.test(*dirtState, rng));
    }
}

TEST_F(RuleTestTest, RandomBlockMatchRuleTest_AlwaysMatch)
{
    auto& registry = BlockRegistry::instance();
    Block* block = registry.getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(block, nullptr);

    RandomBlockMatchRuleTest test(block, 1.0f); // 100% 概率
    const BlockState* state = &block->defaultState();
    math::Random rng(12345);

    // 100% 概率应该总是匹配
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(test.test(*state, rng));
    }
}

TEST_F(RuleTestTest, RandomBlockMatchRuleTest_NeverMatch)
{
    auto& registry = BlockRegistry::instance();
    Block* block = registry.getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(block, nullptr);

    RandomBlockMatchRuleTest test(block, 0.0f); // 0% 概率
    const BlockState* state = &block->defaultState();
    math::Random rng(12345);

    // 0% 概率应该永远不匹配
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(test.test(*state, rng));
    }
}

TEST_F(RuleTestTest, RandomBlockMatchRuleTest_WrongBlock)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stoneBlock, nullptr);

    // 用泥土做目标规则，再拿石头状态去测：方块不匹配，即使概率 100% 也应返回 false
    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));
    if (dirtBlock) {
        RandomBlockMatchRuleTest test(dirtBlock, 1.0f);
        const BlockState* state = &stoneBlock->defaultState();
        math::Random rng(12345);
        EXPECT_FALSE(test.test(*state, rng));
    }
}

// ============================================================================
// PosRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, AlwaysTruePosRuleTest)
{
    AlwaysTruePosRuleTest test;
    math::Random rng(12345);

    EXPECT_TRUE(test.test(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng));
    EXPECT_TRUE(test.test(BlockPos(100, 200, 300), BlockPos(1, 2, 3), BlockPos(10, 20, 30), rng));
    EXPECT_EQ(test.getTypeId(), 0u);
}

TEST_F(RuleTestTest, AlwaysTruePosRuleTest_Clone)
{
    AlwaysTruePosRuleTest original;
    auto clone = original.clone();

    EXPECT_EQ(clone->getTypeId(), 0u);
    math::Random rng(12345);
    EXPECT_TRUE(clone->test(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng));
}

TEST_F(RuleTestTest, LinearPosRuleTest_MinHeight)
{
    // 在最小高度时使用最小概率
    LinearPosRuleTest test(0, 100, 0.0f, 1.0f);

    // Y=0 时，概率 = 0.0f；Y=100 时，概率 = 1.0f；Y=50 时，概率 = 0.5f
    EXPECT_EQ(test.getTypeId(), 1u);
}

TEST_F(RuleTestTest, LinearPosRuleTest_Interpolation)
{
    // 测试线性插值：在 Y=50 时概率应为 0.5，此处仅验证函数不崩溃
    LinearPosRuleTest test(0, 100, 0.0f, 1.0f);
    math::Random rng(12345);
    test.test(BlockPos(0, 50, 0), BlockPos(0, 50, 0), BlockPos(0, 0, 0), rng);
}

TEST_F(RuleTestTest, LinearPosRuleTest_EqualHeights)
{
    // 当 minHeight == maxHeight 时，应使用 minProbability
    LinearPosRuleTest test(50, 50, 0.5f, 1.0f);
    math::Random rng(12345);

    // 无论 Y 坐标如何，概率都固定为 minProbability，此处仅验证不崩溃
    test.test(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng);
    test.test(BlockPos(0, 100, 0), BlockPos(0, 100, 0), BlockPos(0, 0, 0), rng);
}

TEST_F(RuleTestTest, LinearPosRuleTest_Clone)
{
    LinearPosRuleTest original(10, 20, 0.3f, 0.7f);
    auto clone = original.clone();

    EXPECT_EQ(clone->getTypeId(), 1u);
}

// ============================================================================
// RuleEntry 测试
// ============================================================================

TEST_F(RuleTestTest, RuleEntry_AllTrue)
{
    auto inputTest = std::make_unique<AlwaysTrueRuleTest>();
    auto locationTest = std::make_unique<AlwaysTrueRuleTest>();
    auto posTest = std::make_unique<AlwaysTruePosRuleTest>();

    RuleEntry entry(std::move(inputTest), std::move(locationTest), std::move(posTest), 42);

    // 方块状态非空时 AlwaysTrue 谓词通过
    auto& registry = BlockRegistry::instance();
    const BlockState* state = registry.getBlockState(0);
    ASSERT_NE(state, nullptr);

    math::Random rng(12345);
    EXPECT_TRUE(entry.matches(state, state, BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng));
    EXPECT_EQ(entry.outputStateId(), 42u);
}

TEST_F(RuleTestTest, RuleEntry_InputPredicateFails)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));
    if (!stoneBlock || !dirtBlock) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    // 用泥土做输入规则，拿石头状态去测：input 谓词失败
    auto inputTest = std::make_unique<BlockMatchRuleTest>(dirtBlock);
    auto locationTest = std::make_unique<AlwaysTrueRuleTest>();
    auto posTest = std::make_unique<AlwaysTruePosRuleTest>();

    RuleEntry entry(std::move(inputTest), std::move(locationTest), std::move(posTest), 42);

    const BlockState* state = &stoneBlock->defaultState();
    math::Random rng(12345);
    EXPECT_FALSE(entry.matches(state, state, BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng));
}

TEST_F(RuleTestTest, RuleEntry_TwoArgConstructor)
{
    auto inputTest = std::make_unique<AlwaysTrueRuleTest>();
    auto locationTest = std::make_unique<AlwaysTrueRuleTest>();

    RuleEntry entry(std::move(inputTest), std::move(locationTest), 100);

    // 两参构造函数，posPredicate 默认为 AlwaysTruePosRuleTest
    auto& registry = BlockRegistry::instance();
    const BlockState* state = registry.getBlockState(0);
    ASSERT_NE(state, nullptr);

    math::Random rng(12345);
    EXPECT_TRUE(entry.matches(state, state, BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng));
    EXPECT_EQ(entry.outputStateId(), 100u);
    EXPECT_NE(entry.posPredicate(), nullptr);
}

TEST_F(RuleTestTest, RuleEntry_NullStateDoesNotMatch)
{
    // mc::RuleTest 为引用风格，方块状态为空时谓词视为不匹配
    auto inputTest = std::make_unique<BlockMatchRuleTest>(VanillaBlocks::STONE);
    auto locationTest = std::make_unique<AlwaysTrueRuleTest>();
    auto posTest = std::make_unique<AlwaysTruePosRuleTest>();

    RuleEntry entry(std::move(inputTest), std::move(locationTest), std::move(posTest), 42);

    math::Random rng(12345);
    EXPECT_FALSE(entry.matches(nullptr, nullptr, BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(0, 0, 0), rng));
}

// ============================================================================
// RuleStructureProcessor 测试
// ============================================================================

TEST_F(TemplateTest, RuleStructureProcessor_NoMatch)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));

    if (!stoneBlock || !dirtBlock) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    // 输入规则匹配泥土，但传入石头状态：不匹配，保持原样
    auto inputTest = std::make_unique<BlockMatchRuleTest>(dirtBlock);
    auto locationTest = std::make_unique<AlwaysTrueRuleTest>();

    std::vector<std::unique_ptr<RuleEntry>> rules;
    rules.push_back(std::make_unique<RuleEntry>(std::move(inputTest), std::move(locationTest), 42));

    RuleStructureProcessor processor(std::move(rules));

    BlockInfo block(BlockPos(0, 0, 0), stoneBlock->defaultState().stateId());
    PlacementSettings settings;
    math::Random rng(12345);

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), block, block, settings);

    // 没有规则匹配，保持原样
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, stoneBlock->defaultState().stateId());
}

TEST_F(TemplateTest, RuleStructureProcessor_Match)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));

    if (!stoneBlock || !dirtBlock) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    u32 dirtStateId = dirtBlock->defaultState().stateId();

    // 创建一个规则，匹配石头方块，替换为泥土
    auto inputTest = std::make_unique<BlockMatchRuleTest>(stoneBlock);
    auto locationTest = std::make_unique<AlwaysTrueRuleTest>();

    std::vector<std::unique_ptr<RuleEntry>> rules;
    rules.push_back(std::make_unique<RuleEntry>(std::move(inputTest), std::move(locationTest), dirtStateId));

    RuleStructureProcessor processor(std::move(rules));

    BlockInfo block(BlockPos(0, 0, 0), stoneBlock->defaultState().stateId());
    PlacementSettings settings;
    math::Random rng(12345);

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), block, block, settings);

    // 规则匹配，应该返回泥土
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, dirtStateId);
}

TEST_F(TemplateTest, RuleStructureProcessor_MultipleRules)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));
    Block* dirtBlock = registry.getBlock(ResourceLocation("minecraft:dirt"));
    Block* cobblestoneBlock = registry.getBlock(ResourceLocation("minecraft:cobblestone"));

    if (!stoneBlock || !dirtBlock || !cobblestoneBlock) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    u32 cobblestoneStateId = cobblestoneBlock->defaultState().stateId();

    std::vector<std::unique_ptr<RuleEntry>> rules;

    // 规则1：石头 -> 圆石
    rules.push_back(std::make_unique<RuleEntry>(
        std::make_unique<BlockMatchRuleTest>(stoneBlock), std::make_unique<AlwaysTrueRuleTest>(), cobblestoneStateId));

    // 规则2：泥土 -> 不存在的状态ID（这条规则不会被执行到，因为规则1先匹配）
    rules.push_back(std::make_unique<RuleEntry>(
        std::make_unique<BlockMatchRuleTest>(dirtBlock), std::make_unique<AlwaysTrueRuleTest>(), 999));

    RuleStructureProcessor processor(std::move(rules));

    BlockInfo stoneBlockInfo(BlockPos(0, 0, 0), stoneBlock->defaultState().stateId());
    PlacementSettings settings;
    math::Random rng(12345);

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), stoneBlockInfo, stoneBlockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, cobblestoneStateId);
}

// ============================================================================
// ProcessorLists 测试
// ============================================================================

TEST_F(TemplateTest, ProcessorLists_Empty)
{
    const auto& emptyList = ProcessorLists::empty();

    EXPECT_TRUE(emptyList.empty());
    EXPECT_EQ(emptyList.size(), 0u);
}

// ============================================================================
// TemplateJigsawBlockInfo 测试
// ============================================================================

TEST_F(TemplateTest, JigsawBlockInfo_DefaultConstruction)
{
    TemplateJigsawBlockInfo info;

    EXPECT_EQ(info.pos.x, 0);
    EXPECT_EQ(info.pos.y, 0);
    EXPECT_EQ(info.pos.z, 0);
    EXPECT_EQ(info.name, "");
    EXPECT_EQ(info.targetPool, "");
    EXPECT_EQ(info.targetName, "");
    EXPECT_EQ(info.jointType, 0);
}

TEST_F(TemplateTest, JigsawBlockInfo_ParameterizedConstruction)
{
    TemplateJigsawBlockInfo info(
        BlockPos(10, 20, 30), "minecraft:bottom", "minecraft:village/street", "minecraft:empty", 1);

    EXPECT_EQ(info.pos.x, 10);
    EXPECT_EQ(info.pos.y, 20);
    EXPECT_EQ(info.pos.z, 30);
    EXPECT_EQ(info.name, "minecraft:bottom");
    EXPECT_EQ(info.targetPool, "minecraft:village/street");
    EXPECT_EQ(info.targetName, "minecraft:empty");
    EXPECT_EQ(info.jointType, 1);
}

// ============================================================================
// 处理器链集成测试
// ============================================================================

TEST_F(TemplateTest, ProcessorChain_Integration)
{
    auto& registry = BlockRegistry::instance();
    Block* stoneBlock = registry.getBlock(ResourceLocation("minecraft:stone"));

    if (!stoneBlock) {
        GTEST_SKIP() << "Stone block not registered";
    }

    // 创建处理器链
    StructureProcessorList list;

    // 添加一个忽略空气的处理器
    list.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(std::vector<u32>{0}));

    // 添加一个完整度处理器
    list.addProcessor(std::make_unique<IntegrityProcessor>(1.0f));

    EXPECT_EQ(list.size(), 2u);

    // 测试非空气方块
    BlockInfo stoneInfo(BlockPos(0, 0, 0), stoneBlock->defaultState().stateId());
    PlacementSettings settings;

    auto result = list.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), stoneInfo, stoneInfo, settings);
    EXPECT_TRUE(result.has_value());

    // 测试空气方块
    BlockInfo airInfo(BlockPos(0, 0, 0), 0);
    result = list.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), airInfo, airInfo, settings);
    EXPECT_FALSE(result.has_value()); // 被第一个处理器过滤
}

// ============================================================================
// TagMatchRuleTest 测试（template 命名空间版本，复用 mc::TagMatchRuleTest）
// ============================================================================

TEST_F(RuleTestTest, TagMatchRuleTest_MatchesStoneTag)
{
    // BlockTags::initialize() 已在 VanillaBlocks::initialize() 中调用
    TagMatchRuleTest test(ResourceLocation("minecraft", "stone"));
    math::Random rng(12345);

    // 应该匹配 stone 标签中的方块
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::GRANITE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIORITE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::ANDESITE), rng));

    // 不应该匹配不在 stone 标签中的方块
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIRT), rng));
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::COBBLESTONE), rng));
}

TEST_F(RuleTestTest, TagMatchRuleTest_MatchesLogsTag)
{
    TagMatchRuleTest test(ResourceLocation("minecraft", "logs"));
    math::Random rng(12345);

    // 应该匹配 logs 标签中的方块
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::OAK_LOG), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::SPRUCE_LOG), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::BIRCH_LOG), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::JUNGLE_LOG), rng));

    // 不应该匹配不在 logs 标签中的方块
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIRT), rng));
}

TEST_F(RuleTestTest, TagMatchRuleTest_NonExistentTag)
{
    // 不存在的标签应该返回 false
    TagMatchRuleTest test(ResourceLocation("minecraft", "nonexistent_tag"));
    math::Random rng(12345);

    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
}

TEST_F(RuleTestTest, TagMatchRuleTest_Clone)
{
    TagMatchRuleTest test(ResourceLocation("minecraft", "logs"));
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(std::string(clone->name()), "tag_match");

    TagMatchRuleTest* clonedTest = dynamic_cast<TagMatchRuleTest*>(clone.get());
    EXPECT_NE(clonedTest, nullptr);
    EXPECT_EQ(clonedTest->getTagName(), ResourceLocation("minecraft", "logs"));
}
