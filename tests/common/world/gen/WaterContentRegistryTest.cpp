#include <gtest/gtest.h>

#include "resource/ResourceLocation.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/gen/structure/StructureManager.hpp"

using namespace mc;

namespace {

void expectRegisteredBlock(const char* id)
{
    auto* block = BlockRegistry::instance().getBlock(ResourceLocation(id));
    EXPECT_NE(block, nullptr) << "missing block id: " << id;
}

} // namespace

TEST(WaterContentRegistryTest, RegistersWaterUpdateRelatedBlocks)
{
    VanillaBlocks::initialize();

    // 水域方块
    EXPECT_NE(VanillaBlocks::BUBBLE_COLUMN, nullptr);
    EXPECT_NE(VanillaBlocks::TURTLE_EGG, nullptr);
    EXPECT_NE(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::DEAD_TUBE_CORAL_FAN, nullptr);
    EXPECT_NE(VanillaBlocks::DEAD_TUBE_CORAL_WALL_FAN, nullptr);

    // 海晶楼梯/台阶
    EXPECT_NE(VanillaBlocks::PRISMARINE_STAIRS, nullptr);
    EXPECT_NE(VanillaBlocks::PRISMARINE_BRICK_STAIRS, nullptr);
    EXPECT_NE(VanillaBlocks::DARK_PRISMARINE_STAIRS, nullptr);
    EXPECT_NE(VanillaBlocks::PRISMARINE_SLAB, nullptr);
    EXPECT_NE(VanillaBlocks::PRISMARINE_BRICK_SLAB, nullptr);
    EXPECT_NE(VanillaBlocks::DARK_PRISMARINE_SLAB, nullptr);

    // 去皮木头与木头方块
    EXPECT_NE(VanillaBlocks::OAK_WOOD, nullptr);
    EXPECT_NE(VanillaBlocks::STRIPPED_OAK_LOG, nullptr);
    EXPECT_NE(VanillaBlocks::STRIPPED_OAK_WOOD, nullptr);

    expectRegisteredBlock("minecraft:bubble_column");
    expectRegisteredBlock("minecraft:turtle_egg");
    expectRegisteredBlock("minecraft:dead_tube_coral_block");
    expectRegisteredBlock("minecraft:dead_tube_coral_fan");
    expectRegisteredBlock("minecraft:dead_tube_coral_wall_fan");
    expectRegisteredBlock("minecraft:prismarine_stairs");
    expectRegisteredBlock("minecraft:prismarine_brick_stairs");
    expectRegisteredBlock("minecraft:dark_prismarine_stairs");
    expectRegisteredBlock("minecraft:prismarine_slab");
    expectRegisteredBlock("minecraft:prismarine_brick_slab");
    expectRegisteredBlock("minecraft:dark_prismarine_slab");
    expectRegisteredBlock("minecraft:oak_wood");
    expectRegisteredBlock("minecraft:stripped_oak_log");
    expectRegisteredBlock("minecraft:stripped_oak_wood");
}

TEST(WaterContentRegistryTest, RegistersShipwreckAndOceanRuinStructures)
{
    world::gen::structure::StructureRegistry::initialize();

    const auto* shipwreck = world::gen::structure::StructureRegistry::get("shipwreck");
    const auto* oceanRuin = world::gen::structure::StructureRegistry::get("ocean_ruin");

    ASSERT_NE(shipwreck, nullptr);
    ASSERT_NE(oceanRuin, nullptr);

    EXPECT_EQ(shipwreck->structureType(), world::gen::structure::StructureType::Shipwreck);
    EXPECT_EQ(oceanRuin->structureType(), world::gen::structure::StructureType::OceanRuin);
}
