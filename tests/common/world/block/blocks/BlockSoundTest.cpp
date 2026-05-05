#include <gtest/gtest.h>

#include "common/sound/SoundEvents.hpp"
#include "common/resource/ResourceLocation.hpp"

using namespace mc;

// ============================================================================
// Block Sound Events Tests - 验证音效事件常量已正确注册
// ============================================================================

class BlockSoundEventsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // SoundEvents 是静态常量，无需初始化
    }
};

// ============================================================================
// Dispenser/Dropper Sounds
// ============================================================================

TEST_F(BlockSoundEventsTest, DispenserSound_IsValid) {
    EXPECT_EQ(SoundEvents::BLOCK_DISPENSER_DISPENSE.toString(), "minecraft:block.dispenser.dispense");
}

// ============================================================================
// Pressure Plate Sounds
// ============================================================================

TEST_F(BlockSoundEventsTest, WoodenPressurePlateSounds_AreValid) {
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_ON.toString(), "minecraft:block.wooden_pressure_plate.click_on");
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_OFF.toString(), "minecraft:block.wooden_pressure_plate.click_off");
}

TEST_F(BlockSoundEventsTest, MetalPressurePlateSounds_AreValid) {
    EXPECT_EQ(SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_ON.toString(), "minecraft:block.metal_pressure_plate.click_on");
    EXPECT_EQ(SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_OFF.toString(), "minecraft:block.metal_pressure_plate.click_off");
}

// ============================================================================
// TNT Sounds
// ============================================================================

TEST_F(BlockSoundEventsTest, TNTSound_IsValid) {
    EXPECT_EQ(SoundEvents::ENTITY_TNT_PRIMED.toString(), "minecraft:entity.tnt.primed");
}

// ============================================================================
// Composter Sounds
// ============================================================================

TEST_F(BlockSoundEventsTest, ComposterSounds_AreValid) {
    EXPECT_EQ(SoundEvents::BLOCK_COMPOSTER_EMPTY.toString(), "minecraft:block.composter.empty");
    EXPECT_EQ(SoundEvents::BLOCK_COMPOSTER_FILL.toString(), "minecraft:block.composter.fill");
    EXPECT_EQ(SoundEvents::BLOCK_COMPOSTER_FILL_SUCCESS.toString(), "minecraft:block.composter.fill_success");
    EXPECT_EQ(SoundEvents::BLOCK_COMPOSTER_READY.toString(), "minecraft:block.composter.ready");
}

// ============================================================================
// Turtle Egg Sounds
// ============================================================================

TEST_F(BlockSoundEventsTest, TurtleEggSounds_AreValid) {
    // MC 1.16.5 uses entity.turtle.egg_* not entity.turtle_egg.*
    EXPECT_EQ(SoundEvents::ENTITY_TURTLE_EGG_CRACK.toString(), "minecraft:entity.turtle.egg_crack");
    EXPECT_EQ(SoundEvents::ENTITY_TURTLE_EGG_HATCH.toString(), "minecraft:entity.turtle.egg_hatch");
    EXPECT_EQ(SoundEvents::ENTITY_TURTLE_EGG_BREAK.toString(), "minecraft:entity.turtle.egg_break");
}

// ============================================================================
// Sound Event Resource Location Tests
// ============================================================================

TEST_F(BlockSoundEventsTest, SoundEvents_HaveMinecraftNamespace) {
    // 验证所有音效事件都有正确的命名空间
    EXPECT_TRUE(SoundEvents::BLOCK_DISPENSER_DISPENSE.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_ON.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_ON.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::ENTITY_TNT_PRIMED.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::BLOCK_COMPOSTER_READY.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::ENTITY_TURTLE_EGG_HATCH.toString().find("minecraft:") == 0);
}
