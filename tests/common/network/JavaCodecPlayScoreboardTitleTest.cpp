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

// 批 12/13/15/16：进度 + 记分板 + 世界边界 + 地图 S→C/mixed Java wire codec 往返。
// 表级往返（roundTripGeneric）兼测 packetID 分发 + payload codec。altIndex 取自
// IrPacket.hpp variant 顺序：SelectAdvancementTab=49（S→C）/SeenAdvancements=50（C→S，批12）；
// SetObjective=51/SetScore=52/ResetScore=53/SetDisplayObjective=54/SetPlayerTeam=55（批13）；
// InitializeBorder=61/SetBorderCenter=62/SetBorderLerpSize=63/SetBorderSize=64/
// SetBorderWarningDelay=65/SetBorderWarningDistance=66（批15）；MapItemData=67（批16）。
// SetObjective method 0/2 才写 displayName/renderType/numberFormat；SetPlayerTeam method 0/2
// 才写 parameters、method 0/3/4 才写 players——输入只 set 上线字段，未上线字段保持默认。
// MapPatch width=0 是 absent 哨兵，故 present 例须 width>0。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecsExtended.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;

// ============================================================================
// 批 12：进度 mixed（SelectAdvancementTab S→C + SeenAdvancements C→S）
// SelectAdvancementTab present=false 表关闭标签页；SeenAdvancements action 0/1。
// ============================================================================

TEST_F(NetworkTestBase, PlaySelectAdvancementTabPresent)
{
    SelectAdvancementTab in{};
    in.present = true;
    in.tab = "minecraft:adventure/root";
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 49u);
    EXPECT_EQ(std::get<SelectAdvancementTab>(out), in);
}

TEST_F(NetworkTestBase, PlaySelectAdvancementTabAbsent)
{
    SelectAdvancementTab in{};
    in.present = false;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 49u);
    EXPECT_EQ(std::get<SelectAdvancementTab>(out), in);
}

TEST_F(NetworkTestBase, PlaySeenAdvancementsOpenedTab)
{
    SeenAdvancements in{};
    in.action = 0; // OPENED_TAB
    in.tab = "minecraft:story/mine_diamond";
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 50u);
    EXPECT_EQ(std::get<SeenAdvancements>(out), in);
}

TEST_F(NetworkTestBase, PlaySeenAdvancementsClosedScreen)
{
    SeenAdvancements in{};
    in.action = 1; // CLOSED_SCREEN
    auto out = roundTripGeneric(*tables()->playSb, PlayPacket{in});
    ASSERT_EQ(out.index(), 50u);
    EXPECT_EQ(std::get<SeenAdvancements>(out), in);
}

// ============================================================================
// 批 13：记分板 S→C（SetObjective/SetScore/ResetScore/SetDisplayObjective/SetPlayerTeam）
// SetObjective method 0/2 写 displayName/renderType/numberFormat；1 不写。
// SetPlayerTeam method 0/2 写 parameters；0/3/4 写 players；1 全不写。
// ============================================================================

TEST_F(NetworkTestBase, PlaySetObjectiveAdd)
{
    SetObjective in{};
    in.objectiveName = "obj1";
    in.method = 0;                                    // ADD
    in.displayName = {0x01, 'S', 'c', 'o', 'r', 'e'}; // opaque Component
    in.renderType = 0;                                // INTEGER
    in.numberFormat = {0x02, 0x03};                   // opaque Optional<NumberFormat>
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 51u);
    EXPECT_EQ(std::get<SetObjective>(out), in);
}

TEST_F(NetworkTestBase, PlaySetObjectiveRemove)
{
    SetObjective in{};
    in.objectiveName = "obj1";
    in.method = 1; // REMOVE：无额外字段
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 51u);
    EXPECT_EQ(std::get<SetObjective>(out), in);
}

TEST_F(NetworkTestBase, PlaySetObjectiveChange)
{
    SetObjective in{};
    in.objectiveName = "obj2";
    in.method = 2; // CHANGE
    in.displayName = {0x04, 'X'};
    in.renderType = 1; // HEARTS
    in.numberFormat = {};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 51u);
    EXPECT_EQ(std::get<SetObjective>(out), in);
}

TEST_F(NetworkTestBase, PlaySetScore)
{
    SetScore in{};
    in.owner = "Player1";
    in.objectiveName = "obj1";
    in.score = 42;
    in.display = {0x05};      // opaque Optional<Component>
    in.numberFormat = {0x06}; // opaque Optional<NumberFormat>
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 52u);
    EXPECT_EQ(std::get<SetScore>(out), in);
}

TEST_F(NetworkTestBase, PlayResetScoreSingleObjective)
{
    ResetScore in{};
    in.owner = "Player1";
    in.objectiveName = std::string("obj1");
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 53u);
    EXPECT_EQ(std::get<ResetScore>(out), in);
}

TEST_F(NetworkTestBase, PlayResetScoreAllObjectives)
{
    ResetScore in{};
    in.owner = "Player1";
    in.objectiveName = std::nullopt; // 重置该 owner 所有 objective
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 53u);
    EXPECT_EQ(std::get<ResetScore>(out), in);
}

TEST_F(NetworkTestBase, PlaySetDisplayObjective)
{
    SetDisplayObjective in{};
    in.slot = 0; // list slot
    in.objectiveName = "obj1";
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 54u);
    EXPECT_EQ(std::get<SetDisplayObjective>(out), in);
}

TEST_F(NetworkTestBase, PlaySetDisplayObjectiveClearSlot)
{
    SetDisplayObjective in{};
    in.slot = 14;
    in.objectiveName = ""; // 空串表清除该 slot
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 54u);
    EXPECT_EQ(std::get<SetDisplayObjective>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPlayerTeamAdd)
{
    SetPlayerTeam in{};
    in.name = "team_red";
    in.method = 0;                      // ADD：写 parameters + players
    in.parameters = {0x07, 0x08, 0x09}; // opaque Parameters
    in.players = {"Player1", "Player2"};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 55u);
    EXPECT_EQ(std::get<SetPlayerTeam>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPlayerTeamRemove)
{
    SetPlayerTeam in{};
    in.name = "team_red";
    in.method = 1; // REMOVE：无 parameters 无 players
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 55u);
    EXPECT_EQ(std::get<SetPlayerTeam>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPlayerTeamChange)
{
    SetPlayerTeam in{};
    in.name = "team_blue";
    in.method = 2; // CHANGE：写 parameters 不写 players
    in.parameters = {0x0A};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 55u);
    EXPECT_EQ(std::get<SetPlayerTeam>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPlayerTeamJoin)
{
    SetPlayerTeam in{};
    in.name = "team_red";
    in.method = 3; // JOIN：写 players 不写 parameters
    in.players = {"Player3"};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 55u);
    EXPECT_EQ(std::get<SetPlayerTeam>(out), in);
}

TEST_F(NetworkTestBase, PlaySetPlayerTeamLeave)
{
    SetPlayerTeam in{};
    in.name = "team_red";
    in.method = 4; // LEAVE：写 players
    in.players = {"Player1", "Player2", "Player3"};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 55u);
    EXPECT_EQ(std::get<SetPlayerTeam>(out), in);
}

// ============================================================================
// 批 15：世界边界 S→C（6 包：InitializeBorder/SetBorderCenter/SetBorderLerpSize/
//        SetBorderSize/SetBorderWarningDelay/SetBorderWarningDistance）
// ============================================================================

TEST_F(NetworkTestBase, PlayInitializeBorder)
{
    InitializeBorder in{};
    in.newCenterX = 0.0;
    in.newCenterZ = 0.0;
    in.oldSize = 60000000.0;
    in.newSize = 60000000.0;
    in.lerpTime = 0LL;
    in.newAbsoluteMaxSize = 29999984;
    in.warningBlocks = 5;
    in.warningTime = 15;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 61u);
    EXPECT_EQ(std::get<InitializeBorder>(out), in);
}

TEST_F(NetworkTestBase, PlaySetBorderCenter)
{
    SetBorderCenter in{};
    in.newCenterX = 1024.5;
    in.newCenterZ = -512.25;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 62u);
    EXPECT_EQ(std::get<SetBorderCenter>(out), in);
}

TEST_F(NetworkTestBase, PlaySetBorderLerpSize)
{
    SetBorderLerpSize in{};
    in.oldSize = 1000.0;
    in.newSize = 500.0;
    in.lerpTime = 1000LL;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 63u);
    EXPECT_EQ(std::get<SetBorderLerpSize>(out), in);
}

TEST_F(NetworkTestBase, PlaySetBorderSize)
{
    SetBorderSize in{};
    in.size = 2000.0;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 64u);
    EXPECT_EQ(std::get<SetBorderSize>(out), in);
}

TEST_F(NetworkTestBase, PlaySetBorderWarningDelay)
{
    SetBorderWarningDelay in{};
    in.warningDelay = 30;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 65u);
    EXPECT_EQ(std::get<SetBorderWarningDelay>(out), in);
}

TEST_F(NetworkTestBase, PlaySetBorderWarningDistance)
{
    SetBorderWarningDistance in{};
    in.warningBlocks = 8;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 66u);
    EXPECT_EQ(std::get<SetBorderWarningDistance>(out), in);
}

// ============================================================================
// 批 16：地图 S→C（MapItemData）
// Optional<List<MapDecoration>> 与 Optional<MapPatch> 均可空；MapPatch present 须 width>0。
// ============================================================================

TEST_F(NetworkTestBase, PlayMapItemDataEmpty)
{
    MapItemData in{};
    in.mapId = 5;
    in.scale = 3;
    in.locked = true;
    in.decorations = std::nullopt;
    in.colorPatch = std::nullopt;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 67u);
    EXPECT_EQ(std::get<MapItemData>(out), in);
}

TEST_F(NetworkTestBase, PlayMapItemDataWithDecorationsOnly)
{
    MapItemData in{};
    in.mapId = 1;
    in.scale = 0;
    in.locked = false;
    MapDecorationWire d{};
    d.typeRegistryIdPlusOne = 1; // PLAYER
    d.x = 10;
    d.y = -5;
    d.rotation = 4;
    d.name = std::nullopt;
    MapDecorationWire d2{};
    d2.typeRegistryIdPlusOne = 6; // FRAME
    d2.x = 0;
    d2.y = 0;
    d2.rotation = 0;
    d2.name = std::vector<u8>{0x01, 'F'}; // opaque Component
    in.decorations = std::vector<MapDecorationWire>{d, d2};
    in.colorPatch = std::nullopt;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 67u);
    EXPECT_EQ(std::get<MapItemData>(out), in);
}

TEST_F(NetworkTestBase, PlayMapItemDataWithColorPatch)
{
    MapItemData in{};
    in.mapId = 7;
    in.scale = 4;
    in.locked = true;
    MapPatchWire patch{};
    patch.startX = 10;
    patch.startY = 20;
    patch.width = 4;
    patch.height = 2;
    patch.colors = std::vector<u8>(8, 0xAB); // 4*2=8 字节
    in.decorations = std::nullopt;
    in.colorPatch = MapPatchWire{patch};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 67u);
    EXPECT_EQ(std::get<MapItemData>(out), in);
}

TEST_F(NetworkTestBase, PlayMapItemDataFull)
{
    MapItemData in{};
    in.mapId = 99;
    in.scale = 2;
    in.locked = false;
    MapDecorationWire d{};
    d.typeRegistryIdPlusOne = 1;
    d.x = 1;
    d.y = 1;
    d.rotation = 8;
    d.name = std::vector<u8>{0x02, 'N', 'P', 'C'};
    in.decorations = std::vector<MapDecorationWire>{d};
    MapPatchWire patch{};
    patch.startX = 0;
    patch.startY = 0;
    patch.width = 1;
    patch.height = 1;
    patch.colors = std::vector<u8>{0xFF};
    in.colorPatch = MapPatchWire{patch};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 67u);
    EXPECT_EQ(std::get<MapItemData>(out), in);
}
