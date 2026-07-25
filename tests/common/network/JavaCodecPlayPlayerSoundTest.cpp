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

// 批 8/10/11/14：玩家列表 + 粒子/爆炸 + Boss 条 + 标题 S→C Java wire codec 往返。
// 表级往返（roundTripGeneric）兼测 packetID 分发 + payload codec。altIndex 取自
// IrPacket.hpp variant 顺序：PlayerInfoUpdate=25/PlayerInfoRemove=26（批8）；
// LevelParticles=47/BossEvent=48（批10/11）；SetTitleText=56/SetSubtitleText=57/
// SetActionBarText=58/SetTitlesAnimation=59/ClearTitles=60（批14）；Explosion=82（批10）。
// PlayerInfoUpdate 的 actions 位掩码按 set 的位顺序写 per-entry 负载；INITIALIZE_CHAT 与
// UPDATE_DISPLAY_NAME 双端均写 Bool(false) 不携带 session/displayName，自洽故直接 ==。
// ParticleOptions 按 type 分发：Simple/Block/Item/Dust/Color/Vibration/Trail 八分支。
// BossEvent/Explosion 按条件字段——输入只 set 该 operation/hasKnockback 真正上线的字段，
// 未上线字段保持默认，避免解码默认值与输入非默认值不等。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecsExtended.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/particle/ParticleTypes.hpp"

#include <gtest/gtest.h>

#include <array>
#include <vector>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;
using namespace mc;
using namespace mc::particle;

// PlayerInfoUpdate action 位常量定义在 player_info_detail 命名空间内，引入别名简化引用。
namespace {
constexpr u16 kAdd = backend::java::codecs::player_info_detail::kActionAddPlayer;
constexpr u16 kInitChat = backend::java::codecs::player_info_detail::kActionInitializeChat;
constexpr u16 kGameMode = backend::java::codecs::player_info_detail::kActionUpdateGameMode;
constexpr u16 kListed = backend::java::codecs::player_info_detail::kActionUpdateListed;
constexpr u16 kLatency = backend::java::codecs::player_info_detail::kActionUpdateLatency;
constexpr u16 kDisplayName = backend::java::codecs::player_info_detail::kActionUpdateDisplayName;
constexpr u16 kListOrder = backend::java::codecs::player_info_detail::kActionUpdateListOrder;
constexpr u16 kHat = backend::java::codecs::player_info_detail::kActionUpdateHat;
} // namespace

namespace {

std::array<u8, 16> sampleUuid()
{
    return {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};
}

} // namespace

// ============================================================================
// 批 8：玩家列表 S→C（PlayerInfoUpdate/PlayerInfoRemove）
// actions 位掩码：bit0=ADD_PLAYER bit1=INITIALIZE_CHAT bit2=UPDATE_GAME_MODE
//                 bit3=UPDATE_LISTED bit4=UPDATE_LATENCY bit5=UPDATE_DISPLAY_NAME
//                 bit6=UPDATE_LIST_ORDER bit7=UPDATE_HAT
// ============================================================================

TEST_F(NetworkTestBase, PlayPlayerInfoUpdateAddPlayerOnly)
{
    PlayerInfoUpdate in{};
    in.actions = kAdd; // 仅 bit0
    PlayerInfoEntry e{};
    e.uuid = sampleUuid();
    e.name = std::string("Alice");
    e.properties = {{"textures", "abc"}};
    in.entries = {e};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 25u);
    EXPECT_EQ(std::get<PlayerInfoUpdate>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerInfoUpdateAllCarriedBits)
{
    // 设置所有我方承载字段的位（ADD/GAME_MODE/LISTED/LATENCY/LIST_ORDER/HAT），
    // 以及 INITIALIZE_CHAT/UPDATE_DISPLAY_NAME（双端写 Bool(false) 不携带负载，自洽）。
    PlayerInfoUpdate in{};
    in.actions = kAdd | kInitChat | kGameMode | kListed | kLatency | kDisplayName | kListOrder | kHat;
    PlayerInfoEntry e{};
    e.uuid = sampleUuid();
    e.name = std::string("Bob");
    e.properties = {};
    e.gameMode = 1; // Creative
    e.listed = true;
    e.latency = 50;
    e.listOrder = 3;
    e.showHat = true;
    in.entries = {e};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 25u);
    EXPECT_EQ(std::get<PlayerInfoUpdate>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerInfoUpdateMultipleEntries)
{
    PlayerInfoUpdate in{};
    in.actions = kLatency; // 仅 latency 位
    PlayerInfoEntry a{};
    a.uuid = sampleUuid();
    a.latency = 10;
    PlayerInfoEntry b{};
    b.uuid = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    b.latency = 200;
    in.entries = {a, b};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 25u);
    EXPECT_EQ(std::get<PlayerInfoUpdate>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerInfoRemove)
{
    PlayerInfoRemove in{};
    in.uuids = {
        sampleUuid(), {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 26u);
    EXPECT_EQ(std::get<PlayerInfoRemove>(out), in);
}

TEST_F(NetworkTestBase, PlayPlayerInfoRemoveEmpty)
{
    PlayerInfoRemove in{};
    in.uuids = {};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 26u);
    EXPECT_EQ(std::get<PlayerInfoRemove>(out), in);
}

// ============================================================================
// 批 10：粒子/爆炸 S→C（LevelParticles 8 type 分支 + Explosion）
// ParticleOptions 按 type 分发：Simple=无额外字节 / Block=VarInt(blockStateId) /
// Item=ItemStack wire / Dust(及 Redstone)=INT color + FLOAT scale /
// DustColorTransition=fromColor+toColor+scale / EntityEffect|Flash|TintedLeaves=INT color /
// Vibration=PositionSource+arrivalInTicks / Trail=Vec3+INT color+VAR_INT duration。
// ============================================================================

TEST_F(NetworkTestBase, PlayLevelParticlesSimpleType)
{
    LevelParticles in{};
    in.overrideLimiter = false;
    in.alwaysShow = true;
    in.x = 1.5;
    in.y = 64.0;
    in.z = -2.5;
    in.xDist = 0.1f;
    in.yDist = 0.2f;
    in.zDist = 0.3f;
    in.maxSpeed = 0.5f;
    in.count = 10;
    in.particle.type = ParticleTypeId::Cloud; // SimpleParticleType，无 options 字节
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesBlockType)
{
    LevelParticles in{};
    in.overrideLimiter = true;
    in.alwaysShow = false;
    in.x = 0.0;
    in.y = 0.0;
    in.z = 0.0;
    in.xDist = 0.0f;
    in.yDist = 0.0f;
    in.zDist = 0.0f;
    in.maxSpeed = 0.0f;
    in.count = 1;
    in.particle.type = ParticleTypeId::Block;
    in.particle.blockStateId = 15; // stone default state
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesItemType)
{
    LevelParticles in{};
    in.count = 5;
    in.maxSpeed = 0.1f;
    in.particle.type = ParticleTypeId::Item;
    in.particle.item.itemId = 1; // stone
    in.particle.item.count = 1;
    in.particle.item.componentsPatch = {}; // 空栈内联 patch（VarInt 0）
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesDustType)
{
    LevelParticles in{};
    in.count = 20;
    in.maxSpeed = 0.4f;
    in.particle.type = ParticleTypeId::Dust;
    in.particle.color = 0xFFFF0000u; // ARGB 红
    in.particle.scale = 1.5f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesDustColorTransitionType)
{
    LevelParticles in{};
    in.count = 3;
    in.particle.type = ParticleTypeId::DustColorTransition;
    in.particle.fromColor = 0xFF0000FFu; // 蓝
    in.particle.toColor = 0xFFFF00FFu;   // 品红
    in.particle.scale = 0.8f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesColorType)
{
    LevelParticles in{};
    in.count = 7;
    in.particle.type = ParticleTypeId::EntityEffect; // ColorParticleOption（INT color）
    in.particle.color = 0xFF00FF00u;                 // 绿
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesVibrationBlockSourceType)
{
    LevelParticles in{};
    in.count = 1;
    in.particle.type = ParticleTypeId::Vibration;
    in.particle.vibrationSourceKind = 0;           // Block 源
    in.particle.vibrationBlockPosPacked = 0x1FFLL; // BlockPos.asLong
    in.particle.arrivalInTicks = 20;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesVibrationEntitySourceType)
{
    LevelParticles in{};
    in.count = 1;
    in.particle.type = ParticleTypeId::Vibration;
    in.particle.vibrationSourceKind = 1; // Entity 源
    in.particle.vibrationEntityId = 42;
    in.particle.vibrationYOffset = 1.5f;
    in.particle.arrivalInTicks = 40;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayLevelParticlesTrailType)
{
    LevelParticles in{};
    in.count = 1;
    in.particle.type = ParticleTypeId::Trail;
    in.particle.trailTargetX = 10.5;
    in.particle.trailTargetY = 64.0;
    in.particle.trailTargetZ = -10.25;
    in.particle.color = 0xFFFFFFFFu;
    in.particle.trailDuration = 30;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 47u);
    EXPECT_EQ(std::get<LevelParticles>(out), in);
}

TEST_F(NetworkTestBase, PlayExplosionWithoutKnockback)
{
    Explosion in{};
    in.centerX = 1.5;
    in.centerY = 64.0;
    in.centerZ = -2.5;
    in.radius = 2.0f;
    in.blockCount = 5;
    in.hasPlayerKnockback = false;                         // 不写 knockback 三元组
    in.explosionParticle.type = ParticleTypeId::Explosion; // SimpleParticleType
    in.explosionSound.direct = true;
    in.explosionSound.identifier = "minecraft:entity.generic.explode";
    in.explosionSound.hasFixedRange = true;
    in.explosionSound.fixedRange = 16.0f;
    in.blockParticles = {}; // 空粒子表
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 82u);
    EXPECT_EQ(std::get<Explosion>(out), in);
}

TEST_F(NetworkTestBase, PlayExplosionWithKnockbackAndBlockParticles)
{
    Explosion in{};
    in.centerX = 0.0;
    in.centerY = 0.0;
    in.centerZ = 0.0;
    in.radius = 4.0f;
    in.blockCount = 3;
    in.hasPlayerKnockback = true;
    in.knockbackX = 0.5;
    in.knockbackY = 0.8;
    in.knockbackZ = -0.3;
    in.explosionParticle.type = ParticleTypeId::Explosion;
    in.explosionSound.direct = true;
    in.explosionSound.identifier = "minecraft:entity.generic.explode";
    in.explosionSound.hasFixedRange = false;
    ExplosionParticleInfo pi{};
    pi.particle.type = ParticleTypeId::Block;
    pi.particle.blockStateId = 1;
    pi.scaling = 1.0f;
    pi.speed = 0.5f;
    in.blockParticles = {pi};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 82u);
    EXPECT_EQ(std::get<Explosion>(out), in);
}

// ============================================================================
// 批 11：Boss 条 S→C（BossEvent 6 operation 分发）
// operation 0=ADD(name+progress+color+overlay+flags) 1=REMOVE 2=UPDATE_PROGRESS(progress)
//          3=UPDATE_NAME(name) 4=UPDATE_STYLE(color+overlay) 5=UPDATE_PROPERTIES(flags)
// ============================================================================

TEST_F(NetworkTestBase, PlayBossEventAdd)
{
    BossEvent in{};
    in.uuid = sampleUuid();
    in.operation = 0;                     // ADD
    in.name = {0x0A, 'B', 'o', 's', 's'}; // opaque Component
    in.progress = 0.5f;
    in.color = 1;   // PINK
    in.overlay = 0; // PROGRESS
    in.flags = 0x05;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 48u);
    EXPECT_EQ(std::get<BossEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayBossEventRemove)
{
    BossEvent in{};
    in.uuid = sampleUuid();
    in.operation = 1; // REMOVE：switch 内无额外字节
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 48u);
    EXPECT_EQ(std::get<BossEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayBossEventUpdateProgress)
{
    BossEvent in{};
    in.uuid = sampleUuid();
    in.operation = 2; // UPDATE_PROGRESS
    in.progress = 0.75f;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 48u);
    EXPECT_EQ(std::get<BossEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayBossEventUpdateName)
{
    BossEvent in{};
    in.uuid = sampleUuid();
    in.operation = 3; // UPDATE_NAME
    in.name = {0x0B, 'X', 'Y', 'Z'};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 48u);
    EXPECT_EQ(std::get<BossEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayBossEventUpdateStyle)
{
    BossEvent in{};
    in.uuid = sampleUuid();
    in.operation = 4; // UPDATE_STYLE
    in.color = 2;     // BLUE
    in.overlay = 1;   // NOTCHED_6
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 48u);
    EXPECT_EQ(std::get<BossEvent>(out), in);
}

TEST_F(NetworkTestBase, PlayBossEventUpdateProperties)
{
    BossEvent in{};
    in.uuid = sampleUuid();
    in.operation = 5; // UPDATE_PROPERTIES
    in.flags = 0x02;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 48u);
    EXPECT_EQ(std::get<BossEvent>(out), in);
}

// ============================================================================
// 批 14：标题 S→C（SetTitleText/SetSubtitleText/SetActionBarText/SetTitlesAnimation/ClearTitles）
// 三文本包 text 为 opaque Component；SetTitlesAnimation 三 i32；ClearTitles 一 bool。
// ============================================================================

TEST_F(NetworkTestBase, PlaySetTitleText)
{
    SetTitleText in{};
    in.text = {0x01, 'H', 'i'};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 56u);
    EXPECT_EQ(std::get<SetTitleText>(out), in);
}

TEST_F(NetworkTestBase, PlaySetSubtitleText)
{
    SetSubtitleText in{};
    in.text = {0x02, 's', 'u', 'b'};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 57u);
    EXPECT_EQ(std::get<SetSubtitleText>(out), in);
}

TEST_F(NetworkTestBase, PlaySetActionBarText)
{
    SetActionBarText in{};
    in.text = {0x03, 'a', 'c', 't', 'i', 'o', 'n'};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 58u);
    EXPECT_EQ(std::get<SetActionBarText>(out), in);
}

TEST_F(NetworkTestBase, PlaySetTitlesAnimation)
{
    SetTitlesAnimation in{};
    in.fadeIn = 10;
    in.stay = 70;
    in.fadeOut = 20;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 59u);
    EXPECT_EQ(std::get<SetTitlesAnimation>(out), in);
}

TEST_F(NetworkTestBase, PlayClearTitlesResetTimes)
{
    ClearTitles in{};
    in.resetTimes = true;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 60u);
    EXPECT_EQ(std::get<ClearTitles>(out), in);
}

TEST_F(NetworkTestBase, PlayClearTitlesKeepTimes)
{
    ClearTitles in{};
    in.resetTimes = false;
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 60u);
    EXPECT_EQ(std::get<ClearTitles>(out), in);
}
