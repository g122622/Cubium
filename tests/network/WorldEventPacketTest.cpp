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

// 原 WorldEventPacketTest.cpp 的 WorldEventPacket（旧 Packet 子类）测试已随
// SoundPackets 删除（阶段5）：WorldEvent 已由 IR ir::play::LevelEvent 承载。
// 保留 WorldEvents 常量唯一性/取值测试（WorldEvents.hpp 是活的协议常量）。

#include "common/world/WorldEvents.hpp"
#include <gtest/gtest.h>

#include <set>

using mc::i32;
using namespace mc::world;

// ==================== WorldEvents 常量测试 ====================

class WorldEventsTest : public ::testing::Test {
protected:
};

TEST_F(WorldEventsTest, DoorSoundEvents)
{
    // 门开关音效事件ID
    EXPECT_EQ(WorldEvents::IRON_DOOR_OPEN_SOUND, 1005);
    EXPECT_EQ(WorldEvents::WOODEN_DOOR_OPEN_SOUND, 1006);
    EXPECT_EQ(WorldEvents::IRON_DOOR_CLOSE_SOUND, 1011);
    EXPECT_EQ(WorldEvents::WOODEN_DOOR_CLOSE_SOUND, 1012);
    EXPECT_EQ(WorldEvents::FENCE_GATE_OPEN_SOUND, 1008);
    EXPECT_EQ(WorldEvents::FENCE_GATE_CLOSE_SOUND, 1014);
    EXPECT_EQ(WorldEvents::WOODEN_TRAPDOOR_OPEN_SOUND, 1007);
    EXPECT_EQ(WorldEvents::WOODEN_TRAPDOOR_CLOSE_SOUND, 1013);
    EXPECT_EQ(WorldEvents::IRON_TRAPDOOR_OPEN_SOUND, 1037);
    EXPECT_EQ(WorldEvents::IRON_TRAPDOOR_CLOSE_SOUND, 1036);
}

TEST_F(WorldEventsTest, FireAndExplosionEvents)
{
    // 火焰和爆炸相关事件
    EXPECT_EQ(WorldEvents::FIRE_EXTINGUISH_SOUND, 1009);
    EXPECT_EQ(WorldEvents::LAVA_EXTINGUISH, 1501);
    EXPECT_EQ(WorldEvents::BREAK_BLOCK_EFFECTS, 2001);
}

TEST_F(WorldEventsTest, RecordEvents)
{
    // 唱片相关事件
    EXPECT_EQ(WorldEvents::PLAY_RECORD_SOUND, 1010);
}

TEST_F(WorldEventsTest, ParticleEvents)
{
    // 粒子效果事件
    EXPECT_EQ(WorldEvents::DISPENSER_SMOKE, 2000);
    EXPECT_EQ(WorldEvents::BREAK_BLOCK_EFFECTS, 2001);
    EXPECT_EQ(WorldEvents::POTION_IMPACT_INSTANT, 2002);
    EXPECT_EQ(WorldEvents::ENDER_EYE_SHATTER, 2003);
    EXPECT_EQ(WorldEvents::MOB_SPAWNER_PARTICLES, 2004);
    EXPECT_EQ(WorldEvents::BONEMEAL_PARTICLES, 2005);
    EXPECT_EQ(WorldEvents::DRAGON_FIREBALL_HIT, 2006);
    EXPECT_EQ(WorldEvents::POTION_IMPACT, 2007);
    EXPECT_EQ(WorldEvents::SPAWN_EXPLOSION_PARTICLE, 2008);
    EXPECT_EQ(WorldEvents::WET_SPONGE_DRY, 2009);
    EXPECT_EQ(WorldEvents::SHOOT_WHITE_SMOKE, 2010);
    EXPECT_EQ(WorldEvents::PLANT_GROWTH_PARTICLES, 2011);
    EXPECT_EQ(WorldEvents::TURTLE_EGG_PLACEMENT, 2012);
    EXPECT_EQ(WorldEvents::SMASH_ATTACK, 2013);
}

TEST_F(WorldEventsTest, SpecialEffectEvents)
{
    // 特殊效果事件 (1500-1505)
    EXPECT_EQ(WorldEvents::COMPOSTER_FILLED_UP, 1500);
    EXPECT_EQ(WorldEvents::LAVA_EXTINGUISH, 1501);
    EXPECT_EQ(WorldEvents::REDSTONE_TORCH_BURNOUT, 1502);
    EXPECT_EQ(WorldEvents::END_PORTAL_FRAME_FILL, 1503);
    EXPECT_EQ(WorldEvents::DRIPSTONE_DRIP, 1504);
    EXPECT_EQ(WorldEvents::PLANT_GROWTH_EFFECT, 1505);
}

TEST_F(WorldEventsTest, SpecialEvents)
{
    // 特殊事件
    EXPECT_EQ(WorldEvents::ANVIL_DESTROYED_SOUND, 1029);
    EXPECT_EQ(WorldEvents::ANVIL_USE_SOUND, 1030);
    EXPECT_EQ(WorldEvents::ANVIL_LAND_SOUND, 1031);
    EXPECT_EQ(WorldEvents::PORTAL_TRAVEL_SOUND, 1032);
    EXPECT_EQ(WorldEvents::CHORUS_FLOWER_GROW_SOUND, 1033);
    EXPECT_EQ(WorldEvents::CHORUS_FLOWER_DEATH_SOUND, 1034);
}

TEST_F(WorldEventsTest, MobSoundEvents)
{
    // 生物相关音效事件
    EXPECT_EQ(WorldEvents::GHAST_WARN_SOUND, 1015);
    EXPECT_EQ(WorldEvents::GHAST_SHOOT_SOUND, 1016);
    EXPECT_EQ(WorldEvents::ENDER_DRAGON_SHOOT_SOUND, 1017);
    EXPECT_EQ(WorldEvents::BLAZE_SHOOT_SOUND, 1018);
    EXPECT_EQ(WorldEvents::BAT_TAKEOFF_SOUND, 1025);
    EXPECT_EQ(WorldEvents::PHANTOM_BITE_SOUND, 1039);
    EXPECT_EQ(WorldEvents::ENDERMAN_GROWL_SOUND, 3001);
}

TEST_F(WorldEventsTest, ZombieSoundEvents)
{
    // 僵尸相关音效事件
    EXPECT_EQ(WorldEvents::ZOMBIE_ATTACK_DOOR_WOOD_SOUND, 1019);
    EXPECT_EQ(WorldEvents::ZOMBIE_ATTACK_DOOR_IRON_SOUND, 1020);
    EXPECT_EQ(WorldEvents::ZOMBIE_BREAK_DOOR_WOOD_SOUND, 1021);
    EXPECT_EQ(WorldEvents::ZOMBIE_INFECT_SOUND, 1026);
    EXPECT_EQ(WorldEvents::ZOMBIE_VILLAGER_CONVERTED_SOUND, 1027);
    EXPECT_EQ(WorldEvents::ZOMBIE_CONVERT_TO_DROWNED_SOUND, 1040);
    EXPECT_EQ(WorldEvents::HUSK_CONVERT_TO_ZOMBIE_SOUND, 1041);
}

TEST_F(WorldEventsTest, WitherSoundEvents)
{
    // 凋灵相关音效事件
    EXPECT_EQ(WorldEvents::WITHER_BREAK_BLOCK_SOUND, 1022);
    EXPECT_EQ(WorldEvents::WITHER_BREAK_BLOCK, 1023);
    EXPECT_EQ(WorldEvents::WITHER_SHOOT_SOUND, 1024);
}

TEST_F(WorldEventsTest, DispenserSoundEvents)
{
    // 发射器相关音效事件
    EXPECT_EQ(WorldEvents::DISPENSER_DISPENSE_SOUND, 1000);
    EXPECT_EQ(WorldEvents::DISPENSER_FAIL_SOUND, 1001);
    EXPECT_EQ(WorldEvents::DISPENSER_LAUNCH_SOUND, 1002);
}

TEST_F(WorldEventsTest, ComposterEvent)
{
    // 堆肥桶事件
    EXPECT_EQ(WorldEvents::COMPOSTER_FILLED_UP, 1500);
    // data > 0: 堆肥桶还有空间
    // data <= 0: 堆肥桶已满
}

TEST_F(WorldEventsTest, RedstoneEvent)
{
    // 红石火把熄灭事件
    EXPECT_EQ(WorldEvents::REDSTONE_TORCH_BURNOUT, 1502);
}

TEST_F(WorldEventsTest, EndPortalEvent)
{
    // 末地传送门框架填充事件
    EXPECT_EQ(WorldEvents::END_PORTAL_FRAME_FILL, 1503);
}

TEST_F(WorldEventsTest, GatewaySpawnEvent)
{
    // 末地传送门生成效果
    EXPECT_EQ(WorldEvents::GATEWAY_SPAWN_EFFECTS, 3000);
}

TEST_F(WorldEventsTest, EventsAreUnique)
{
    // 确保不同类型的事件ID都是唯一的
    std::set<i32> eventIds;

    // 发射器事件
    eventIds.insert(WorldEvents::DISPENSER_DISPENSE_SOUND);
    eventIds.insert(WorldEvents::DISPENSER_FAIL_SOUND);
    eventIds.insert(WorldEvents::DISPENSER_LAUNCH_SOUND);

    // 门事件
    eventIds.insert(WorldEvents::IRON_DOOR_OPEN_SOUND);
    eventIds.insert(WorldEvents::WOODEN_DOOR_OPEN_SOUND);
    eventIds.insert(WorldEvents::IRON_DOOR_CLOSE_SOUND);
    eventIds.insert(WorldEvents::WOODEN_DOOR_CLOSE_SOUND);
    eventIds.insert(WorldEvents::FENCE_GATE_OPEN_SOUND);
    eventIds.insert(WorldEvents::FENCE_GATE_CLOSE_SOUND);
    eventIds.insert(WorldEvents::WOODEN_TRAPDOOR_OPEN_SOUND);
    eventIds.insert(WorldEvents::WOODEN_TRAPDOOR_CLOSE_SOUND);
    eventIds.insert(WorldEvents::IRON_TRAPDOOR_OPEN_SOUND);
    eventIds.insert(WorldEvents::IRON_TRAPDOOR_CLOSE_SOUND);

    // 唱片事件
    eventIds.insert(WorldEvents::PLAY_RECORD_SOUND);

    // 特殊事件
    eventIds.insert(WorldEvents::FIRE_EXTINGUISH_SOUND);
    eventIds.insert(WorldEvents::ANVIL_DESTROYED_SOUND);
    eventIds.insert(WorldEvents::ANVIL_USE_SOUND);
    eventIds.insert(WorldEvents::ANVIL_LAND_SOUND);
    eventIds.insert(WorldEvents::PORTAL_TRAVEL_SOUND);

    // 粒子事件
    eventIds.insert(WorldEvents::DISPENSER_SMOKE);
    eventIds.insert(WorldEvents::BREAK_BLOCK_EFFECTS);
    eventIds.insert(WorldEvents::POTION_IMPACT_INSTANT);
    eventIds.insert(WorldEvents::ENDER_EYE_SHATTER);
    eventIds.insert(WorldEvents::MOB_SPAWNER_PARTICLES);
    eventIds.insert(WorldEvents::BONEMEAL_PARTICLES);

    // 验证所有事件ID都是唯一的
    EXPECT_GE(eventIds.size(), 25u);
}
