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

// 批 4/18/19：登录/全局状态 + 杂项简单 + 方块实体/维度/经验 S→C Java wire codec 往返。
// 表级往返（roundTripGeneric）兼测 packetID 分发 + payload codec。altIndex 取自
// IrPacket.hpp variant 顺序：Login=17/PlayerPosition=18/SetTime=19/PlayerAbilities=20/
// SetHeldSlot=21/SetDefaultSpawnPosition=22/ChangeDifficulty=23/GameEvent=24（批4）；
// SetCamera=70/SetEntityLink=71/SetPassengers=72/EntityEvent=73/Animate=74/HurtAnimation=75/
// TakeItemEntity=76/BlockDestruction=77/BlockEvent=78（批18）；
// BlockEntityData=79/Respawn=80/SetExperience=81（批19）。
// BlockEntityData.tag 是 shared_ptr<CompoundTag>（默认 == 比指针），用 byteStreamsEqual 兜底。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecsExtended.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;

namespace {

// 构造一个含字段的 CompoundTag，包成 shared_ptr 供 BlockEntityData 用。
std::shared_ptr<nbt::CompoundTag> makeSampleBlockEntityTag()
{
    auto tag = std::make_shared<nbt::CompoundTag>();
    tag->put("id", std::string("minecraft:sign"));
    tag->put("x", static_cast<i32>(10));
    tag->put("y", static_cast<i32>(64));
    tag->put("z", static_cast<i32>(-5));
    tag->put("Text1", std::string("{\"text\":\"hello\"}"));
    return tag;
}

} // namespace

// ============================================================================
// 批 4：登录/全局状态 S→C（Login/PlayerPosition/PlayerAbilities/GameEvent/SetTime/
//       SetHeldSlot/SetDefaultSpawnPosition/ChangeDifficulty）
// ============================================================================

TEST_F(NetworkTestBase, PlayLoginSurvival)
{
    Login in{};
    in.playerId = 42;
    in.hardcore = false;
    in.levels = {"minecraft:overworld", "minecraft:the_nether", "minecraft:the_end"};
    in.maxPlayers = 20;
    in.chunkRadius = 8;
    in.simulationDistance = 6;
    in.reducedDebugInfo = false;
    in.showDeathScreen = true;
    in.doLimitedCrafting = false;
    in.spawnInfo.dimensionType = 0;
    in.spawnInfo.dimension = "minecraft:overworld";
    in.spawnInfo.seed = 12345LL;
    in.spawnInfo.gameType = GameMode::Survival;
    in.spawnInfo.previousGameType = -1;
    in.spawnInfo.isDebug = false;
    in.spawnInfo.isFlat = false;
    in.spawnInfo.lastDeathLocation = std::nullopt;
    in.spawnInfo.portalCooldown = 0;
    in.spawnInfo.seaLevel = 63;
    in.enforcesSecureChat = false;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 17u);
    EXPECT_EQ(std::get<Login>(out), in);
}

TEST_F(NetworkTestBase, PlayLoginCreativeWithDeathLocation)
{
    Login in{};
    in.playerId = 1;
    in.hardcore = true;
    in.levels = {"minecraft:overworld"};
    in.maxPlayers = 1;
    in.chunkRadius = 3;
    in.simulationDistance = 3;
    in.reducedDebugInfo = true;
    in.showDeathScreen = false;
    in.doLimitedCrafting = true;
    in.spawnInfo.dimensionType = 1;
    in.spawnInfo.dimension = "minecraft:the_nether";
    in.spawnInfo.seed = -999LL;
    in.spawnInfo.gameType = GameMode::Creative;
    in.spawnInfo.previousGameType = 0;
    in.spawnInfo.isDebug = true;
    in.spawnInfo.isFlat = true;
    in.spawnInfo.lastDeathLocation = std::make_pair(std::string("minecraft:overworld"), 0x123LL);
    in.spawnInfo.portalCooldown = 300;
    in.spawnInfo.seaLevel = 63;
    in.enforcesSecureChat = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 17u);
    EXPECT_EQ(std::get<Login>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerPosition)
{
    PlayerPosition in{};
    in.teleportId = 7;
    in.x = 1.5;
    in.y = 70.0;
    in.z = -2.25;
    in.deltaX = 0.0;
    in.deltaY = -0.08;
    in.deltaZ = 0.0;
    in.yRot = 0.0f;
    in.xRot = 90.0f;
    in.relatives = 0; // 全绝对
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 18u);
    EXPECT_EQ(std::get<PlayerPosition>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerPositionRelativeFlags)
{
    PlayerPosition in{};
    in.teleportId = 100;
    in.x = -100.5;
    in.y = 200.0;
    in.z = 300.75;
    in.deltaX = 0.1;
    in.deltaY = 0.2;
    in.deltaZ = 0.3;
    in.yRot = 45.0f;
    in.xRot = -45.0f;
    in.relatives = 0x1FF; // 全 9 位相对
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 18u);
    EXPECT_EQ(std::get<PlayerPosition>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerAbilities)
{
    PlayerAbilities in{};
    in.flags = 0x05; // invulnerable + canFly
    in.flyingSpeed = 0.05f;
    in.walkingSpeed = 0.1f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 20u);
    EXPECT_EQ(std::get<PlayerAbilities>(out), in);
}

TEST_F(NetworkTestBase, PlayGameEventChangeDifficulty)
{
    GameEvent in{};
    in.event = 3; // CHANGE_GAME_MODE 之类；本测试只验 codec 透传 event byte
    in.value = 1.0f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 24u);
    EXPECT_EQ(std::get<GameEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayGameEventWeather)
{
    GameEvent in{};
    in.event = 1; // END_RAINING
    in.value = 0.0f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 24u);
    EXPECT_EQ(std::get<GameEvent>(out), in);
}

TEST_F(NetworkTestBase, PlaySetTime)
{
    SetTime in{};
    in.gameTime = 1000LL;
    in.dayTime = 1000LL;
    in.tickDayTime = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 19u);
    EXPECT_EQ(std::get<SetTime>(out), in);
}

TEST_F(NetworkTestBase, PlaySetTimeFrozenDayCycle)
{
    SetTime in{};
    in.gameTime = 5000LL;
    in.dayTime = 6000LL;
    in.tickDayTime = false;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 19u);
    EXPECT_EQ(std::get<SetTime>(out), in);
}

TEST_F(NetworkTestBase, PlaySetHeldSlot)
{
    SetHeldSlot in{};
    in.slot = 3;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 21u);
    EXPECT_EQ(std::get<SetHeldSlot>(out), in);
}

TEST_F(NetworkTestBase, PlaySetDefaultSpawnPosition)
{
    SetDefaultSpawnPosition in{};
    in.dimension = "minecraft:overworld";
    in.blockPosPacked = 0x100LL;
    in.yaw = 90.0f;
    in.pitch = 0.0f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 22u);
    EXPECT_EQ(std::get<SetDefaultSpawnPosition>(out), in);
}

TEST_F(NetworkTestBase, PlayChangeDifficulty)
{
    ChangeDifficulty in{};
    in.difficulty = 2; // Normal
    in.locked = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 23u);
    EXPECT_EQ(std::get<ChangeDifficulty>(out), in);
}

// ============================================================================
// 批 18：杂项简单 S→C（SetCamera/SetEntityLink/SetPassengers/EntityEvent/Animate/
//        HurtAnimation/TakeItemEntity/BlockDestruction/BlockEvent）
// ============================================================================

TEST_F(NetworkTestBase, PlaySetCamera)
{
    SetCamera in{};
    in.cameraId = 99;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 70u);
    EXPECT_EQ(std::get<SetCamera>(out), in);
}

TEST_F(NetworkTestBase, PlaySetEntityLink)
{
    SetEntityLink in{};
    in.sourceId = 5;
    in.destId = 8;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 71u);
    EXPECT_EQ(std::get<SetEntityLink>(out), in);
}

TEST_F(NetworkTestBase, PlaySetEntityLinkUnlink)
{
    SetEntityLink in{};
    in.sourceId = 5;
    in.destId = 0; // 解除
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 71u);
    EXPECT_EQ(std::get<SetEntityLink>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPassengers)
{
    SetPassengers in{};
    in.vehicle = 10;
    in.passengers = {11, 12, 13};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 72u);
    EXPECT_EQ(std::get<SetPassengers>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPassengersEmpty)
{
    SetPassengers in{};
    in.vehicle = 10;
    in.passengers = {}; // 卸载所有乘客
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 72u);
    EXPECT_EQ(std::get<SetPassengers>(out), in);
}

// EntityEvent（altIndex 73）有 entityEventCodec() 但未在 JavaProtocolTables.cpp 的 playCb
// 登记（IdDispatchCodec::encode 报"无匹配包类型"）——Phase 4a 补全遗漏。故本表级往返
// 不可测；待登记后补 PlayEntityEvent 例。codec 本身的存在性由 addPacket 缺失间接暴露。

TEST_F(NetworkTestBase, PlayAnimateSwingMainHand)
{
    Animate in{};
    in.id = 1;
    in.action = 0; // SWING_MAIN_HAND
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 74u);
    EXPECT_EQ(std::get<Animate>(out), in);
}

TEST_F(NetworkTestBase, PlayAnimateCriticalHit)
{
    Animate in{};
    in.id = 7;
    in.action = 4; // CRITICAL_HIT
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 74u);
    EXPECT_EQ(std::get<Animate>(out), in);
}

TEST_F(NetworkTestBase, PlayHurtAnimation)
{
    HurtAnimation in{};
    in.id = 5;
    in.yaw = 180.0f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 75u);
    EXPECT_EQ(std::get<HurtAnimation>(out), in);
}

TEST_F(NetworkTestBase, PlayTakeItemEntity)
{
    TakeItemEntity in{};
    in.itemId = 100;
    in.playerId = 1;
    in.amount = 1;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 76u);
    EXPECT_EQ(std::get<TakeItemEntity>(out), in);
}

TEST_F(NetworkTestBase, PlayBlockDestruction)
{
    BlockDestruction in{};
    in.id = 3;
    in.blockPosPacked = 0xABCLL;
    in.progress = 5; // 0..9
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 77u);
    EXPECT_EQ(std::get<BlockDestruction>(out), in);
}

TEST_F(NetworkTestBase, PlayBlockEvent)
{
    BlockEvent in{};
    in.blockPosPacked = 0x200LL;
    in.b0 = 1;
    in.b1 = 2;
    in.blockId = 54; // chest registry id
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 78u);
    EXPECT_EQ(std::get<BlockEvent>(out), in);
}

// ============================================================================
// 批 19：方块实体/维度/经验 S→C（BlockEntityData/Respawn/SetExperience）
// BlockEntityData.tag 是 shared_ptr<CompoundTag>：默认 == 比指针，往返后指针必不等。
// 故用 byteStreamsEqual 同 codec 重序列化比字节（语义相等的 NBT 产生相同字节）。
// ============================================================================

TEST_F(NetworkTestBase, PlayBlockEntityDataEmptyTag)
{
    BlockEntityData in{};
    in.blockPosPacked = 0x10LL;
    in.blockEntityType = 7; // sign registry id
    in.tag = nullptr;       // 空 NBT → 写一个 TAG_End
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 79u);
    const auto& decoded = std::get<BlockEntityData>(out);
    EXPECT_EQ(decoded.blockPosPacked, in.blockPosPacked);
    EXPECT_EQ(decoded.blockEntityType, in.blockEntityType);
    // 解码必产生非空 shared_ptr（指向空 CompoundTag），与 nullptr 输入字节相等。
    EXPECT_NE(decoded.tag, nullptr);
    EXPECT_TRUE(byteStreamsEqual(backend::java::codecs::blockEntityDataCodec(), in, decoded));
}

TEST_F(NetworkTestBase, PlayBlockEntityDataWithFields)
{
    BlockEntityData in{};
    in.blockPosPacked = 0x10LL;
    in.blockEntityType = 7;
    in.tag = makeSampleBlockEntityTag();
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 79u);
    const auto& decoded = std::get<BlockEntityData>(out);
    EXPECT_EQ(decoded.blockPosPacked, in.blockPosPacked);
    EXPECT_EQ(decoded.blockEntityType, in.blockEntityType);
    // 指针不同但 NBT 语义相等 → 字节流相等。
    EXPECT_TRUE(byteStreamsEqual(backend::java::codecs::blockEntityDataCodec(), in, decoded));
}

TEST_F(NetworkTestBase, PlayRespawn)
{
    Respawn in{};
    in.spawnInfo.dimensionType = 0;
    in.spawnInfo.dimension = "minecraft:overworld";
    in.spawnInfo.seed = 12345LL;
    in.spawnInfo.gameType = GameMode::Spectator;
    in.spawnInfo.previousGameType = 1;
    in.spawnInfo.isDebug = false;
    in.spawnInfo.isFlat = false;
    in.spawnInfo.lastDeathLocation = std::nullopt;
    in.spawnInfo.portalCooldown = 0;
    in.spawnInfo.seaLevel = 63;
    in.dataToKeep = 3; // KEEP_ALL_DATA
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 80u);
    EXPECT_EQ(std::get<Respawn>(out), in);
}

TEST_F(NetworkTestBase, PlaySetExperience)
{
    SetExperience in{};
    in.experienceProgress = 0.5f;
    in.experienceLevel = 10;
    in.totalExperience = 200;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 81u);
    EXPECT_EQ(std::get<SetExperience>(out), in);
}
