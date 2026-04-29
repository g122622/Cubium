#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include <vector>
#include <memory>

namespace mc::network {

// 数据包类型ID
enum class PacketType : u16 {
    // 内部控制包
    Handshake = 0,
    KeepAlive = 1,
    Disconnect = 2,

    // 客户端 -> 服务端 (登录阶段)
    LoginRequest = 100,

    // 客户端 -> 服务端 (游戏阶段)
    PlayerMove = 101,
    TeleportConfirm = 102,
    ChatMessage = 103,
    BlockInteraction = 104,
    PlayerTryUseItemOnBlock = 105,  // 方块放置

    // 服务端 -> 客户端 (登录阶段)
    LoginResponse = 200,

    // 服务端 -> 客户端 (游戏阶段)
    PlayerSpawn = 201,
    PlayerDespawn = 202,
    ChunkData = 203,
    UnloadChunk = 204,
    BlockUpdate = 205,
    Teleport = 206,
    ChatBroadcast = 207,
    TimeUpdate = 208,   // 时间同步
    GameStateChange = 209, // 游戏状态变化（天气等）

    // 实体同步包 (S->C)
    SpawnEntity = 210,          // 实体生成（物品、经验球等）
    SpawnMob = 211,             // Mob生成
    SpawnLiving = 212,          // LivingEntity生成
    EntityMetadata = 213,       // 实体数据同步
    EntityVelocity = 214,       // 实体速度
    EntityTeleport = 215,       // 实体传送
    EntityDestroy = 216,        // 实体销毁
    EntityAnimation = 217,      // 实体动画
    EntityMove = 218,           // 实体移动（相对）
    EntityHeadLook = 219,       // 实体头部朝向
    EntityStatus = 220,         // 实体状态（受伤、死亡等）
    LightUpdate = 221,          // 光照更新 (S->C)
    CollectItem = 222,          // 物品拾取动画 (S->C)
    BlockBreakAnim = 223,       // 方块破坏动画 (S->C)

    // 维度相关包
    ChangeDimension = 224,      // 维度切换 (S->C)
    Respawn = 225,              // 重生 (S->C)
    DimensionInfo = 226,        // 维度信息 (S->C)
    ConfirmDimensionChange = 227, // 确认维度切换 (C->S)
    SpawnPosition = 228,        // 世界出生点 (S->C)

    // 命令系统
    CommandTree = 229,          // 命令树同步 (S->C)

    // 玩家列表
    PlayerListItem = 231,       // 玩家列表更新 (S->C)

    // 睡眠系统
    Sleep = 230,                // 睡眠状态同步 (S->C)

    // 背包相关包 (双向)
    ContainerContent = 300,     // 容器内容同步 (S->C)
    ContainerSlot = 301,        // 单个槽位更新 (S->C)
    ContainerClick = 302,       // 容器点击 (C->S)
    CloseContainer = 303,       // 关闭容器 (双向)
    OpenContainer = 304,        // 打开容器 (S->C)
    PlayerInventory = 305,      // 玩家背包同步 (S->C)
    HotbarSelect = 306,         // 快捷栏选择 (C->S)
    HotbarSet = 307,            // 快捷栏设置 (S->C)
    PlayerAbilities = 308,      // 玩家能力同步 (S->C)
    CreativeInventoryAction = 309, // 创造模式背包动作 (C->S)
    ServerDifficulty = 310,     // 难度同步 (S->C)

    // 声音相关包 (S->C)
    PlaySound = 400,            // 播放声音
    StopSound = 401,            // 停止声音
    PlaySoundEffect = 402,      // 播放声音效果（实体/方块等）

    // 玩家经验包 (S->C)
    SetExperience = 500,        // 同步玩家经验
    SpawnExperienceOrb = 501,   // 生成经验球

    // 粒子包 (S->C)
    Particle = 510,             // 粒子生成
};

// 数据包头
struct PacketHeader {
    u32 size;           // 数据包总大小 (包含头部)
    u16 type;           // 数据包类型 (PacketType)
    u16 flags;          // 标志位
    u16 reserved;       // 保留
    u16 padding;        // 填充 (确保头部大小为12字节)
};

static_assert(sizeof(PacketHeader) == 12, "PacketHeader should be 12 bytes");

// 数据包基类
class Packet {
public:
    Packet(PacketType type);
    virtual ~Packet() = default;

    PacketType type() const { return m_type; }
    u16 flags() const { return m_flags; }
    void setFlags(u16 flags) { m_flags = flags; }

    // 序列化到字节数组
    [[nodiscard]] virtual Result<std::vector<u8>> serialize() const = 0;

    // 从字节数组反序列化
    [[nodiscard]] virtual Result<void> deserialize(const u8* data, size_t size) = 0;

    // 获取预期大小 (用于预分配)
    virtual size_t expectedSize() const { return sizeof(PacketHeader); }

protected:
    PacketType m_type;
    u16 m_flags = 0;
};

// 心跳包 (KeepAlive)
class KeepAlivePacket : public Packet {
public:
    KeepAlivePacket() : Packet(PacketType::KeepAlive) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u64 timestamp() const { return m_timestamp; }
    void setTimestamp(u64 ts) { m_timestamp = ts; }

private:
    u64 m_timestamp = 0;
};

// 断开连接包
class DisconnectPacket : public Packet {
public:
    DisconnectPacket() : Packet(PacketType::Disconnect) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    const String& reason() const { return m_reason; }
    void setReason(const String& reason) { m_reason = reason; }

private:
    String m_reason;
};

// 辅助函数
constexpr size_t PACKET_HEADER_SIZE = sizeof(PacketHeader);
constexpr u16 PACKET_FLAG_COMPRESSED = 0x0001;
constexpr u16 PACKET_FLAG_ENCRYPTED = 0x0002;
constexpr u16 PACKET_FLAG_RELIABLE = 0x0004;

} // namespace mc::network
