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

#include "common/sound/network/SoundPackets.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include <gtest/gtest.h>

using namespace mc::sound;
using namespace mc::world;
using mc::BlockPos;
using mc::i32;

// ==================== WorldEventPacket 基础测试 ====================

class WorldEventPacketTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testEventId = WorldEvents::IRON_DOOR_OPEN_SOUND;
        testPos = BlockPos(100, 64, -200);
        testData = 0;
    }

    i32 testEventId;
    BlockPos testPos;
    i32 testData;
};

TEST_F(WorldEventPacketTest, DefaultConstruction)
{
    WorldEventPacket packet;
    EXPECT_EQ(packet.getEventId(), 0);
    EXPECT_EQ(packet.getX(), 0);
    EXPECT_EQ(packet.getY(), 0);
    EXPECT_EQ(packet.getZ(), 0);
    EXPECT_EQ(packet.getData(), 0);
}

TEST_F(WorldEventPacketTest, ParameterizedConstruction)
{
    WorldEventPacket packet(testEventId, testPos.x, testPos.y, testPos.z, testData);

    EXPECT_EQ(packet.getEventId(), testEventId);
    EXPECT_EQ(packet.getX(), testPos.x);
    EXPECT_EQ(packet.getY(), testPos.y);
    EXPECT_EQ(packet.getZ(), testPos.z);
    EXPECT_EQ(packet.getData(), testData);
}

TEST_F(WorldEventPacketTest, BlockPosConstruction)
{
    WorldEventPacket packet(testEventId, testPos, testData);

    EXPECT_EQ(packet.getEventId(), testEventId);
    EXPECT_EQ(packet.getX(), testPos.x);
    EXPECT_EQ(packet.getY(), testPos.y);
    EXPECT_EQ(packet.getZ(), testPos.z);
    EXPECT_EQ(packet.getData(), testData);
}

TEST_F(WorldEventPacketTest, SerializeDeserialize)
{
    WorldEventPacket original(testEventId, testPos, testData);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    WorldEventPacket deserialized;
    auto deserializeResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success());

    EXPECT_EQ(deserialized.getEventId(), original.getEventId());
    EXPECT_EQ(deserialized.getX(), original.getX());
    EXPECT_EQ(deserialized.getY(), original.getY());
    EXPECT_EQ(deserialized.getZ(), original.getZ());
    EXPECT_EQ(deserialized.getData(), original.getData());
}

TEST_F(WorldEventPacketTest, SerializeDeserializeWithNegativeCoords)
{
    BlockPos negativePos(-100, -64, -300);
    WorldEventPacket original(WorldEvents::BREAK_BLOCK_EFFECTS, negativePos, 42);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    WorldEventPacket deserialized;
    auto deserializeResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success());

    EXPECT_EQ(deserialized.getEventId(), WorldEvents::BREAK_BLOCK_EFFECTS);
    EXPECT_EQ(deserialized.getX(), -100);
    EXPECT_EQ(deserialized.getY(), -64);
    EXPECT_EQ(deserialized.getZ(), -300);
    EXPECT_EQ(deserialized.getData(), 42);
}

TEST_F(WorldEventPacketTest, SerializeDeserializePlayRecord)
{
    // 测试播放唱片事件（data 为唱片物品ID）
    WorldEventPacket original(WorldEvents::PLAY_RECORD_SOUND, BlockPos(50, 60, 70), 500);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    WorldEventPacket deserialized;
    auto deserializeResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success());

    EXPECT_EQ(deserialized.getEventId(), WorldEvents::PLAY_RECORD_SOUND);
    EXPECT_EQ(deserialized.getData(), 500);
}

TEST_F(WorldEventPacketTest, ExpectedSize)
{
    WorldEventPacket packet;
    EXPECT_GT(packet.expectedSize(), 0u);
}

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
