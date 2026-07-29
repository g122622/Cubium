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

// 批 5/6/9：实体同步 + 区块/方块 + 声音 S→C Java wire codec 往返。
// 表级往返（roundTripGeneric）兼测 packetID 分发 + payload codec。altIndex 取自
// IrPacket.hpp variant 顺序：AddEntity=28/RemoveEntities=29/TeleportEntity=30/
// MoveEntityPos=31/MoveEntityPosRot=32/MoveEntityRot=33/SetEntityMotion=34/RotateHead=35/
// SetEntityData=27（批5）；LevelChunkWithLight=36/LightUpdate=37/BlockUpdate=38（批6）；
// PlaySound=43/StopSound=44/SoundEntity=45/LevelEvent=46（批9）。
// AddEntity.movement / SetEntityMotion.x/y/z 经 LpVec3 有损量化：零向量精确往返（0x00 单字节
// 特例），非零向量用 byteStreamsEqual（重量化字节幂等）。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <array>
#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;

namespace {

std::array<u8, 16> sampleUuid()
{
    return {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
}

} // namespace

// ============================================================================
// 批 5：实体同步 S→C（AddEntity/RemoveEntities/TeleportEntity/MoveEntity×3/
//       SetEntityMotion/RotateHead/SetEntityData）
// ============================================================================

TEST_F(NetworkTestBase, PlayAddEntityZeroMovement)
{
    AddEntity in{};
    in.entityId = 100;
    in.uuid = sampleUuid();
    in.entityTypeId = 1; // zombie registry id（仅透传）
    in.x = 1.5;
    in.y = 64.0;
    in.z = -2.5;
    in.movementX = 0.0;
    in.movementY = 0.0;
    in.movementZ = 0.0; // 零向量 → LpVec3 精确往返
    in.yRot = 0;
    in.xRot = 0;
    in.yHeadRot = 0;
    in.data = 0;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 28u);
    EXPECT_EQ(std::get<AddEntity>(out), in);
}

TEST_F(NetworkTestBase, PlayAddEntityNonZeroMovementByteStream)
{
    // 非零 movement 经 LpVec3 有损量化，解码值 ≠ 原值，故用 byteStreamsEqual（重量化幂等）。
    AddEntity in{};
    in.entityId = 101;
    in.uuid = sampleUuid();
    in.entityTypeId = 2;
    in.x = 100.5;
    in.y = 70.0;
    in.z = 200.25;
    in.movementX = 0.3;
    in.movementY = -0.08;
    in.movementZ = 0.1;
    in.yRot = 45;
    in.xRot = -30;
    in.yHeadRot = 60;
    in.data = 7;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 28u);
    const auto& decoded = std::get<AddEntity>(out);
    EXPECT_EQ(decoded.entityId, in.entityId);
    EXPECT_EQ(decoded.uuid, in.uuid);
    EXPECT_EQ(decoded.x, in.x);
    EXPECT_EQ(decoded.y, in.y);
    EXPECT_EQ(decoded.z, in.z);
    EXPECT_TRUE(byteStreamsEqual(backend::java::codecs::addEntityCodec(), in, decoded));
}

TEST_F(NetworkTestBase, PlayRemoveEntities)
{
    RemoveEntities in{};
    in.entityIds = {1, 2, 3, 100};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 29u);
    EXPECT_EQ(std::get<RemoveEntities>(out), in);
}

TEST_F(NetworkTestBase, PlayRemoveEntitiesEmpty)
{
    RemoveEntities in{};
    in.entityIds = {};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 29u);
    EXPECT_EQ(std::get<RemoveEntities>(out), in);
}

TEST_F(NetworkTestBase, PlayTeleportEntity)
{
    TeleportEntity in{};
    in.entityId = 50;
    in.x = 256.5;
    in.y = 64.0;
    in.z = -128.25;
    in.deltaX = 0.0;
    in.deltaY = 0.0;
    in.deltaZ = 0.0;
    in.yRot = 90.0f;
    in.xRot = 0.0f;
    in.relatives = 0;
    in.onGround = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 30u);
    EXPECT_EQ(std::get<TeleportEntity>(out), in);
}

TEST_F(NetworkTestBase, PlayMoveEntityPos)
{
    MoveEntityPos in{};
    in.entityId = 5;
    in.deltaX = 100;
    in.deltaY = -50;
    in.deltaZ = 200;
    in.onGround = false;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 31u);
    EXPECT_EQ(std::get<MoveEntityPos>(out), in);
}

TEST_F(NetworkTestBase, PlayMoveEntityPosRot)
{
    MoveEntityPosRot in{};
    in.entityId = 5;
    in.deltaX = 1;
    in.deltaY = 2;
    in.deltaZ = 3;
    in.yRot = 90;
    in.xRot = 45;
    in.onGround = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 32u);
    EXPECT_EQ(std::get<MoveEntityPosRot>(out), in);
}

TEST_F(NetworkTestBase, PlayMoveEntityRot)
{
    MoveEntityRot in{};
    in.entityId = 5;
    in.yRot = 180;
    in.xRot = -90;
    in.onGround = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 33u);
    EXPECT_EQ(std::get<MoveEntityRot>(out), in);
}

TEST_F(NetworkTestBase, PlaySetEntityMotionZero)
{
    SetEntityMotion in{};
    in.entityId = 7;
    in.x = 0.0;
    in.y = 0.0;
    in.z = 0.0; // 零向量 → LpVec3 精确往返
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 34u);
    EXPECT_EQ(std::get<SetEntityMotion>(out), in);
}

TEST_F(NetworkTestBase, PlaySetEntityMotionNonZeroByteStream)
{
    SetEntityMotion in{};
    in.entityId = 7;
    in.x = 0.5;
    in.y = -0.2;
    in.z = 0.1;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 34u);
    const auto& decoded = std::get<SetEntityMotion>(out);
    EXPECT_EQ(decoded.entityId, in.entityId);
    EXPECT_TRUE(byteStreamsEqual(backend::java::codecs::setEntityMotionCodec(), in, decoded));
}

TEST_F(NetworkTestBase, PlayRotateHead)
{
    RotateHead in{};
    in.entityId = 9;
    in.yHeadRot = -45;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 35u);
    EXPECT_EQ(std::get<RotateHead>(out), in);
}

TEST_F(NetworkTestBase, PlaySetEntityDataEmpty)
{
    SetEntityData in{};
    in.entityId = 11;
    in.packedItems = {0xFF}; // 仅 EOF 终止符（空元数据）
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 27u);
    EXPECT_EQ(std::get<SetEntityData>(out), in);
}

TEST_F(NetworkTestBase, PlaySetEntityDataWithBytes)
{
    SetEntityData in{};
    in.entityId = 11;
    // 任意非空 opaque 元数据字节（codec 透传，吞掉 entityId 后所有尾字节）
    in.packedItems = {0x01, 0x00, 0x05, 0x42, 0x02, 0x01, 0x01, 0xFF};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 27u);
    EXPECT_EQ(std::get<SetEntityData>(out), in);
}

// ============================================================================
// 批 6：区块/方块 S→C（LevelChunkWithLight/LightUpdate/BlockUpdate）
// LevelChunkWithLight IR 为 vanilla 1.21.11 结构化字段（heightmaps/sections/blockEntities/
// lightMasks/lightUpdates），codec 按 vanilla wire 编码（i32x+i32z+heightmaps+sectionBuf+
// blockEntities+4BitSet+2List，无外层长度前缀）。此处测表级往返对称性。
// ============================================================================

TEST_F(NetworkTestBase, PlayLevelChunkWithLightEmpty)
{
    LevelChunkWithLight in{};
    in.x = 0;
    in.z = 0;
    // 全空结构化字段：空 heightmaps/sections/blockEntities/lightMasks/lightUpdates。
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 36u);
    EXPECT_EQ(std::get<LevelChunkWithLight>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelChunkWithLightFilled)
{
    LevelChunkWithLight in{};
    in.x = -5;
    in.z = 12;

    // 单段 section：states/biomes 均 SingleValue palette（bits=0，1 个 globalId，无 storage）。
    ChunkSectionWire sec{};
    sec.nonEmptyBlockCount = 42;
    sec.states.bits = 0;
    sec.states.paletteGlobalIds = {1u}; // minecraft:stone
    sec.biomes.bits = 0;
    sec.biomes.paletteGlobalIds = {40u}; // plains biome registry id
    in.sections = {sec};

    // 单条高度图：WORLD_SURFACE(typeId=1)，空 data（空列）。
    HeightmapEntryWire hm{};
    hm.typeId = 1;
    in.heightmaps = {hm};

    // 单段 sky 光照：skyYMask bit5 置位 + 一条 2048 字节 nibble。
    in.lightMasks[0] = std::vector<u64>{1ULL << 5};
    in.lightUpdates[0] = std::vector<std::vector<u8>>{std::vector<u8>(2048, 0x5A)};

    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 36u);
    EXPECT_EQ(std::get<LevelChunkWithLight>(out), in);
}

TEST_F(NetworkTestBase, PlayLightUpdate)
{
    // 1.21.11 ClientboundLightUpdatePacket：VarInt(x)+VarInt(z)+4×BitSet+2×List<byte[≤2048]>。
    // 构造单段 sky 光照更新：bitIndex=5（光照段 Y=0，minLightSection=-5）置位，附一条 2048 字节 nibble。
    LightUpdate in{};
    in.x = 3;
    in.z = -7;
    in.lightMasks[0] = std::vector<i64>{1LL << 5}; // skyYMask：bit5 置位
    in.lightUpdates[0] = std::vector<std::vector<u8>>{std::vector<u8>(2048, 0x5A)};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 37u);
    EXPECT_EQ(std::get<LightUpdate>(out), in);
}

TEST_F(NetworkTestBase, PlayBlockUpdate)
{
    BlockUpdate in{};
    in.blockPosPacked = 0x200LL;
    in.blockStateId = 15; // vanilla block state id（透传）
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 38u);
    EXPECT_EQ(std::get<BlockUpdate>(out), in);
}

// ============================================================================
// 批 9：声音 S→C（PlaySound/StopSound/SoundEntity/LevelEvent）
// PlaySound/SoundEntity 的 soundHolder 是 opaque Holder<SoundEvent> 字节透传。
// StopSound 按 flags 条件编 source(flags&1)/name(flags&2)。
// ============================================================================

TEST_F(NetworkTestBase, PlaySoundDirect)
{
    PlaySound in{};
    in.soundHolder = {0x00, 0x0A, 'b', 'l', 'o', 'c', 'k', '.', 'h', 'i', 't'}; // VarInt(0)+len+id 内联
    in.source = 1;                                                              // BLOCK
    in.x = 10;
    in.y = 64;
    in.z = -5;
    in.volume = 1.0f;
    in.pitch = 0.5f;
    in.seed = 12345LL;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 43u);
    EXPECT_EQ(std::get<PlaySound>(out), in);
}

TEST_F(NetworkTestBase, PlayStopSoundAll)
{
    StopSound in{};
    in.flags = 0; // 既无 source 也无 name（停止全部）
    in.source = 0;
    in.name = "";
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 44u);
    EXPECT_EQ(std::get<StopSound>(out), in);
}

TEST_F(NetworkTestBase, PlayStopSoundByName)
{
    StopSound in{};
    in.flags = 0x02; // 仅 name
    in.source = 0;
    in.name = "minecraft:block.chest.close";
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 44u);
    EXPECT_EQ(std::get<StopSound>(out), in);
}

TEST_F(NetworkTestBase, PlayStopSoundBySourceAndName)
{
    StopSound in{};
    in.flags = 0x03; // source + name
    in.source = 2;   // MUSIC
    in.name = "minecraft:music.menu";
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 44u);
    EXPECT_EQ(std::get<StopSound>(out), in);
}

TEST_F(NetworkTestBase, PlaySoundEntity)
{
    SoundEntity in{};
    in.soundHolder = {0x00, 0x08, 'm', 'o', 'b', '.', 'h', 'i', 't'};
    in.source = 3; // NEUTRAL
    in.entityId = 42;
    in.volume = 0.7f;
    in.pitch = 1.0f;
    in.seed = -1LL;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 45u);
    EXPECT_EQ(std::get<SoundEntity>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelEvent)
{
    LevelEvent in{};
    in.type = 2001; // block break particles
    in.blockPosPacked = 0x300LL;
    in.data = 15; // block state id
    in.globalEvent = false;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 46u);
    EXPECT_EQ(std::get<LevelEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelEventGlobal)
{
    LevelEvent in{};
    in.type = 1015; // thunder
    in.blockPosPacked = 0;
    in.data = 0;
    in.globalEvent = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 46u);
    EXPECT_EQ(std::get<LevelEvent>(out), in);
}
