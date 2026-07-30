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

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::network::ir::play {

// ============================================================================
// 公共子结构
// ============================================================================

// ItemStackView 定义见 ItemStackView.hpp（独立成头供 entity/core 复用）。

/**
 * @brief 玩家移动位掩码（1.21.11 ServerboundMovePlayer 的 flags 字节）
 *
 * bit0=onGround，bit1=horizontalCollision。四个 MovePlayer 变体共用。
 */
struct MovePlayerFlags {
    bool onGround;
    bool horizontalCollision;

    [[nodiscard]] u8 pack() const noexcept
    {
        u8 v = 0;
        if (onGround) {
            v |= 0x01;
        }
        if (horizontalCollision) {
            v |= 0x02;
        }
        return v;
    }

    static MovePlayerFlags unpack(u8 v) noexcept { return MovePlayerFlags{(v & 0x01) != 0, (v & 0x02) != 0}; }

    [[nodiscard]] friend bool operator==(const MovePlayerFlags&, const MovePlayerFlags&) noexcept = default;
};

/**
 * @brief 方块命中结果（UseItemOn 的 blockHit 子结构）
 *
 * 对应 Java FriendlyByteBuf.writeBlockHitResult。
 * pos=命中方块坐标；direction=点击面（Direction ordinal 0..5）；
 * hitX/Y/Z=命中点相对方块原点的偏移；inside=是否在方块内；worldBorderHit=是否世界边界。
 */
struct BlockHitResult {
    i64 blockPosPacked; // BlockPos.asLong
    i32 direction;      // Direction ordinal
    f32 hitX;
    f32 hitY;
    f32 hitZ;
    bool inside;
    bool worldBorderHit;

    [[nodiscard]] friend bool operator==(const BlockHitResult&, const BlockHitResult&) noexcept = default;
};

// ============================================================================
// 通用包
// ============================================================================

/**
 * @brief KeepAlive（双向，S→C id=0 / C→S id=27）
 *
 * S→C 发 id，C→S 原样回 id。id 通常为服务端当前 tick 计数。
 */
struct KeepAlive {
    i64 id;
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const KeepAlive&, const KeepAlive&) noexcept = default;
};

/**
 * @brief Disconnect（S→C，id=1B）
 *
 * reason 为 JSON 文本组件字符串（组件 NBT 对齐留 Phase6）。
 */
struct Disconnect {
    std::string reason; // JSON 文本组件
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const Disconnect&, const Disconnect&) noexcept = default;
};

/**
 * @brief Chat（C→S，id=8）
 *
 * 1.21.11 签名链字段较复杂；当前承载消息 + 时间戳，签名相关字段留 TODO。
 */
struct Chat {
    std::string message;
    i64 timestamp; // epoch 毫秒
    i64 salt;
    std::optional<std::vector<u8>> signature; // 256 字节签名，离线模式 nullopt
    i32 lastSeenOffset;                       // LastSeenMessages.Update.offset
    std::array<u8, 3> lastSeenAcknowledged;   // 20 位 fixedBitSet → 3 字节
    u8 lastSeenChecksum;                      // LastSeenMessages.Update.checksum
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const Chat&, const Chat&) noexcept = default;
};

// ============================================================================
// 玩家移动（C→S，id=29/30/31/32）
// ============================================================================

/**
 * @brief MovePlayerPos（C→S，id=29）
 *
 * 线格式：F64(x)+F64(y)+F64(z)+U8(flags)。flags 见 MovePlayerFlags。
 */
struct MovePlayerPos {
    f64 x;
    f64 y;
    f64 z;
    MovePlayerFlags flags;
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const MovePlayerPos&, const MovePlayerPos&) noexcept = default;
};

/**
 * @brief MovePlayerPosRot（C→S，id=30）
 */
struct MovePlayerPosRot {
    f64 x;
    f64 y;
    f64 z;
    f32 yRot;
    f32 xRot;
    MovePlayerFlags flags;
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const MovePlayerPosRot&, const MovePlayerPosRot&) noexcept = default;
};

/**
 * @brief MovePlayerRot（C→S，id=31）
 */
struct MovePlayerRot {
    f32 yRot;
    f32 xRot;
    MovePlayerFlags flags;
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const MovePlayerRot&, const MovePlayerRot&) noexcept = default;
};

/**
 * @brief MovePlayerStatusOnly（C→S，id=32，仅 onGround/碰撞）
 */
struct MovePlayerStatusOnly {
    MovePlayerFlags flags;
    BedrockMeta bedrock{};

    [[nodiscard]] friend bool operator==(const MovePlayerStatusOnly&, const MovePlayerStatusOnly&) noexcept = default;
};

/**
 * @brief AcceptTeleportation（C→S，id=0）
 *
 * 玩家收到 PlayerPosition 后回此包确认 teleportId。
 */
struct AcceptTeleportation {
    i32 teleportId;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const AcceptTeleportation&, const AcceptTeleportation&) noexcept = default;
};

/**
 * @brief PlayerCommand（C→S，id=41，实体动作）
 *
 * action 0..6：STOP_SLEEPING/START_SPRINTING/STOP_SPRINTING/START_RIDING_JUMP/
 * STOP_RIDING_JUMP/OPEN_INVENTORY/START_FALL_FLYING。（sneaking 走 PlayerInput）
 */
struct PlayerCommand {
    i32 entityId;
    i32 action;
    i32 data;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerCommand&, const PlayerCommand&) noexcept = default;
};

/**
 * @brief PlayerInput（C→S，id=42，输入位掩码）
 *
 * 1 字节位域：bit0=forward bit1=backward bit2=left bit3=right
 * bit4=jump bit5=shift bit6=sprint。
 */
struct PlayerInput {
    u8 input;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerInput&, const PlayerInput&) noexcept = default;
};

/**
 * @brief UseItemOn（C→S，id=63，右键方块）
 */
struct UseItemOn {
    i32 hand; // 0=MAIN_HAND 1=OFF_HAND
    BlockHitResult blockHit;
    i32 sequence;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const UseItemOn&, const UseItemOn&) noexcept = default;
};

/**
 * @brief UseItem（C→S，id=64，右键空气/使用物品）
 *
 * 1.21.11 在 hand+sequence 后增加 yRot/xRot。
 */
struct UseItem {
    i32 hand;
    i32 sequence;
    f32 yRot;
    f32 xRot;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const UseItem&, const UseItem&) noexcept = default;
};

/**
 * @brief PlayerAction（C→S，id=40，开始/停止挖块等）
 *
 * 线格式：VarInt(action) + BlockPos(long) + Direction(byte) + VarInt(sequence)。
 * action: 0=START_DESTROY 1=ABORT_DESTROY 2=STOP_DESTROY 3=DROP_ALL_ITEMS
 * 4=DROP_ITEM 5=SWAP_ITEM_WITH_OFFHAND 6=SWAP_HANDS。
 */
struct PlayerAction {
    i32 action;
    i64 blockPosPacked; // BlockPos.asLong
    i32 direction;      // Direction ordinal
    i32 sequence;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerAction&, const PlayerAction&) noexcept = default;
};

// ============================================================================
// 玩家手持物品
// ============================================================================

/**
 * @brief SetCarriedItem（C→S，id=52，切热栏槽）
 *
 * 线格式：Short(slot)。slot 0..8。
 */
struct SetCarriedItem {
    i16 slot;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetCarriedItem&, const SetCarriedItem&) noexcept = default;
};

// ============================================================================
// 容器交互
// ============================================================================

/**
 * @brief 容器点击槽位变更（1.21.11 用 HashedStack）
 *
 * 对应 Java HashedStack：present=false 空；true 则 itemId+count+组件哈希 patch。
 * IR 仅承载 itemId/count；HashedPatchMap（added/removed 组件哈希）在 codec 层双端写空、
 * 读侧按定界跳过——我方互通自洽，真 Java 互通因哈希值为空而不做组件校验（可接受降级）。
 */
struct HashedStack {
    bool present;
    u32 itemId; // present=false 时无意义
    i32 count;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const HashedStack&, const HashedStack&) noexcept = default;
};

/**
 * @brief ContainerClick（C→S，id=17）
 *
 * 线格式：VarInt(containerId)+VarInt(stateId)+Short(slot)+Byte(button)+VarInt(clickType)
 *   + VarInt(changedSlots 数) × { Short(slot)+HashedStack } + HashedStack(carriedItem)。
 * clickType: 0=PICKUP 1=QUICK_MOVE 2=SWAP 3=CLONE 4=THROW 5=QUICK_CRAFT 6=PICKUP_ALL。
 */
struct ChangedSlot {
    i16 slot;
    HashedStack stack;
    [[nodiscard]] friend bool operator==(const ChangedSlot&, const ChangedSlot&) noexcept = default;
};
struct ContainerClick {
    i32 containerId;
    i32 stateId;
    i16 slotNum;
    i8 buttonNum;
    i32 clickType;
    std::vector<ChangedSlot> changedSlots;
    HashedStack carriedItem;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ContainerClick&, const ContainerClick&) noexcept = default;
};

/**
 * @brief ContainerClose（C→S id=18 / S→C id=17，关闭容器）
 *
 * 线格式：VarInt(containerId)。
 */
struct ContainerClose {
    i32 containerId;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ContainerClose&, const ContainerClose&) noexcept = default;
};

// ============================================================================
// terminal：Play→Configuration 切换
// ============================================================================

/**
 * @brief ConfigurationAcknowledged（C→S，id=15，terminal）
 *
 * 玩家在 Play 阶段收到服务端 StartConfiguration(S→C) 后回此包，双方切回 Configuration。
 */
struct ConfigurationAcknowledged {
    static constexpr bool kTerminal = true;

    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const ConfigurationAcknowledged&, const ConfigurationAcknowledged&) noexcept = default;
};

// ============================================================================
// 服务端→客户端：进游戏与全局状态
// ============================================================================

/**
 * @brief Login（S→C，id=48，进游戏 join）
 *
 * 对应 Java ClientboundLoginPacket。commonPlayerSpawnInfo 内联。
 * 线格式见 JavaCodecs（严格对齐 1.21.11 字段顺序）。
 *
 * dimensionType：我方互通用简单 VarInt(维度 id) 双端透传（客户端 Login 分支不消费该字段）；
 * Java 1.21.11 用 Holder<DimensionType>（VarInt mode：0=内联 NBT，>0=registry 引用），
 * 真内联模式未支持——仅影响真 Java 互通，不影响我方互通。
 */
struct CommonPlayerSpawnInfo {
    i32 dimensionType;     // 维度类型 id（我方互通 VarInt 透传；真 Java holder 编码见 struct 注释）
    std::string dimension; // 维度 ResourceKey，如 "minecraft:overworld"
    i64 seed;
    GameMode gameType;   // 0..3
    i8 previousGameType; // 0..3 或 -1 表 null
    bool isDebug;
    bool isFlat;
    std::optional<std::pair<std::string, i64>> lastDeathLocation; // dimension + BlockPos(long)
    i32 portalCooldown;
    i32 seaLevel;
    [[nodiscard]] friend bool operator==(const CommonPlayerSpawnInfo&, const CommonPlayerSpawnInfo&) noexcept = default;
};

struct Login {
    i32 playerId;
    bool hardcore;
    std::vector<std::string> levels; // 维度 ResourceKey 列表
    i32 maxPlayers;
    i32 chunkRadius;
    i32 simulationDistance;
    bool reducedDebugInfo;
    bool showDeathScreen;
    bool doLimitedCrafting;
    CommonPlayerSpawnInfo spawnInfo;
    bool enforcesSecureChat;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Login&, const Login&) noexcept = default;
};

/**
 * @brief PlayerPosition（S→C，id=70，传送玩家）
 *
 * 1.21.11 结构：teleportId + PositionMoveRotation(position,delta,yRot,xRot) + relatives(int 9 位)。
 * relatives 位：0=X 1=Y 2=Z 3=Y_ROT 4=X_ROT 5=DELTA_X 6=DELTA_Y 7=DELTA_Z 8=ROTATE_DELTA。
 */
struct PlayerPosition {
    i32 teleportId;
    f64 x;
    f64 y;
    f64 z;
    f64 deltaX;
    f64 deltaY;
    f64 deltaZ;
    f32 yRot;
    f32 xRot;
    u32 relatives; // 9 位位掩码
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerPosition&, const PlayerPosition&) noexcept = default;
};

/**
 * @brief SetTime（S→C，id=111）
 *
 * 线格式：Long(gameTime)+Long(dayTime)+Bool(tickDayTime)。
 */
struct SetTime {
    i64 gameTime;
    i64 dayTime;
    bool tickDayTime;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetTime&, const SetTime&) noexcept = default;
};

/**
 * @brief PlayerAbilities（S→C，id=62）
 *
 * flags 位：bit0=invulnerable bit1=flying bit2=canFly bit3=instabuild。
 */
struct PlayerAbilities {
    u8 flags;
    f32 flyingSpeed;
    f32 walkingSpeed;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerAbilities&, const PlayerAbilities&) noexcept = default;
};

/**
 * @brief SetHeldSlot（S→C，id=103，服务端同步玩家主手槽）
 */
struct SetHeldSlot {
    i32 slot;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetHeldSlot&, const SetHeldSlot&) noexcept = default;
};

/**
 * @brief SetChunkCacheCenter（S→C，id=76）
 *
 * 设置客户端 ClientChunkCache.Storage 的视野中心（viewCenterX/Z）。对齐 vanilla
 * ClientboundSetChunkCacheCenterPacket：VarInt(x) + VarInt(z)。客户端 inRange 判定
 * 为 Chebyshev 距离 |x-viewCenterX|<=chunkRadius && |z-viewCenterZ|<=chunkRadius，
 * 中心默认 (0,0)；若服务端从不发送此包，出生点远离原点的玩家收到的所有区块都会被
 * “Ignoring chunk since it's not in the view range” 丢弃，LevelLoadTracker 第二闸门
 * isSectionCompiledAndVisible 永远过不去，卡 “加载地形中” 直至心跳超时。
 * vanilla 在 ChunkMap.applyChunkTrackingView 中玩家区块中心变化时发送，须先于区块数据。
 */
struct SetChunkCacheCenter {
    i32 x;
    i32 z;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetChunkCacheCenter&, const SetChunkCacheCenter&) noexcept = default;
};

/**
 * @brief SetDefaultSpawnPosition（S→C，id=95）
 *
 * 1.21.11 为 RespawnData(globalPos, yaw, pitch)。
 */
struct SetDefaultSpawnPosition {
    std::string dimension; // GlobalPos 的 dimension ResourceKey
    i64 blockPosPacked;    // BlockPos.asLong
    f32 yaw;
    f32 pitch;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(
        const SetDefaultSpawnPosition&, const SetDefaultSpawnPosition&) noexcept = default;
};

/**
 * @brief ChangeDifficulty（S→C，id=10）
 */
struct ChangeDifficulty {
    i32 difficulty; // 0..3
    bool locked;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ChangeDifficulty&, const ChangeDifficulty&) noexcept = default;
};

/**
 * @brief GameEvent（S→C，id=38，原 GameStateChange）
 *
 * 线格式：Byte(event)+Float(value)。event 见 Java 1.21.11 枚举。
 */
struct GameEvent {
    u8 event;
    f32 value;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const GameEvent&, const GameEvent&) noexcept = default;
};

// ============================================================================
// 服务端→客户端：实体同步
// ============================================================================

/**
 * @brief AddEntity（S→C，id=1，生成实体）
 *
 * movement 用 1.21.11 LpVec3 低精度变长格式（codec 实现 LpVec3）。
 * 旋转为 packed degrees（byte）。
 *
 * entityTypeId：我方互通用 EntityRegistry 内部 id（VarInt）双端透传，客户端经
 * EntityRegistry::getTypeById 反查类型。Java 1.21.11 用 Holder<EntityType>
 * （VarInt mode：0=内联 ResourceLocation，>0=registry 引用）；引用模式与我方 id 语义
 * 自然对应，内联模式未支持——仅影响真 Java 互通，不影响我方互通。
 */
struct AddEntity {
    i32 entityId;
    std::array<u8, 16> uuid;
    i32 entityTypeId; // EntityType registry id
    f64 x;
    f64 y;
    f64 z;
    f64 movementX;
    f64 movementY;
    f64 movementZ;
    i8 yRot;     // packed degrees
    i8 xRot;     // packed degrees
    i8 yHeadRot; // packed degrees
    i32 data;    // 实体特定 data（如抛掷物 owner id）
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const AddEntity&, const AddEntity&) noexcept = default;
};

/**
 * @brief RemoveEntities（S→C，id=75）
 *
 * 线格式：VarInt(count) + count×VarInt(entityId)。
 */
struct RemoveEntities {
    std::vector<i32> entityIds;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const RemoveEntities&, const RemoveEntities&) noexcept = default;
};

/**
 * @brief TeleportEntity（S→C，id=123）
 *
 * 1.21.11 与 PlayerPosition 同构（含 delta + relatives + onGround）。
 */
struct TeleportEntity {
    i32 entityId;
    f64 x;
    f64 y;
    f64 z;
    f64 deltaX;
    f64 deltaY;
    f64 deltaZ;
    f32 yRot;
    f32 xRot;
    u32 relatives;
    bool onGround;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const TeleportEntity&, const TeleportEntity&) noexcept = default;
};

/**
 * @brief MoveEntityPos（S→C，id=51，相对位移）
 *
 * 线格式：VarInt(entityId)+Short(xa)+Short(ya)+Short(za)+Bool(onGround)。
 */
struct MoveEntityPos {
    i32 entityId;
    i16 deltaX;
    i16 deltaY;
    i16 deltaZ;
    bool onGround;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const MoveEntityPos&, const MoveEntityPos&) noexcept = default;
};

/**
 * @brief MoveEntityPosRot（S→C，id=52）
 */
struct MoveEntityPosRot {
    i32 entityId;
    i16 deltaX;
    i16 deltaY;
    i16 deltaZ;
    i8 yRot; // packed degrees
    i8 xRot; // packed degrees
    bool onGround;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const MoveEntityPosRot&, const MoveEntityPosRot&) noexcept = default;
};

/**
 * @brief MoveEntityRot（S→C，id=54）
 */
struct MoveEntityRot {
    i32 entityId;
    i8 yRot;
    i8 xRot;
    bool onGround;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const MoveEntityRot&, const MoveEntityRot&) noexcept = default;
};

/**
 * @brief SetEntityMotion（S→C，id=99）
 *
 * 1.21.11 用 LpVec3。当前承载 3 个 double（codec 实现 LpVec3 编码）。
 */
struct SetEntityMotion {
    i32 entityId;
    f64 x;
    f64 y;
    f64 z;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetEntityMotion&, const SetEntityMotion&) noexcept = default;
};

/**
 * @brief RotateHead（S→C，id=81）
 *
 * 线格式：VarInt(entityId)+Byte(packed yHeadRot)。
 */
struct RotateHead {
    i32 entityId;
    i8 yHeadRot;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const RotateHead&, const RotateHead&) noexcept = default;
};

/**
 * @brief SetEntityData（S→C，id=97，实体元数据）
 *
 * 元数据为序列化的 DataValue 字节流（byte index + VarInt serializerId + value）+ EOF 0xFF，
 * 由 EntityMetadataSerializer（1.21.11 格式）生成/解析。packedItems 透传该完整字节段。
 */
struct SetEntityData {
    i32 entityId;
    std::vector<u8> packedItems; // 已序列化的元数据字节（含 EOF 0xFF）
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SetEntityData&, const SetEntityData&) noexcept = default;
};

// ============================================================================
// 服务端→客户端：区块与方块
// ============================================================================

// LevelChunkWithLight（S→C，id=44，区块数据 + 光照）结构化定义见 PlayPacketsExtended.hpp：
// 该包 IR 携带 vanilla 语义字段（PalettedContainer section / 高度图 long[] / BlockEntityInfo /
// 光照 BitSet+List），其中 BlockEntityInfo 需 NBT，故置于已 include nbt 的 Extended 头。

/**
 * @brief LightUpdate（S→C，id=47）
 *
 * 线格式（1.21.11 ClientboundLightUpdatePacket）：
 *   VarInt(x) + VarInt(z) + 4×BitSet(长整型数组) + 2×List<byte[≤2048]>
 *   四个 BitSet 顺序：skyYMask / blockYMask / emptySkyYMask / emptyBlockYMask。
 *   两个列表顺序：skyUpdates / blockUpdates（每个元素 VarInt(len)+nibble 字节）。
 *   BitSet 以最小长整型数组序列化（VarInt(longCount) + longCount×i64 大端），位 i 对应
 *   光照段 Y = minLightSection + i（minLightSection = MIN_SECTION_Y - 1，主世界=-5）。
 *   yMask 中的位表示该光照段有非空 nibble 数据（对应列表里一条 2048 字节）；
 *   emptyMask 中的位表示该光照段为空（全亮，无 nibble 数据）。
 */
struct LightUpdate {
    i32 x;
    i32 z;
    /// skyYMask / blockYMask / emptySkyYMask / emptyBlockYMask（最小长整型数组形式，big-endian 线编码）。
    std::array<std::vector<i64>, 4> lightMasks;
    /// skyUpdates / blockUpdates，每条是一个光照段的 2048 字节 nibble（顺序与对应 yMask 的置位位一致）。
    std::array<std::vector<std::vector<u8>>, 2> lightUpdates;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const LightUpdate&, const LightUpdate&) noexcept = default;
};

/**
 * @brief BlockUpdate（S→C，id=8）
 *
 * 线格式：BlockPos(long)+VarInt(blockStateId)。
 */
struct BlockUpdate {
    i64 blockPosPacked; // BlockPos.asLong
    i32 blockStateId;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const BlockUpdate&, const BlockUpdate&) noexcept = default;
};

// ============================================================================
// 服务端→客户端：容器同步
// ============================================================================

/**
 * @brief ContainerSetContent（S→C，id=18）
 *
 * 线格式：VarInt(containerId)+VarInt(stateId)+VarInt(count)×ItemStack(optional)
 *   + carriedItem(ItemStack optional)。
 */
struct ContainerSetContent {
    i32 containerId;
    i32 stateId;
    std::vector<ItemStackView> items;
    ItemStackView carriedItem;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ContainerSetContent&, const ContainerSetContent&) noexcept = default;
};

/**
 * @brief ContainerSetSlot（S→C，id=20）
 *
 * 线格式：VarInt(containerId)+VarInt(stateId)+Short(slot)+ItemStack(optional)。
 */
struct ContainerSetSlot {
    i32 containerId;
    i32 stateId;
    i16 slot;
    ItemStackView item;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ContainerSetSlot&, const ContainerSetSlot&) noexcept = default;
};

/**
 * @brief OpenScreen（S→C，id=57）
 *
 * 线格式：VarInt(containerId)+VarInt(menuType)+Utf8(title JSON)。
 */
struct OpenScreen {
    i32 containerId;
    i32 menuType;
    std::string title; // JSON 文本组件
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const OpenScreen&, const OpenScreen&) noexcept = default;
};

/**
 * @brief ContainerSetData（S→C，id=19，原 WindowProperty）
 *
 * 线格式：VarInt(containerId)+Short(property)+Short(value)。
 */
struct ContainerSetData {
    i32 containerId;
    i16 property;
    i16 value;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ContainerSetData&, const ContainerSetData&) noexcept = default;
};

// ============================================================================
// 服务端→客户端：玩家列表
// ============================================================================

/**
 * @brief PlayerInfoUpdate（S→C，id=68）
 *
 * 1.21.11 action 位掩码（9 位）+ entries。每个 entry 按 set 的 action 顺序载负载。
 * 当前承载 actions + per-entry 负载字段（可选）。
 * TODO(Phase6): 完整 INITIALIZE_CHAT（RemoteChatSession.Data）对齐。
 */
struct PlayerInfoEntry {
    std::array<u8, 16> uuid;
    // ADD_PLAYER
    std::optional<std::string> name;
    std::vector<std::pair<std::string, std::string>> properties; // name→value
    // UPDATE_GAME_MODE
    std::optional<i32> gameMode;
    // UPDATE_LISTED
    std::optional<bool> listed;
    // UPDATE_LATENCY
    std::optional<i32> latency;
    // UPDATE_DISPLAY_NAME：我方不生产/不消费显示名（客户端分支为扩展点），IR 不承载该字段；
    // codec 读侧遇真 Java 服务端的 displayName 时按 NBT compound 跳过（nbt_io::readCompound），
    // 写侧固定 Bool(false)。上层接入 ITextComponent NBT codec 后如需展示再补字段。
    // UPDATE_LIST_ORDER
    std::optional<i32> listOrder;
    // UPDATE_HAT
    std::optional<bool> showHat;
    [[nodiscard]] friend bool operator==(const PlayerInfoEntry&, const PlayerInfoEntry&) noexcept = default;
};

struct PlayerInfoUpdate {
    u16 actions; // 9 位位掩码
    std::vector<PlayerInfoEntry> entries;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerInfoUpdate&, const PlayerInfoUpdate&) noexcept = default;
};

/**
 * @brief PlayerInfoRemove（S→C，id=67，移除玩家列表项）
 *
 * 线格式：VarInt(count) + count×UUID。
 */
struct PlayerInfoRemove {
    std::vector<std::array<u8, 16>> uuids;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PlayerInfoRemove&, const PlayerInfoRemove&) noexcept = default;
};

} // namespace mc::network::ir::play
