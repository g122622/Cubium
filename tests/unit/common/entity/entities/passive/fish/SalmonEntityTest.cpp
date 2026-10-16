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

/**
 * @file SalmonEntityTest.cpp
 * @brief 鲑鱼实体体型（Variant）业务联动单元测试
 *
 * 参考 MC 1.21.11 net.minecraft.world.entity.animal.fish.Salmon。
 * 覆盖：体型枚举 id/缩放、默认体型、尺寸随体型缩放、NBT 往返、
 *       生成权重（多次 finalizeSpawn 应覆盖三种体型）。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

using namespace mc;

namespace {

/// 暴露 protected 序列化方法供测试
class TestableSalmonEntity : public SalmonEntity {
public:
    using SalmonEntity::SalmonEntity;

    using SalmonEntity::addAdditionalSaveData;
    using SalmonEntity::readAdditionalSaveData;
};

/// 生成测试用世界（BaseTestWorld 构造函数为 protected，须派生）
class SalmonTestWorld final : public mc::test::BaseTestWorld {
};

} // namespace

class SalmonEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        salmon = std::make_unique<TestableSalmonEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    }

    void TearDown() override { salmon.reset(); }

    std::unique_ptr<TestableSalmonEntity> salmon;
};

// ==================== 体型枚举 ====================

TEST_F(SalmonEntityTest, Variant_IdsMatchVanilla)
{
    EXPECT_EQ(static_cast<i32>(SalmonEntity::Variant::Small), 0);
    EXPECT_EQ(static_cast<i32>(SalmonEntity::Variant::Medium), 1);
    EXPECT_EQ(static_cast<i32>(SalmonEntity::Variant::Large), 2);
}

TEST_F(SalmonEntityTest, Variant_ScalesMatchVanilla)
{
    EXPECT_FLOAT_EQ(SalmonEntity::variantScale(SalmonEntity::Variant::Small), 0.5f);
    EXPECT_FLOAT_EQ(SalmonEntity::variantScale(SalmonEntity::Variant::Medium), 1.0f);
    EXPECT_FLOAT_EQ(SalmonEntity::variantScale(SalmonEntity::Variant::Large), 1.5f);
}

TEST_F(SalmonEntityTest, VariantById_ClampsOutOfRange)
{
    EXPECT_EQ(SalmonEntity::variantById(-5), SalmonEntity::Variant::Small);
    EXPECT_EQ(SalmonEntity::variantById(0), SalmonEntity::Variant::Small);
    EXPECT_EQ(SalmonEntity::variantById(1), SalmonEntity::Variant::Medium);
    EXPECT_EQ(SalmonEntity::variantById(2), SalmonEntity::Variant::Large);
    EXPECT_EQ(SalmonEntity::variantById(99), SalmonEntity::Variant::Large);
}

TEST_F(SalmonEntityTest, DefaultVariant_IsMedium)
{
    // vanilla Salmon.Variant.DEFAULT = MEDIUM
    EXPECT_EQ(salmon->getVariant(), SalmonEntity::Variant::Medium);
}

TEST_F(SalmonEntityTest, SetVariant_ChangesVariant)
{
    salmon->setVariant(SalmonEntity::Variant::Large);
    EXPECT_EQ(salmon->getVariant(), SalmonEntity::Variant::Large);

    salmon->setVariant(SalmonEntity::Variant::Small);
    EXPECT_EQ(salmon->getVariant(), SalmonEntity::Variant::Small);
}

// ==================== 尺寸缩放 ====================

TEST_F(SalmonEntityTest, Dimensions_ScaleByVariant)
{
    // 先设置 typeId 使 entityType() 可解析出登记尺寸（鲑鱼 0.7 x 0.4）
    salmon->setTypeId("minecraft:salmon");

    salmon->setVariant(SalmonEntity::Variant::Medium);
    const f32 mediumWidth = salmon->width();
    const f32 mediumHeight = salmon->height();

    salmon->setVariant(SalmonEntity::Variant::Large);
    EXPECT_FLOAT_EQ(salmon->width(), mediumWidth * 1.5f);
    EXPECT_FLOAT_EQ(salmon->height(), mediumHeight * 1.5f);

    salmon->setVariant(SalmonEntity::Variant::Small);
    EXPECT_FLOAT_EQ(salmon->width(), mediumWidth * 0.5f);
    EXPECT_FLOAT_EQ(salmon->height(), mediumHeight * 0.5f);
}

// ==================== NBT 往返 ====================

TEST_F(SalmonEntityTest, NbtRoundTrip_VariantPreserved)
{
    salmon->setVariant(SalmonEntity::Variant::Large);

    nbt::tags::compound_tag tag;
    salmon->addAdditionalSaveData(tag);

    TestableSalmonEntity loaded(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_EQ(loaded.getVariant(), SalmonEntity::Variant::Large);
}

TEST_F(SalmonEntityTest, NbtRoundTrip_EmptyTagDefaultsToMedium)
{
    nbt::tags::compound_tag emptyTag;

    TestableSalmonEntity loaded(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(emptyTag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_EQ(loaded.getVariant(), SalmonEntity::Variant::Medium);
}

// ==================== 生成权重 ====================

TEST_F(SalmonEntityTest, FinalizeSpawn_WeightedVariantCoverage)
{
    // 多次 finalizeSpawn，三种体型都应至少在统计上出现（权重 30/50/15 无零权重项）
    SalmonTestWorld world;
    entity::combat::DifficultyInstance difficulty = entity::combat::DifficultyInstance::at(world, BlockPos(0, 64, 0));

    bool sawSmall = false;
    bool sawMedium = false;
    bool sawLarge = false;
    for (i32 i = 0; i < 500; ++i) {
        salmon->finalizeSpawn(world, difficulty, world::spawn::SpawnReason::Natural);
        switch (salmon->getVariant()) {
            case SalmonEntity::Variant::Small:
                sawSmall = true;
                break;
            case SalmonEntity::Variant::Medium:
                sawMedium = true;
                break;
            case SalmonEntity::Variant::Large:
                sawLarge = true;
                break;
        }
    }

    EXPECT_TRUE(sawSmall);
    EXPECT_TRUE(sawMedium);
    EXPECT_TRUE(sawLarge);
}