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
#include "common/network/ir/packets/play/PlayPackets.hpp"
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
// 复杂嵌套树（命令树 Node、配方 RecipeDisplay、进度 Advancement 树、Component 文本、
// ParticleOptions、NumberFormat、MapDecoration/MapPatch、Explosion 粒子表、WeightedList）
// 暂以 opaque 字节透传承载（std::vector<u8>），codec 仅按 VarInt(len)+bytes 读写，
// 完整解析留待后续阶段（标 TODO(Phase6)），不影响我方互通。
// 复用 PlayPackets.hpp 的 CommonPlayerSpawnInfo（Respawn 内联）。
// ============================================================================

// ----------------------------------------------------------------------------
// 声音（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief PlaySound（S→C，id=115）
 *
 * Holder<SoundEvent> 暂用 opaque 字节透传（registry id 或内联 SoundEvent）。
 * 坐标为 ×8 取整后的 int（Java writeInt）。
 */
struct PlaySound {
    std::vector<u8> soundHolder; // opaque：Holder<SoundEvent>
    i32 source;                  // SoundSource ordinal
    i32 x;                       // 实际坐标 ×8 取整
    i32 y;
    i32 z;
    f32 volume;
    f32 pitch;
    i64 seed;
    BedrockMeta bedrock{};
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
};

/**
 * @brief SoundEntity（S→C，id=114，实体发声）
 */
struct SoundEntity {
    std::vector<u8> soundHolder; // opaque：Holder<SoundEvent>
    i32 source;
    i32 entityId;
    f32 volume;
    f32 pitch;
    i64 seed;
    BedrockMeta bedrock{};
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
};

// ----------------------------------------------------------------------------
// 粒子（S→C）
// ----------------------------------------------------------------------------

/**
 * @brief LevelParticles（S→C，id=46）
 *
 * ParticleOptions 为 registry id + 粒子专属 options，暂 opaque 透传。
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
    std::vector<u8> particle; // opaque：ParticleOptions
    BedrockMeta bedrock{};
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
};

// ----------------------------------------------------------------------------
// 进度（Advancements）
// ----------------------------------------------------------------------------

/**
 * @brief UpdateAdvancements（S→C，id=128，opaque）
 *
 * 整体 opaque：reset + List<AdvancementHolder> + Set<Identifier> removed
 * + Map<Identifier,AdvancementProgress> + bool showAdvancements。
 * TODO(Phase6): 完整进度树解析。
 */
struct UpdateAdvancements {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
};

/**
 * @brief SelectAdvancementTab（S→C，id=83）
 *
 * Nullable<Identifier>：present=false 表关闭标签页。
 */
struct SelectAdvancementTab {
    bool present;
    std::string tab; // present=true 时有效
    BedrockMeta bedrock{};
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
};

/**
 * @brief SetPlayerTeam（S→C，id=107，单包 + method 分发）
 *
 * method：0=ADD 1=REMOVE 2=CHANGE 3=JOIN 4=LEAVE。
 * ADD/CHANGE 带 Parameters（displayName/options/visibility/collision/color/prefix/suffix），
 * ADD/JOIN/LEAVE 带 players 列表。Parameters 内 Component 字段 opaque 透传。
 */
struct SetPlayerTeam {
    std::string name;
    u8 method;
    std::vector<u8> parameters;       // opaque：Parameters（method 0/2）
    std::vector<std::string> players; // method 0/3/4
    BedrockMeta bedrock{};
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
};

/**
 * @brief SetSubtitleText（S→C，id=110）
 */
struct SetSubtitleText {
    std::vector<u8> text; // opaque：Component
    BedrockMeta bedrock{};
};

/**
 * @brief SetActionBarText（S→C，id=85）
 */
struct SetActionBarText {
    std::vector<u8> text; // opaque：Component
    BedrockMeta bedrock{};
};

/**
 * @brief SetTitlesAnimation（S→C，id=113）
 */
struct SetTitlesAnimation {
    i32 fadeIn;
    i32 stay;
    i32 fadeOut;
    BedrockMeta bedrock{};
};

/**
 * @brief ClearTitles（S→C，id=14）
 */
struct ClearTitles {
    bool resetTimes;
    BedrockMeta bedrock{};
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
};

/**
 * @brief SetBorderCenter（S→C，id=86）
 */
struct SetBorderCenter {
    f64 newCenterX;
    f64 newCenterZ;
    BedrockMeta bedrock{};
};

/**
 * @brief SetBorderLerpSize（S→C，id=87）
 */
struct SetBorderLerpSize {
    f64 oldSize;
    f64 newSize;
    i64 lerpTime; // VarLong
    BedrockMeta bedrock{};
};

/**
 * @brief SetBorderSize（S→C，id=88）
 */
struct SetBorderSize {
    f64 size;
    BedrockMeta bedrock{};
};

/**
 * @brief SetBorderWarningDelay（S→C，id=89）
 */
struct SetBorderWarningDelay {
    i32 warningDelay; // VarInt
    BedrockMeta bedrock{};
};

/**
 * @brief SetBorderWarningDistance（S→C，id=90）
 */
struct SetBorderWarningDistance {
    i32 warningBlocks; // VarInt
    BedrockMeta bedrock{};
};

// ----------------------------------------------------------------------------
// 地图（S→C，opaque）
// ----------------------------------------------------------------------------

/**
 * @brief MapItemData（S→C，id=49，opaque）
 *
 * mapId+scale+locked+Optional<List<MapDecoration>>+Optional<MapPatch>。整体 opaque。
 * TODO(Phase6): 完整地图装饰/色块解析。
 */
struct MapItemData {
    std::vector<u8> payload; // opaque
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
};

/**
 * @brief SetPassengers（S→C，id=105）
 */
struct SetPassengers {
    i32 vehicle;                 // VarInt
    std::vector<i32> passengers; // VarInt 列表
    BedrockMeta bedrock{};
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
};

/**
 * @brief HurtAnimation（S→C，id=41）
 */
struct HurtAnimation {
    i32 id; // VarInt
    f32 yaw;
    BedrockMeta bedrock{};
};

/**
 * @brief TakeItemEntity（S→C，id=122，拾取动画）
 */
struct TakeItemEntity {
    i32 itemId;   // VarInt
    i32 playerId; // VarInt
    i32 amount;   // VarInt
    BedrockMeta bedrock{};
};

/**
 * @brief BlockDestruction（S→C，id=5）
 */
struct BlockDestruction {
    i32 id; // VarInt（破坏者实体 id）
    i64 blockPosPacked;
    u8 progress;
    BedrockMeta bedrock{};
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
};

// ----------------------------------------------------------------------------
// 爆炸（S→C，opaque）
// ----------------------------------------------------------------------------

/**
 * @brief Explosion（S→C，id=36，1.21.11 结构，opaque）
 *
 * 1.21.11 已无 BlockInteraction 枚举与 affectedBlocks 列表，改为 blockCount:int +
 * WeightedList<ExplosionParticleInfo> + ParticleOptions explosionParticle +
 * Holder<SoundEvent> explosionSound。整体 opaque。
 * TODO(Phase6): 完整爆炸粒子表解析。
 */
struct Explosion {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
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
};

/**
 * @brief PaddleBoat（C→S，id=34）
 */
struct PaddleBoat {
    bool left;
    bool right;
    BedrockMeta bedrock{};
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
};

// ----------------------------------------------------------------------------
// 命令树（S→C，opaque）
// ----------------------------------------------------------------------------

/**
 * @brief Commands（S→C，id=16，opaque）
 *
 * List<Node> + VarInt rootIndex。Node 树（flags/children/redirect/按 type 分发的
 * Literal/Argument stub）整体 opaque。
 * TODO(Phase6): 完整命令树 Node 解析。
 */
struct Commands {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
};

// ----------------------------------------------------------------------------
// 配方
// ----------------------------------------------------------------------------

/**
 * @brief UpdateRecipes（S→C，id=131，opaque）
 *
 * 1.21.11 为 RecipePropertySet map + 切石机配方集合，opaque。
 * TODO(Phase6): 完整配方集解析。
 */
struct UpdateRecipes {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
};

/**
 * @brief RecipeBookAdd（S→C，id=72，opaque）
 *
 * List<Entry>(contents=RecipeDisplayEntry + flags) + bool replace，opaque。
 * TODO(Phase6): 完整 RecipeDisplay 解析。
 */
struct RecipeBookAdd {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
};

/**
 * @brief RecipeBookRemove（S→C，id=73，opaque）
 *
 * List<RecipeDisplayId>。整体 opaque。
 * TODO(Phase6): 解析。
 */
struct RecipeBookRemove {
    std::vector<u8> payload; // opaque
    BedrockMeta bedrock{};
};

/**
 * @brief PlaceGhostRecipe（S→C，id=61，opaque）
 *
 * VarInt(containerId) + RecipeDisplay。RecipeDisplay opaque。
 * TODO(Phase6): 解析。
 */
struct PlaceGhostRecipe {
    i32 containerId;               // VarInt
    std::vector<u8> recipeDisplay; // opaque
    BedrockMeta bedrock{};
};

/**
 * @brief PlaceRecipe（C→S，id=38）
 */
struct PlaceRecipe {
    i32 containerId; // VarInt
    i32 recipe;      // VarInt RecipeDisplayId
    bool useMaxItems;
    BedrockMeta bedrock{};
};

} // namespace mc::network::ir::play
