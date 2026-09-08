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

#pragma once

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacketBase.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::network::ir::play {

// ============================================================================
// 说明
//
// 本头补全 PlayPackets.hpp 在用包子集之外的游戏阶段包，对齐 Java 1.21.11 线协议。
// ParticleOptions、Explosion（含粒子表/声音）、LevelParticles、BlockEntityData（NBT）
// 等已结构化 codec（见 JavaPlayCodecsExtended.hpp）。仍以 opaque 字节透传（std::vector<u8>，
// codec 按 VarInt(len)+bytes 读写）的复杂嵌套树（命令树 Node、MapDecoration/MapPatch、
// Component 文本、NumberFormat 等）属我方互通自洽的 opaque 透传层：双端同 codec 读写，
// 我方互通必达；真 Java 互通需各自完整 codec，属独立子项，不影响我方互通。
// 复用 PlayPackets.hpp 的 CommonPlayerSpawnInfo（Respawn 内联）。
// ============================================================================

// ----------------------------------------------------------------------------
// 声音（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief Holder<SoundEvent>（1.21.11，对齐 ByteBufCodecs.holder）
 *
 * 线格式：VarInt(mode)——0=内联（DIRECT），>0=引用（registry holder id = mode-1）。
 * - 内联：Identifier(VarInt len + UTF-8) + Optional<Float>(bool present + f32 fixedRange)
 * - 引用：仅 VarInt id（本项目无 sound registry 整数 id 表，引用模式不可还原，
 *   故我方互通统一用内联模式编码；解码兼容两种模式但引用模式无资源可查）。
 *
 * 被 PlaySound/SoundEntity/Explosion 复用（定义前置供 PlaySound 引用为值成员）。
 */
struct SoundEventHolder {
    bool direct = true;     // true=内联，false=引用
    std::string identifier; // direct=true 时有效（如 "minecraft:entity.generic.explode"）
    bool hasFixedRange = false;
    f32 fixedRange = 0.0f;
    i32 referenceId = 0; // direct=false 时有效（Java holder id - 1）
    [[nodiscard]] friend bool operator==(const SoundEventHolder&, const SoundEventHolder&) noexcept = default;
};

/**
 * @brief PlaySound（S→C，id=115）
 *
 * Holder<SoundEvent> 结构化字段（内联/引用，对齐 vanilla wire），见 SoundEventHolder。
 * 坐标为 ×8 取整后的 int（Java writeInt）。
 */
struct PlaySound {
    SoundEventHolder soundHolder; // Holder<SoundEvent>
    i32 source;                   // SoundSource ordinal
    i32 x;                        // 实际坐标 ×8 取整
    i32 y;
    i32 z;
    f32 volume;
    f32 pitch;
    i64 seed;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlaySound&, const PlaySound&) noexcept = default;
};

/**
 * @brief StopSound（S→C，id=117）
 *
 * flags bit0=HAS_SOURCE，bit1=HAS_SOUND。source/name 仅在对应 flag 置位时有效。
 */
struct StopSound {
    u8 flags;
    i32 source;       // flags&1 时有效
    std::string name; // flags&2 时有效
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const StopSound&, const StopSound&) noexcept = default;
};

/**
 * @brief SoundEntity（S→C，id=114，实体发声）
 */
struct SoundEntity {
    SoundEventHolder soundHolder; // Holder<SoundEvent>
    i32 source;
    i32 entityId;
    f32 volume;
    f32 pitch;
    i64 seed;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SoundEntity&, const SoundEntity&) noexcept = default;
};

/**
 * @brief LevelEvent（S→C，id=45，世界级事件）
 *
 * 线格式：Int(type)+BlockPos(long)+Int(data)+Bool(global)。
 */
struct LevelEvent {
    i32 type;
    i64 blockPosPacked;
    i32 data;
    bool globalEvent;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const LevelEvent&, const LevelEvent&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 粒子（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief ParticleOptions（1.21.11 粒子选项，对齐 ParticleTypes.STREAM_CODEC）
 *
 * 线格式：VarInt(registryId) + 各类型专属 payload。registryId 取自
 * particle::toProtocolId(type)——本项目的 ParticleTypeId(0..114) 即 Java 注册顺序，
 * 内部扩展类型(115..123)经 toProtocolId 映射到对应协议类型（如 Breaking→Block）。
 *
 * 各类型 payload（对齐 net.minecraft.core.particles 各 *ParticleOption.STREAM_CODEC）：
 * - SimpleParticleType（无 options）：无额外字节
 * - BlockParticleOption（Block/BlockMarker/FallingDust/DustPillar/BlockCrumble）：VarInt(blockStateId)
 * - ItemParticleOption（Item/ItemSlime/ItemSnowball/ItemCobweb）：完整 ItemStack wire
 * - ColorParticleOption（EntityEffect/Flash/TintedLeaves）：INT color（ARGB 大端）
 * - DustParticleOptions（Dust/Redstone）：INT color（ARGB）+ FLOAT scale
 * - DustColorTransitionOptions：INT fromColor + INT toColor + FLOAT scale
 * - VibrationParticleOption：PositionSource + VAR_INT arrivalInTicks
 *     PositionSource = VarInt(kind: 0=Block 1=Entity)
 *       kind=0: i64 packedBlockPos（BlockPos.asLong）
 *       kind=1: VarInt entityId + FLOAT yOffset
 * - TrailParticleOption：Vec3(3×F64 target) + INT color（ARGB）+ VAR_INT duration
 *
 * 本结构为扁平 POD（与既有 IR 风格一致）：仅对应 type 的字段有效，codec 按 type 分支读写。
 */
struct ParticleOptions {
    particle::ParticleTypeId type = particle::ParticleTypeId::Invalid;

    // ColorParticleOption / Dust / DustColorTransition / Trail 共用 color 字段（ARGB）
    u32 color = 0;
    f32 scale = 1.0f;  // Dust / DustColorTransition
    u32 fromColor = 0; // DustColorTransition
    u32 toColor = 0;   // DustColorTransition

    u32 blockStateId = 0; // BlockParticleOption
    ItemStackView item;   // ItemParticleOption（完整 ItemStack wire）

    // VibrationParticleOption
    u8 vibrationSourceKind = 0;      // 0=Block, 1=Entity
    i64 vibrationBlockPosPacked = 0; // kind=0 时有效（BlockPos.asLong）
    i32 vibrationEntityId = 0;       // kind=1 时有效
    f32 vibrationYOffset = 0.0f;     // kind=1 时有效
    i32 arrivalInTicks = 0;

    // TrailParticleOption
    f64 trailTargetX = 0.0;
    f64 trailTargetY = 0.0;
    f64 trailTargetZ = 0.0;
    i32 trailDuration = 0; // color 字段重用为 trail 颜色
    [[nodiscard]] friend bool operator==(const ParticleOptions&, const ParticleOptions&) noexcept = default;
};

/**
 * @brief LevelParticles（S→C，id=46）
 *
 * 线格式对齐 ClientboundLevelParticlesPacket：overrideLimiter/alwaysShow(bool×2) +
 * x/y/z(double×3) + xDist/yDist/zDist(float×3) + maxSpeed(float) + count(int) + particle(ParticleOptions)。
 */
struct LevelParticles {
    bool overrideLimiter;
    bool alwaysShow;
    f64 x;
    f64 y;
    f64 z;
    f32 xDist;
    f32 yDist;
    f32 zDist;
    f32 maxSpeed;
    i32 count;
    ParticleOptions particle;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const LevelParticles&, const LevelParticles&) noexcept = default;
};

// ----------------------------------------------------------------------------
// Boss 条（S→C，id=9，单包 + operation 分发）
// ----------------------------------------------------------------------------

/**
 * @brief BossEvent（S→C，id=9）
 *
 * operation：0=ADD 1=REMOVE 2=UPDATE_PROGRESS 3=UPDATE_NAME 4=UPDATE_STYLE
 *            5=UPDATE_PROPERTIES。各 operation 子字段见 Java，ADD 需 name/progress/
 *            color/overlay/flags，REMOVE 无，UPDATE_PROGRESS 需 progress，依此类推。
 * name 为 Component，opaque 透传。
 */
struct BossEvent {
    std::array<u8, 16> uuid;
    u8 operation;
    std::vector<u8> name; // opaque：Component（ADD/UPDATE_NAME）
    f32 progress;         // ADD/UPDATE_PROGRESS
    i32 color;            // BossBarColor ordinal（ADD/UPDATE_STYLE）
    i32 overlay;          // BossBarOverlay ordinal（ADD/UPDATE_STYLE）
    u8 flags;             // ADD/UPDATE_PROPERTIES：bit0=DARKEN bit1=MUSIC bit2=FOG
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BossEvent&, const BossEvent&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 进度（Advancements）
// ----------------------------------------------------------------------------

/**
 * @brief SelectAdvancementTab（S→C，id=83）
 *
 * Nullable<Identifier>：present=false 表关闭标签页。
 */
struct SelectAdvancementTab {
    bool present;
    std::string tab; // present=true 时有效
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SelectAdvancementTab&, const SelectAdvancementTab&) noexcept = default;
};

/**
 * @brief SeenAdvancements（C→S，id=49）
 *
 * action：0=OPENED_TAB 1=CLOSED_SCREEN。OPENED_TAB 时 tab 有效。
 */
struct SeenAdvancements {
    i32 action;
    std::string tab; // action=0 时有效
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SeenAdvancements&, const SeenAdvancements&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 记分板（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief SetObjective（S→C，id=104，单包 + method 分发）
 *
 * method：0=ADD 1=REMOVE 2=CHANGE。ADD/CHANGE 需 displayName/renderType/numberFormat。
 * displayName 为 Component，numberFormat 为 opaque（registry id + 子格式）。
 */
struct SetObjective {
    std::string objectiveName;
    u8 method;
    std::vector<u8> displayName;  // opaque：Component（method 0/2）
    i32 renderType;               // RenderType ordinal（method 0/2）
    std::vector<u8> numberFormat; // opaque：Optional<NumberFormat>（method 0/2）
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetObjective&, const SetObjective&) noexcept = default;
};

/**
 * @brief SetScore（S→C，id=108）
 *
 * display/numberFormat 为 opaque Optional。
 */
struct SetScore {
    std::string owner;
    std::string objectiveName;
    i32 score;
    std::vector<u8> display;      // opaque：Optional<Component>
    std::vector<u8> numberFormat; // opaque：Optional<NumberFormat>
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetScore&, const SetScore&) noexcept = default;
};

/**
 * @brief ResetScore（S→C，id=77）
 *
 * objectiveName 可空（nullopt 表重置该 owner 所有 objective）。
 */
struct ResetScore {
    std::string owner;
    std::optional<std::string> objectiveName;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ResetScore&, const ResetScore&) noexcept = default;
};

/**
 * @brief SetDisplayObjective（S→C，id=96）
 *
 * slot 为 DisplaySlot 自定义 id（非 ordinal）。objectiveName 空串表清除该 slot。
 */
struct SetDisplayObjective {
    i32 slot;
    std::string objectiveName;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetDisplayObjective&, const SetDisplayObjective&) noexcept = default;
};

/**
 * @brief SetPlayerTeam（S→C，id=107，单包 + method 分发）
 *
 * method：0=ADD 1=REMOVE 2=CHANGE 3=JOIN 4=LEAVE。
 * ADD/CHANGE 带 TeamParameters（displayName/options/visibility/collision/color/prefix/suffix），
 * ADD/JOIN/LEAVE 带 players 列表。Parameters 内 displayName/prefix/suffix 为 Component NBT wire 字节，
 * visibility/collision 为 Team.Visibility/CollisionRule 的 VarInt id（0-3），color 为 ChatFormatting
 * ordinal VarInt，options 为 friendlyFlags 打包 Byte。对齐 vanilla ClientboundSetPlayerTeamPacket.Parameters。
 */
struct SetPlayerTeam {
    std::string name;
    u8 method;
    // TeamParameters（仅 method 0/2 有效，对齐 vanilla 扁平 7 字段顺序）
    std::vector<u8> displayName; // Component NBT wire 字节
    u8 options =
        0; // friendlyFlags 打包（bit0 friendlyFire/bit1 seeInvisibles/bit2 nametagVisibility/bit3 collisionRule）
    i32 visibility = 0;     // Team.Visibility id（VarInt，0=Always/1=Never/2=HideForOtherTeams/3=HideForOwnTeam）
    i32 collision = 0;      // Team.CollisionRule id（VarInt，0=Always/1=Never/2=PushOtherTeams/3=PushOwnTeam）
    i32 color = 0;          // ChatFormatting ordinal（VarInt）
    std::vector<u8> prefix; // Component NBT wire 字节
    std::vector<u8> suffix; // Component NBT wire 字节
    std::vector<std::string> players; // method 0/3/4
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetPlayerTeam&, const SetPlayerTeam&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 标题（S→C，1.21.11 拆 5 包）
// ----------------------------------------------------------------------------

/**
 * @brief SetTitleText（S→C，id=112）
 */
struct SetTitleText {
    std::vector<u8> text; // opaque：Component
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetTitleText&, const SetTitleText&) noexcept = default;
};

/**
 * @brief SetSubtitleText（S→C，id=110）
 */
struct SetSubtitleText {
    std::vector<u8> text; // opaque：Component
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetSubtitleText&, const SetSubtitleText&) noexcept = default;
};

/**
 * @brief SetActionBarText（S→C，id=85）
 */
struct SetActionBarText {
    std::vector<u8> text; // opaque：Component
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetActionBarText&, const SetActionBarText&) noexcept = default;
};

/**
 * @brief SystemChat（S→C，id=119）
 *
 * 1.21.11 系统聊天/动作栏消息。content 为 Component NBT wire 字节（自定界，无外层 VarInt 长度，
 * 对齐 vanilla ClientboundSystemChatPacket.content = ComponentSerialization.TRUSTED_STREAM_CODEC）。
 * overlay=false 显示在聊天窗口，overlay=true 显示在动作栏。
 */
struct SystemChat {
    std::vector<u8> content; // Component NBT wire 字节
    bool overlay = false;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SystemChat&, const SystemChat&) noexcept = default;
};

/**
 * @brief SetTitlesAnimation（S→C，id=113）
 */
struct SetTitlesAnimation {
    i32 fadeIn;
    i32 stay;
    i32 fadeOut;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetTitlesAnimation&, const SetTitlesAnimation&) noexcept = default;
};

/**
 * @brief ClearTitles（S→C，id=14）
 */
struct ClearTitles {
    bool resetTimes;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ClearTitles&, const ClearTitles&) noexcept = default;
};

// ============================================================================
// 玩家战斗（S→C，1.21.11 共 3 包）
//
// 对应 Java 1.21.11 ClientboundPlayerCombatEnterPacket / EndPacket / KillPacket。
// 驱动客户端战斗状态机与死亡画面（DeathScreen）。三包均为 S→C，仅发给当事玩家。
// 变体顺序即 altIndex：PlayerCombatEnter=111 / PlayerCombatEnd=112 / PlayerCombatKill=113。
// ============================================================================

/**
 * @brief PlayerCombatEnter（S→C，id=65）
 *
 * 对应 Java 1.21.11 ClientboundPlayerCombatEnterPacket：无负载。
 * vanilla 为 INSTANCE 单例，STREAM_CODEC = StreamCodec.unit(INSTANCE)。
 * 客户端收到后进入战斗状态（开启战斗计时器等）。
 */
struct PlayerCombatEnter {
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerCombatEnter&, const PlayerCombatEnter&) noexcept = default;
};

/**
 * @brief PlayerCombatEnd（S→C，id=64）
 *
 * 对应 Java 1.21.11 ClientboundPlayerCombatEndPacket：VarInt(duration)。
 * duration 来自 CombatTracker.getCombatDuration()。客户端收到后结束战斗状态。
 */
struct PlayerCombatEnd {
    i32 duration; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerCombatEnd&, const PlayerCombatEnd&) noexcept = default;
};

/**
 * @brief PlayerCombatKill（S→C，id=66）
 *
 * 对应 Java 1.21.11 ClientboundPlayerCombatKillPacket：VarInt(playerId) + Component(message)。
 * Component NBT wire 字节（自定界，无外层 VarInt 长度，
 * 对齐 vanilla ComponentSerialization.TRUSTED_STREAM_CODEC）。
 * 客户端收到后显示死亡画面（DeathScreen）。
 * isSkippable() 返回 true（原版允许跳过该包）。
 */
struct PlayerCombatKill {
    i32 playerId;            // VarInt
    std::vector<u8> message; // Component NBT wire 字节
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerCombatKill&, const PlayerCombatKill&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 世界边界（S→C，1.21.11 拆 6 包）
// ----------------------------------------------------------------------------

/**
 * @brief InitializeBorder（S→C，id=42）
 */
struct InitializeBorder {
    f64 newCenterX;
    f64 newCenterZ;
    f64 oldSize;
    f64 newSize;
    i64 lerpTime; // VarLong
    i32 newAbsoluteMaxSize;
    i32 warningBlocks;
    i32 warningTime;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const InitializeBorder&, const InitializeBorder&) noexcept = default;
};

/**
 * @brief SetBorderCenter（S→C，id=86）
 */
struct SetBorderCenter {
    f64 newCenterX;
    f64 newCenterZ;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetBorderCenter&, const SetBorderCenter&) noexcept = default;
};

/**
 * @brief SetBorderLerpSize（S→C，id=87）
 */
struct SetBorderLerpSize {
    f64 oldSize;
    f64 newSize;
    i64 lerpTime; // VarLong
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetBorderLerpSize&, const SetBorderLerpSize&) noexcept = default;
};

/**
 * @brief SetBorderSize（S→C，id=88）
 */
struct SetBorderSize {
    f64 size;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetBorderSize&, const SetBorderSize&) noexcept = default;
};

/**
 * @brief SetBorderWarningDelay（S→C，id=89）
 */
struct SetBorderWarningDelay {
    i32 warningDelay; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetBorderWarningDelay&, const SetBorderWarningDelay&) noexcept = default;
};

/**
 * @brief SetBorderWarningDistance（S→C，id=90）
 */
struct SetBorderWarningDistance {
    i32 warningBlocks; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const SetBorderWarningDistance&, const SetBorderWarningDistance&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 地图（S→C，结构化，对齐 1.21.11 ClientboundMapItemDataPacket）
// ----------------------------------------------------------------------------

/**
 * @brief 地图装饰 wire 表示（对应 Java MapDecoration）
 *
 * type 字段是 MAP_DECORATION_TYPE 注册表的 Holder：vanilla MapDecorationType.STREAM_CODEC
 * = ByteBufCodecs.holderRegistry，wire = 纯 VarInt(registryId)，无 mode 前缀、无内联 NBT 分支。
 * registryId 即 DecorationType 枚举值（PLAYER=0 … TRIAL_CHAMBERS=34，与 Java 注册顺序一致）。
 * name 是 Optional<Component>，Component 暂以 JSON 字节 opaque 透传（nullopt=无名称）。
 */
struct MapDecorationWire {
    u32 typeRegistryId = 0; // VarInt(registryId)；PLAYER→0，默认 0
    i8 x = 0;
    i8 y = 0;
    u8 rotation = 0;                     // &0x0F
    std::optional<std::vector<u8>> name; // opaque Component（JSON 字节）

    [[nodiscard]] friend bool operator==(const MapDecorationWire&, const MapDecorationWire&) noexcept = default;
};

/**
 * @brief 地图色块 wire 表示（对应 Java MapItemSavedData.MapPatch）
 *
 * 1.21.11 wire 顺序：width, height, startX, startY, ByteArray(colors)。
 * absent 用 width==0 哨兵表示（colorPatch 整体为 nullopt 时 codec 写 width=0）。
 * colors 为 width*height 字节，行优先索引 colors[i + j*width] ↔ mapData[startX+i + (startY+j)*128]。
 */
struct MapPatchWire {
    u8 startX = 0;
    u8 startY = 0;
    u8 width = 0;
    u8 height = 0;
    std::vector<u8> colors; // width*height 字节

    [[nodiscard]] friend bool operator==(const MapPatchWire&, const MapPatchWire&) noexcept = default;
};

/**
 * @brief MapItemData（S→C，id=49）
 *
 * 对齐 Java 1.21.11 ClientboundMapItemDataPacket STREAM_CODEC（composite）：
 * VarInt(mapId) + byte(scale) + bool(locked) +
 * Optional<List<MapDecoration>>（bool present + VarInt count + 项）+
 * Optional<MapPatch>（width==0 哨兵表 absent）。
 */
struct MapItemData {
    i32 mapId = 0; // VarInt
    u8 scale = 0;
    bool locked = false;
    std::optional<std::vector<MapDecorationWire>> decorations;
    std::optional<MapPatchWire> colorPatch;

    [[nodiscard]] friend bool operator==(const MapItemData&, const MapItemData&) noexcept = default;
    BedrockMeta bedrock{};
};

// ----------------------------------------------------------------------------
// 告示牌
// ----------------------------------------------------------------------------

/**
 * @brief OpenSignEditor（S→C，id=58）
 */
struct OpenSignEditor {
    i64 blockPosPacked; // BlockPos.asLong
    bool isFrontText;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const OpenSignEditor&, const OpenSignEditor&) noexcept = default;
};

/**
 * @brief SignUpdate（C→S，id=59，玩家提交告示牌文本）
 *
 * 服务端→客户端的告示牌内容变更走 BlockEntityData（携 sign NBT）。
 */
struct SignUpdate {
    i64 blockPosPacked;
    bool isFrontText;
    std::array<std::string, 4> lines; // 每行最长 384 字符
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SignUpdate&, const SignUpdate&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 简单单包（S→C，除注明外）
// ----------------------------------------------------------------------------

/**
 * @brief SetCamera（S→C，id=91）
 */
struct SetCamera {
    i32 cameraId; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetCamera&, const SetCamera&) noexcept = default;
};

/**
 * @brief SetEntityLink（S→C，id=98，骑乘关系）
 *
 * 线格式：Int(sourceId)+Int(destId)。destId=0 表解除。
 */
struct SetEntityLink {
    i32 sourceId;
    i32 destId;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetEntityLink&, const SetEntityLink&) noexcept = default;
};

/**
 * @brief SetPassengers（S→C，id=105）
 */
struct SetPassengers {
    i32 vehicle;                 // VarInt
    std::vector<i32> passengers; // VarInt 列表
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetPassengers&, const SetPassengers&) noexcept = default;
};

/**
 * @brief EntityEvent（S→C，id=34）
 *
 * 线格式：Int(entityId)+Byte(eventId)。
 */
struct EntityEvent {
    i32 entityId;
    u8 eventId;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const EntityEvent&, const EntityEvent&) noexcept = default;
};

/**
 * @brief Animate（S→C，id=2）
 *
 * action：0=SWING_MAIN_HAND 2=WAKE_UP 3=SWING_OFF_HAND 4=CRITICAL_HIT
 *         5=MAGIC_CRITICAL_HIT。
 */
struct Animate {
    i32 id; // VarInt
    u8 action;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Animate&, const Animate&) noexcept = default;
};

/**
 * @brief HurtAnimation（S→C，id=41）
 */
struct HurtAnimation {
    i32 id; // VarInt
    f32 yaw;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const HurtAnimation&, const HurtAnimation&) noexcept = default;
};

/**
 * @brief TakeItemEntity（S→C，id=122，拾取动画）
 */
struct TakeItemEntity {
    i32 itemId;   // VarInt
    i32 playerId; // VarInt
    i32 amount;   // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const TakeItemEntity&, const TakeItemEntity&) noexcept = default;
};

/**
 * @brief BlockDestruction（S→C，id=5）
 */
struct BlockDestruction {
    i32 id; // VarInt（破坏者实体 id）
    i64 blockPosPacked;
    u8 progress;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BlockDestruction&, const BlockDestruction&) noexcept = default;
};

/**
 * @brief BlockEvent（S→C，id=7）
 *
 * 线格式：BlockPos(long)+Byte(b0)+Byte(b1)+Holder<Block>(VarInt id)。
 */
struct BlockEvent {
    i64 blockPosPacked;
    u8 b0;
    u8 b1;
    i32 blockId; // VarInt block registry id
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BlockEvent&, const BlockEvent&) noexcept = default;
};

/**
 * @brief BlockEntityData（S→C，id=6）
 *
 * 线格式：BlockPos(long)+Holder<BlockEntityType>(VarInt id)+CompoundTag（无长度前缀，
 * NBT 自定界，读取器通过解析复合标签的 TAG_End 判定结束，对齐 1.21.11
 * ClientboundBlockEntityDataPacket.STREAM_CODEC）。
 *
 * tag 用 shared_ptr 承载：compound_tag 持有 map<unique_ptr>，仅复制构造（无移动），
 * 直接按值存入 variant 会破坏 variant 的移动语义；shared_ptr 在 IR 零拷贝 Local 直传与
 * 变体迁移之间均为廉价指针拷贝。空 NBT 用 nullptr 表示。
 */
struct BlockEntityData {
    i64 blockPosPacked;
    i32 blockEntityType;                   // VarInt registry id
    std::shared_ptr<nbt::CompoundTag> tag; // Java 大端二进制 CompoundTag
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BlockEntityData&, const BlockEntityData&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 维度（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief Respawn（S→C，id=80）
 *
 * 复用 CommonPlayerSpawnInfo + u8 dataToKeep 位掩码
 *（1=KEEP_ATTRIBUTE_MODIFIERS 2=KEEP_ENTITY_DATA 3=KEEP_ALL_DATA）。
 */
struct Respawn {
    CommonPlayerSpawnInfo spawnInfo;
    u8 dataToKeep;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Respawn&, const Respawn&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 经验（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief SetExperience（S→C，id=101）
 */
struct SetExperience {
    f32 experienceProgress;
    i32 experienceLevel; // VarInt
    i32 totalExperience; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetExperience&, const SetExperience&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 爆炸（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief ExplosionParticleInfo（1.21.11 爆炸粒子表条目）
 *
 * 线格式：ParticleOptions + FLOAT scaling + FLOAT speed。
 */
struct ExplosionParticleInfo {
    ParticleOptions particle;
    f32 scaling = 1.0f;
    f32 speed = 1.0f;
    [[nodiscard]] friend bool operator==(const ExplosionParticleInfo&, const ExplosionParticleInfo&) noexcept = default;
};

/**
 * @brief Explosion（S→C，id=36，1.21.11 结构化）
 *
 * 对齐 ClientboundExplodePacket.STREAM_CODEC：
 * Vec3 center + FLOAT radius + INT blockCount + Optional<Vec3> playerKnockback +
 * ParticleOptions explosionParticle + Holder<SoundEvent> explosionSound +
 * WeightedList<ExplosionParticleInfo> blockParticles。
 *
 * 1.21.11 已无 affectedBlocks 列表（改为 blockCount:int + blockParticles 粒子表）；
 * 客户端击退由 playerKnockback(Optional<Vec3>) 承载。
 */
struct Explosion {
    f64 centerX;
    f64 centerY;
    f64 centerZ;
    f32 radius;
    i32 blockCount;
    bool hasPlayerKnockback; // Optional<Vec3>
    f64 knockbackX;
    f64 knockbackY;
    f64 knockbackZ;
    ParticleOptions explosionParticle;
    SoundEventHolder explosionSound;
    std::vector<ExplosionParticleInfo> blockParticles; // WeightedList（权重在 codec 侧读写，IR 不承载）
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Explosion&, const Explosion&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 载具 / 交互
// ----------------------------------------------------------------------------

/**
 * @brief ServerboundMoveVehicle（C→S，id=33）
 */
struct ServerboundMoveVehicle {
    f64 x;
    f64 y;
    f64 z;
    f32 yRot;
    f32 xRot;
    bool onGround;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const ServerboundMoveVehicle&, const ServerboundMoveVehicle&) noexcept = default;
};

/**
 * @brief ClientboundMoveVehicle（S→C，id=55）
 */
struct ClientboundMoveVehicle {
    f64 x;
    f64 y;
    f64 z;
    f32 yRot;
    f32 xRot;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const ClientboundMoveVehicle&, const ClientboundMoveVehicle&) noexcept = default;
};

/**
 * @brief PaddleBoat（C→S，id=34）
 */
struct PaddleBoat {
    bool left;
    bool right;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PaddleBoat&, const PaddleBoat&) noexcept = default;
};

/**
 * @brief Interact（C→S，id=25，分发 action）
 *
 * action：0=INTERACT 1=ATTACK 2=INTERACT_AT。
 * INTERACT/INTERACT_AT 带 hand（0=MAIN_HAND 1=OFF_HAND）；INTERACT_AT 额外带命中点。
 */
struct Interact {
    i32 entityId; // VarInt
    i32 action;
    i32 hand; // action 0/2
    f32 hitX; // action 2
    f32 hitY;
    f32 hitZ;
    bool usingSecondaryAction;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Interact&, const Interact&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 命令树（S→C，opaque）
// ----------------------------------------------------------------------------

/**
 * @brief Commands（S→C，id=16，opaque）
 *
 * List<Node> + VarInt rootIndex。Node 树（flags/children/redirect/按 type 分发的
 * Literal/Argument stub）由服务端 sendCommandTreePacket 经
 * mc::network::java::codecs::encodeCommandTree(snapshot) 编码为 1.21.11 二进制
 * CommandNode 树字节后填入 payload 透传（见 CommandTreeEncoder.hpp）。IR 层保持
 * opaque 是项目既有模式（复杂嵌套结构独立子项）；codec 层仅做 opaque 透传。
 */
struct Commands {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Commands&, const Commands&) noexcept = default;
};

/**
 * @brief PlaceRecipe（C→S，id=38）
 */
struct PlaceRecipe {
    i32 containerId; // VarInt
    i32 recipe;      // VarInt RecipeDisplayId
    bool useMaxItems;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlaceRecipe&, const PlaceRecipe&) noexcept = default;
};

// ============================================================================
// 服务端→客户端：区块与方块
// ============================================================================

/**
 * @brief PalettedContainer 的 vanilla 线语义中间表示（states 4096 entry / biomes 64 entry 通用）
 *
 * 对齐 1.21.11 PalettedContainer.write：u8 bits + palette + long[] storage（无长度前缀）。
 * bits 字段即 BitStorage.getBits()，按 vanilla Strategy.getConfigurationForBitCount 阈值：
 *   blocks(4096)：size=1→bits=0(SingleValue)；size 2-16→bits=4(Linear)；17-32→5；33-64→6；
 *                 65-128→7；129-256→8(HashMap)；≥257→Global bits=ceillog2(blockRegistrySize)=15。
 *   biomes(64)：size=1→0；2→1；3-4→2；5-8→3(Linear)；≥9→Global bits=ceillog2(66)=7。
 *   Global 时 paletteGlobalIds 空，storage 直接存全局 id；SingleValue 时 storage 空。
 * storage 是 LSB-first long 数组，longCount=ceil(entryCount/floor(64/bits))（非 ceil(entryCount*bits/64)）。
 * codec 写时逐元素大端 writeI64，无长度前缀（长度由 bits+entryCount 隐含）。
 */
struct PalettedContainerWire {
    u8 bits = 0;
    std::vector<u32> paletteGlobalIds; // 全局 id（block state globalId 或 biome registry id）
    std::vector<u64> storage;          // LSB-first long[]，无长度前缀
    [[nodiscard]] friend bool operator==(const PalettedContainerWire&, const PalettedContainerWire&) noexcept = default;
};

/**
 * @brief 单个区块段（16³）的 vanilla 线语义中间表示
 *
 * 对齐 1.21.11 LevelChunkSection.write：i16 nonEmptyBlockCount + states + biomes。
 * 不含 ticking/tickingFluidCount（1.21.11 已移除）。
 */
struct ChunkSectionWire {
    i16 nonEmptyBlockCount = 0;
    PalettedContainerWire states; // 4096 entry
    PalettedContainerWire biomes; // 64 entry
    [[nodiscard]] friend bool operator==(const ChunkSectionWire&, const ChunkSectionWire&) noexcept = default;
};

/**
 * @brief 高度图条目（vanilla HEIGHTMAPS_STREAM_CODEC 的一项）
 *
 * 对齐 1.21.11：map(Heightmap.Types idMapper→VarInt(id), LONG_ARRAY)。
 * typeId 是 Heightmap.Types 的 id 字段（codec 写为 VarInt，非 u8）：
 *   1=WORLD_SURFACE, 4=MOTION_BLOCKING, 5=MOTION_BLOCKING_NO_LEAVES。
 *   客户端只发 sendToClient(usage==CLIENT) 的这 3 个；OCEAN_FLOOR(id=3) 是 LIVE_WORLD 不发。
 * data 为 9-bit packed long[]（主世界 chunkHeight=384 → bits=ceillog2(385)=9，
 *   longCount=ceil(256/floor(64/9))=ceil(256/7)=37 long），元素值=(最高方块Y+1)-chunkMinY（空列=0），
 *   LSB-first。codec 写时带 VarInt(longCount) 前缀。
 */
struct HeightmapEntryWire {
    u8 typeId = 0;
    std::vector<u64> data; // 37 long（主世界）
    [[nodiscard]] friend bool operator==(const HeightmapEntryWire&, const HeightmapEntryWire&) noexcept = default;
};

/**
 * @brief 方块实体条目（vanilla BlockEntityInfo 列表的一项）
 *
 * 对齐 1.21.11 ClientboundLevelChunkPacketData.BlockEntityInfo.write：
 *   u8 packedXZ((relX<<4)|relZ) + i16 y(绝对) + VarInt(typeRegistryId) + NBT。
 * tag 用 shared_ptr 承载（compound_tag 仅复制构造，按值存入 variant 破坏移动语义；
 * 参考 BlockEntityData）。空 NBT 用 nullptr（codec 写为 EndTag 单字节 0x00）。
 */
struct BlockEntityInfoWire {
    u8 packedXZ = 0;
    i16 y = 0;
    u32 typeRegistryId = 0; // block entity type registry id
    std::shared_ptr<nbt::CompoundTag> tag;
    [[nodiscard]] friend bool operator==(const BlockEntityInfoWire&, const BlockEntityInfoWire&) noexcept = default;
};

/**
 * @brief LevelChunkWithLight（S→C，id=44，区块数据 + 光照）
 *
 * 线格式（1.21.11 ClientboundLevelChunkWithLightPacket）：
 *   i32 x + i32 z + [heightmaps + VarInt(bufLen)+sectionBuf + blockEntities] + [光照 masks/updates]。
 *   无外层长度前缀。chunkData 部分与 lightData 部分各自结构化（见各子结构注释）。
 *
 * 贯彻 IR 思想：本结构持 vanilla 语义字段，服务端从 ChunkData 直接填充、客户端直接消费，
 * 均不碰 wire 字节；只有 levelChunkWithLightCodec（仅远程 Java 客户端经 JavaBackend 走）
 * 把本结构编码成 vanilla wire。本地客户端经 LocalTransport 零拷贝直传本结构。
 *
 * 光照部分（lightData）字段语义与 LightUpdate（id=47）一致：
 *   lightMasks[0..3] = skyYMask / blockYMask / emptySkyYMask / emptyBlockYMask（最小 long 数组）；
 *   lightUpdates[0..1] = skyUpdates / blockUpdates（每条 2048 字节 nibble）。
 *   yMask popcount 必须 == 对应 updates.size()。光段数主世界 26，bit i ↔ lightSectionY = -5+i。
 */
struct LevelChunkWithLight {
    i32 x;
    i32 z;
    std::vector<HeightmapEntryWire>
        heightmaps;                         // 3 个 CLIENT 类型(WORLD_SURFACE/MOTION_BLOCKING/MOTION_BLOCKING_NO_LEAVES)
    std::vector<ChunkSectionWire> sections; // 主世界 24 段
    std::vector<BlockEntityInfoWire> blockEntities;           // 可空
    std::array<std::vector<u64>, 4> lightMasks;               // sky/block/emptySky/emptyBlock
    std::array<std::vector<std::vector<u8>>, 2> lightUpdates; // sky/block，每条 2048 字节
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const LevelChunkWithLight&, const LevelChunkWithLight&) noexcept = default;
};

// ----------------------------------------------------------------------------
// 简单状态同步单包（S→C / C→S，1.21.11 纯字段包）
// ----------------------------------------------------------------------------

/**
 * @brief SetChunkCacheRadius（S→C，id=93）
 *
 * 对齐 Java 1.21.11 ClientboundSetChunkCacheRadiusPacket：VarInt(radius)。
 * 客户端 ClientWorld::setRenderDistance。
 */
struct SetChunkCacheRadius {
    i32 radius; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetChunkCacheRadius&, const SetChunkCacheRadius&) noexcept = default;
};

/**
 * @brief SetSimulationDistance（S→C，id=109）
 *
 * 对齐 Java 1.21.11 ClientboundSetSimulationDistancePacket：VarInt(simulationDistance)。
 * 客户端 ClientWorld.setSimulationDistance 仅存字段，对齐 Java ClientLevel.setServerSimulationDistance
 * （vanilla 客户端该字段唯一消费点为 shouldTickDeath 死亡实体距离裁剪）。
 */
struct SetSimulationDistance {
    i32 simulationDistance; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetSimulationDistance&, const SetSimulationDistance&) noexcept = default;
};

/**
 * @brief SetHealth（S→C，id=102）
 *
 * 对齐 Java 1.21.11 ClientboundSetHealthPacket：Float(health)+VarInt(food)+Float(saturation)。
 * saturation 是 float（非 VarInt），apply 时直接 setSaturationLevel 不除 2。
 */
struct SetHealth {
    f32 health;
    i32 food; // VarInt
    f32 saturation;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetHealth&, const SetHealth&) noexcept = default;
};

/**
 * @brief ClientboundPing（S→C，id=59，common 通道）
 *
 * 对齐 Java 1.21.11 ClientboundPingPacket：Int(id)。客户端回 ServerboundPong(sb:44) 同 id。
 * 注意：与 ping 协议通道的 PongResponse(cb:60) 字段类型不同（后者为 long time）。
 */
struct ClientboundPing {
    i32 id; // Int
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ClientboundPing&, const ClientboundPing&) noexcept = default;
};

/**
 * @brief PongResponse（S→C，id=60，ping 协议通道）
 *
 * 对齐 Java 1.21.11 ClientboundPongResponsePacket：Long(time)。客户端主动 ping(sb:37) 的往返收尾。
 */
struct PongResponse {
    i64 time; // Long
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PongResponse&, const PongResponse&) noexcept = default;
};

/**
 * @brief ServerboundPingRequest（C→S，id=37，ping 协议通道）
 *
 * 对齐 Java 1.21.11 ServerboundPingRequestPacket：Long(time)。服务端回 PongResponse(cb:60) 同 time。
 */
struct ServerboundPingRequest {
    i64 time; // Long
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const ServerboundPingRequest&, const ServerboundPingRequest&) noexcept = default;
};

/**
 * @brief ServerboundPong（C→S，id=44，common 通道）
 *
 * 对齐 Java 1.21.11 ServerboundPongPacket：Int(id)。服务端 cb:59 ping 的响应。
 */
struct ServerboundPong {
    i32 id; // Int
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ServerboundPong&, const ServerboundPong&) noexcept = default;
};

/**
 * @brief ServerboundChangeDifficulty（C→S，id=3）
 *
 * 对齐 Java 1.21.11 ServerboundChangeDifficultyPacket：VarInt(difficulty)。
 * Difficulty.STREAM_CODEC = ByteBufCodecs.idMapper 即 VarInt(0..3)，非 Byte。
 * 客户端在选项菜单切换难度时发送；服务端校验 主机 或 OP(>=GameMaster) 方可应用。
 */
struct ServerboundChangeDifficulty {
    i32 difficulty; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const ServerboundChangeDifficulty&, const ServerboundChangeDifficulty&) noexcept = default;
};

/**
 * @brief LockDifficulty（C→S，id=28）
 *
 * 对应 Java 1.21.11 ServerboundLockDifficultyPacket：Bool(locked)。
 */
struct LockDifficulty {
    bool locked; // Bool
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const LockDifficulty&, const LockDifficulty&) noexcept = default;
};

// ============================================================================
// 区块相关数据包（altIndex 100..107）
//
// 下列 8 个包在此前仅有协议映射登记（assets/data/packet_ids.json）与 Prismarine
// 协议表条目，IR/codec/协议表登记/消费分支四层全缺，本次补全。消费端仅
// ForgetLevelChunk 落业务（复用 ClientWorld::onChunkUnload），其余为 TODO 桩。
// ============================================================================

/**
 * @brief BundleDelimiter（S→C，id=0）
 *
 * 对应 Java 1.21.11 ClientboundBundleDelimiterPacket：无负载。标记一个 bundle 的开始，
 * 客户端将后续包缓存在 bundle 窗口内直至收到下一个 delimiter 提交。bundle 状态机未实现。
 */
struct BundleDelimiter {
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BundleDelimiter&, const BundleDelimiter&) noexcept = default;
};

/**
 * @brief BlockChangedAck（S→C，id=4）
 *
 * 对应 Java 1.21.11 ClientboundBlockChangedAckPacket：VarInt(sequence)。服务端确认客户端
 * 方块挖掘预测的某个 sequence，客户端据此推进本地预测状态机。客户端挖掘预测链路未实现。
 */
struct BlockChangedAck {
    i32 sequence; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BlockChangedAck&, const BlockChangedAck&) noexcept = default;
};

/**
 * @brief ChunkBatchFinished（S→C，id=11）
 *
 * 对应 Java 1.21.11 ClientboundChunkBatchFinishedPacket：VarInt(batchSize)。标记一个区块
 * 批次发送结束，batchSize 为该批次内区块数。客户端据此计算实际接收速率并回发
 * ChunkBatchReceived(sb:10) 反馈期望速率。客户端批次流速控状态机未实现。
 */
struct ChunkBatchFinished {
    i32 batchSize; // VarInt
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ChunkBatchFinished&, const ChunkBatchFinished&) noexcept = default;
};

/**
 * @brief ChunkBatchStart（S→C，id=12）
 *
 * 对应 Java 1.21.11 ClientboundChunkBatchStartPacket：无负载。标记一个区块批次发送开始。
 * 客户端批次流速控状态机未实现。
 */
struct ChunkBatchStart {
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ChunkBatchStart&, const ChunkBatchStart&) noexcept = default;
};

/**
 * @brief ChunkBiomes 单项：区块坐标 packed + 生物群系数据
 *
 * packedChunkPos 为固定 Long（FriendlyByteBuf.writeChunkPos → writeLong(ChunkPos.toLong)，
 * 非 VarLong），位布局：X 低 32 位 @ bit0，Z 低 32 位 @ bit32。data 为 ByteArray，承载该区块
 * 4×4×4 采样生物群系的 PalettedContainer wire。
 *
 * 注意：项目 math::chunkPosToId 位布局（X@bit32 / Z@bit0）与 vanilla ChunkPos.asLong 相反，
 * 不可复用；codec 透传固定 I64，不在 codec 层解包。
 */
struct ChunkBiomeEntry {
    i64 packedChunkPos;   // Long（ChunkPos.asLong：X@bit0, Z@bit32）
    std::vector<u8> data; // ByteArray
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ChunkBiomeEntry&, const ChunkBiomeEntry&) noexcept = default;
};

/**
 * @brief ChunkBiomes（S→C，id=13）
 *
 * 对应 Java 1.21.11 ClientboundChunkBiomesPacket：VarInt(count) + count×{Long(packedChunkPos)
 * + ByteArray(data)}。服务端在某区块生物群系变更时批量下发受影响区块的新生物群系数据。
 * 客户端无生物群系增量更新入口（ClientWorld 仅在区块整体接收时随 ChunkData 落地生物群系），
 * 消费端为 TODO 桩。
 */
struct ChunkBiomes {
    std::vector<ChunkBiomeEntry> biomes; // count×{packedChunkPos + ByteArray}
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ChunkBiomes&, const ChunkBiomes&) noexcept = default;
};

/**
 * @brief ForgetLevelChunk（S→C，id=37）
 *
 * 对应 Java 1.21.11 ClientboundForgetLevelChunkPacket：单个 ChunkPos，经
 * FriendlyByteBuf.writeChunkPos → writeLong(ChunkPos.toLong) 编码为固定 8 字节大端 Long
 * （非 VarLong，亦非两个 VarInt）。ChunkPos.asLong 位布局：X 低 32 位 @ bit0，Z 低 32 位 @ bit32。
 * IR 以原始 packedPos 直存，codec 透传 I64，消费端用 wire::unpackChunkPosLong 解出 x/z。
 * 服务端通知客户端卸载指定区块（玩家走远/区块卸载），客户端消费复用 ClientWorld::onChunkUnload。
 */
struct ForgetLevelChunk {
    i64 packedPos; // Long（ChunkPos.asLong：X@bit0, Z@bit32）
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ForgetLevelChunk&, const ForgetLevelChunk&) noexcept = default;
};

/**
 * @brief SectionBlocksUpdate（S→C，id=82）
 *
 * 对应 Java 1.21.11 ClientboundSectionBlocksUpdatePacket：Long(sectionPos) + VarInt(count)
 * + count×VarLong(blockStatePacked)。单个区块段内多方块变更的聚合包，每条 blockStatePacked
 * 低 12 位为 section 内线性位置索引（0..4095，客户端经 SectionPos.relativeToBlockPos 还原），
 * 高 52 位为 blockStateId（Block.getId）。
 *
 * sectionPos 为固定 Long（SectionPos.STREAM_CODEC → ByteBufCodecs.LONG → writeLong/readLong，
 * 非 VarLong），编码 vanilla SectionPos.asLong：X 22 位 @ bit42，Z 22 位 @ bit20，Y 20 位 @ bit0
 * （合计 64 位）。注意项目 SectionPos::toLong/fromLong 位布局（X@42/Z@20/Y@0）与此不同，故 IR
 * 以原始 i64 直存，codec 透传 I64，不在 codec 层解包。客户端多方块变更聚合消费未实现，为 TODO 桩。
 */
struct SectionBlocksUpdate {
    i64 sectionPos;               // Long（SectionPos.asLong：X22@bit42, Z22@bit20, Y20@bit0）
    std::vector<i64> blockStates; // count×VarLong（低12位 localPos 索引 + 高52位 blockStateId）
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SectionBlocksUpdate&, const SectionBlocksUpdate&) noexcept = default;
};

/**
 * @brief ChunkBatchReceived（C→S，id=10）
 *
 * 对应 Java 1.21.11 ServerboundChunkBatchReceivedPacket：Float(chunksPerTick)。客户端在批次
 * 结束后回发本批次实际接收速率，服务端据此动态调整下一批次区块发送配额。服务端区块批量发送
 * 流速控未实现（ChunkSendManager::processPendingSends 当前每 tick 抽干就绪队列，无配额）。
 */
struct ChunkBatchReceived {
    f32 chunksPerTick; // Float
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ChunkBatchReceived&, const ChunkBatchReceived&) noexcept = default;
};

} // namespace mc::network::ir::play
