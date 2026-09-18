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

#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include <gtest/gtest.h>

using namespace mc;

/**
 * @brief 音效事件常量测试
 *
 * 验证所有音效事件常量是否正确初始化，命名规范是否符合 MC 1.16.5。
 */
class SoundEventsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化工作（如果需要）
    }
};

// ========== 环境音效测试 ==========

TEST_F(SoundEventsTest, AmbientSoundEvents_ValidResourceLocations)
{
    // 验证环境音效
    EXPECT_EQ(SoundEvents::AMBIENT_CAVE.toString(), "minecraft:ambient.cave");

    // 下界群系环境音
    EXPECT_EQ(SoundEvents::AMBIENT_BASALT_DELTAS_ADDITIONS.toString(), "minecraft:ambient.basalt_deltas.additions");
    EXPECT_EQ(SoundEvents::AMBIENT_BASALT_DELTAS_LOOP.toString(), "minecraft:ambient.basalt_deltas.loop");
    EXPECT_EQ(SoundEvents::AMBIENT_BASALT_DELTAS_MOOD.toString(), "minecraft:ambient.basalt_deltas.mood");

    // 水下环境音
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_ENTER.toString(), "minecraft:ambient.underwater.enter");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_EXIT.toString(), "minecraft:ambient.underwater.exit");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP.toString(), "minecraft:ambient.underwater.loop");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS.toString(), "minecraft:ambient.underwater.loop.additions");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS_RARE.toString(),
        "minecraft:ambient.underwater.loop.additions.rare");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS_ULTRA_RARE.toString(),
        "minecraft:ambient.underwater.loop.additions.ultra_rare");
}

// ========== 方块音效测试 ==========

TEST_F(SoundEventsTest, BlockSoundEvents_ValidResourceLocations)
{
    // 门音效
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_DOOR_OPEN.toString(), "minecraft:block.wooden_door.open");
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_DOOR_CLOSE.toString(), "minecraft:block.wooden_door.close");
    EXPECT_EQ(SoundEvents::BLOCK_IRON_DOOR_OPEN.toString(), "minecraft:block.iron_door.open");
    EXPECT_EQ(SoundEvents::BLOCK_IRON_DOOR_CLOSE.toString(), "minecraft:block.iron_door.close");

    // 按钮音效
    EXPECT_EQ(SoundEvents::BLOCK_STONE_BUTTON_CLICK_ON.toString(), "minecraft:block.stone_button.click_on");
    EXPECT_EQ(SoundEvents::BLOCK_STONE_BUTTON_CLICK_OFF.toString(), "minecraft:block.stone_button.click_off");
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_ON.toString(), "minecraft:block.wooden_button.click_on");
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_OFF.toString(), "minecraft:block.wooden_button.click_off");

    // 压力板音效
    EXPECT_EQ(
        SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_ON.toString(), "minecraft:block.stone_pressure_plate.click_on");
    EXPECT_EQ(
        SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_OFF.toString(), "minecraft:block.stone_pressure_plate.click_off");

    // 气泡柱音效
    EXPECT_EQ(
        SoundEvents::BLOCK_BUBBLE_COLUMN_UPWARDS_INSIDE.toString(), "minecraft:block.bubble_column.upwards_inside");
    EXPECT_EQ(
        SoundEvents::BLOCK_BUBBLE_COLUMN_WHIRLPOOL_INSIDE.toString(), "minecraft:block.bubble_column.whirlpool_inside");
}

// ========== 实体音效测试 ==========

TEST_F(SoundEventsTest, PlayerSoundEvents_ValidResourceLocations)
{
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_BURP.toString(), "minecraft:entity.player.burp");
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_HURT.toString(), "minecraft:entity.player.hurt");
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_HURT_DROWN.toString(), "minecraft:entity.player.hurt_drown");
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_HURT_ON_FIRE.toString(), "minecraft:entity.player.hurt_on_fire");
    EXPECT_EQ(
        SoundEvents::ENTITY_PLAYER_HURT_SWEET_BERRY_BUSH.toString(), "minecraft:entity.player.hurt_sweet_berry_bush");
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_DEATH.toString(), "minecraft:entity.player.death");
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_BIG_FALL.toString(), "minecraft:entity.player.big_fall");
    EXPECT_EQ(SoundEvents::ENTITY_PLAYER_SMALL_FALL.toString(), "minecraft:entity.player.small_fall");
}

TEST_F(SoundEventsTest, ZombieSoundEvents_ValidResourceLocations)
{
    EXPECT_EQ(SoundEvents::ENTITY_ZOMBIE_AMBIENT.toString(), "minecraft:entity.zombie.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_ZOMBIE_DEATH.toString(), "minecraft:entity.zombie.death");
    EXPECT_EQ(SoundEvents::ENTITY_ZOMBIE_HURT.toString(), "minecraft:entity.zombie.hurt");
    EXPECT_EQ(SoundEvents::ENTITY_ZOMBIE_STEP.toString(), "minecraft:entity.zombie.step");
}

TEST_F(SoundEventsTest, PassiveMobSoundEvents_ValidResourceLocations)
{
    // 鸡
    EXPECT_EQ(SoundEvents::ENTITY_CHICKEN_AMBIENT.toString(), "minecraft:entity.chicken.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_CHICKEN_DEATH.toString(), "minecraft:entity.chicken.death");
    EXPECT_EQ(SoundEvents::ENTITY_CHICKEN_EGG.toString(), "minecraft:entity.chicken.egg");

    // 牛
    EXPECT_EQ(SoundEvents::ENTITY_COW_AMBIENT.toString(), "minecraft:entity.cow.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_COW_DEATH.toString(), "minecraft:entity.cow.death");
    EXPECT_EQ(SoundEvents::ENTITY_COW_MILK.toString(), "minecraft:entity.cow.milk");

    // 猪
    EXPECT_EQ(SoundEvents::ENTITY_PIG_AMBIENT.toString(), "minecraft:entity.pig.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PIG_DEATH.toString(), "minecraft:entity.pig.death");

    // 羊
    EXPECT_EQ(SoundEvents::ENTITY_SHEEP_AMBIENT.toString(), "minecraft:entity.sheep.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_SHEEP_SHEAR.toString(), "minecraft:entity.sheep.shear");
}

// ========== 物品音效测试 ==========

TEST_F(SoundEventsTest, ItemSoundEvents_ValidResourceLocations)
{
    // 盔甲装备
    EXPECT_EQ(SoundEvents::ITEM_ARMOR_EQUIP_CHAIN.toString(), "minecraft:item.armor.equip_chain");
    EXPECT_EQ(SoundEvents::ITEM_ARMOR_EQUIP_DIAMOND.toString(), "minecraft:item.armor.equip_diamond");
    EXPECT_EQ(SoundEvents::ITEM_ARMOR_EQUIP_IRON.toString(), "minecraft:item.armor.equip_iron");
    EXPECT_EQ(SoundEvents::ITEM_ARMOR_EQUIP_LEATHER.toString(), "minecraft:item.armor.equip_leather");
    EXPECT_EQ(SoundEvents::ITEM_ARMOR_EQUIP_NETHERITE.toString(), "minecraft:item.armor.equip_netherite");

    // 桶
    EXPECT_EQ(SoundEvents::ITEM_BUCKET_EMPTY.toString(), "minecraft:item.bucket.empty");
    EXPECT_EQ(SoundEvents::ITEM_BUCKET_FILL.toString(), "minecraft:item.bucket.fill");
    EXPECT_EQ(SoundEvents::ITEM_BUCKET_EMPTY_LAVA.toString(), "minecraft:item.bucket.empty_lava");
    EXPECT_EQ(SoundEvents::ITEM_BUCKET_FILL_LAVA.toString(), "minecraft:item.bucket.fill_lava");

    // 骨粉
    EXPECT_EQ(SoundEvents::ITEM_BONE_MEAL_USE.toString(), "minecraft:item.bone_meal.use");
}

// ========== 武器音效测试 ==========

TEST_F(SoundEventsTest, WeaponSoundEvents_ValidResourceLocations)
{
    // 三叉戟
    EXPECT_EQ(SoundEvents::ITEM_TRIDENT_THROW.toString(), "minecraft:item.trident.throw");
    EXPECT_EQ(SoundEvents::ITEM_TRIDENT_RIPTIDE_1.toString(), "minecraft:item.trident.riptide_1");
    EXPECT_EQ(SoundEvents::ITEM_TRIDENT_HIT.toString(), "minecraft:item.trident.hit");

    // 弩
    EXPECT_EQ(SoundEvents::ITEM_CROSSBOW_SHOOT.toString(), "minecraft:item.crossbow.shoot");
    EXPECT_EQ(SoundEvents::ITEM_CROSSBOW_LOADING_START.toString(), "minecraft:item.crossbow.loading_start");

    // 盾牌
    EXPECT_EQ(SoundEvents::ITEM_SHIELD_BLOCK.toString(), "minecraft:item.shield.block");
    EXPECT_EQ(SoundEvents::ITEM_SHIELD_BREAK.toString(), "minecraft:item.shield.break");
}

// ========== 音乐音效测试 ==========

TEST_F(SoundEventsTest, MusicSoundEvents_ValidResourceLocations)
{
    EXPECT_EQ(SoundEvents::MUSIC_MENU.toString(), "minecraft:music.menu");
    EXPECT_EQ(SoundEvents::MUSIC_GAME.toString(), "minecraft:music.game");
    EXPECT_EQ(SoundEvents::MUSIC_CREATIVE.toString(), "minecraft:music.creative");
    EXPECT_EQ(SoundEvents::MUSIC_END.toString(), "minecraft:music.end");
    EXPECT_EQ(SoundEvents::MUSIC_DRAGON.toString(), "minecraft:music.dragon");
    EXPECT_EQ(SoundEvents::MUSIC_UNDER_WATER.toString(), "minecraft:music.under_water");
    EXPECT_EQ(SoundEvents::MUSIC_NETHER_BASALT_DELTAS.toString(), "minecraft:music.nether.basalt_deltas");
    EXPECT_EQ(SoundEvents::MUSIC_NETHER_CRIMSON_FOREST.toString(), "minecraft:music.nether.crimson_forest");
}

// ========== ResourceLocation 格式验证 ==========

TEST_F(SoundEventsTest, SoundEvents_HaveMinecraftNamespace)
{
    // 验证所有音效都有 minecraft: 命名空间
    EXPECT_TRUE(SoundEvents::AMBIENT_CAVE.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::BLOCK_STONE_BUTTON_CLICK_ON.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::ENTITY_PLAYER_HURT.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::ITEM_TRIDENT_THROW.toString().find("minecraft:") == 0);
    EXPECT_TRUE(SoundEvents::MUSIC_GAME.toString().find("minecraft:") == 0);
}
