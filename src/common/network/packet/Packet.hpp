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

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::network {

/**
 * @brief 数据包类型枚举
 *
 * 定义所有网络数据包的类型ID，用于序列化和反序列化时识别数据包类型。
 * 类型ID范围：
 * - 0-99: 内部控制包
 * - 100-199: 客户端 -> 服务端包
 * - 200-299: 服务端 -> 客户端包
 * - 300+: 特殊功能包
 */
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
    PlayerTryUseItemOnBlock = 105, // 方块放置
    PlayerInput = 106,             // 玩家输入 (骑乘/移动)
    MoveVehicle = 107,             // 载具移动
    EntityAction = 108,            // 实体动作 (跳跃、潜行等)
    UseEntity = 109,               // 实体交互
    SteerBoat = 110,               // 船划桨状态
    UpdateSign = 111,              // 告示牌文本更新 (C->S)

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
    TimeUpdate = 208,      // 时间同步
    GameStateChange = 209, // 游戏状态变化（天气等）

    // 实体同步包 (S->C)
    SpawnEntity = 210,     // 实体生成（物品、经验球等）
    SpawnMob = 211,        // Mob生成
    SpawnLiving = 212,     // LivingEntity生成
    EntityMetadata = 213,  // 实体数据同步
    EntityVelocity = 214,  // 实体速度
    EntityTeleport = 215,  // 实体传送
    EntityDestroy = 216,   // 实体销毁
    EntityAnimation = 217, // 实体动画
    EntityMove = 218,      // 实体移动（相对）
    EntityHeadLook = 219,  // 实体头部朝向
    EntityStatus = 220,    // 实体状态（受伤、死亡等）
    LightUpdate = 221,     // 光照更新 (S->C)
    CollectItem = 222,     // 物品拾取动画 (S->C)
    BlockBreakAnim = 223,  // 方块破坏动画 (S->C)
    SetPassengers = 224,   // 设置乘客列表 (S->C)

    // 维度相关包
    Respawn = 225,                // 重生/维度切换 (S->C)
    DimensionInfo = 226,          // 维度信息 (S->C)
    ConfirmDimensionChange = 227, // 确认维度切换 (C->S)
    SpawnPosition = 228,          // 世界出生点 (S->C)
    VehicleMove = 229,            // 载具移动同步 (S->C)

    // 命令系统
    CommandTree = 230, // 命令树同步 (S->C)

    // 睡眠系统
    Sleep = 231, // 睡眠状态同步 (S->C)

    // 玩家列表
    PlayerListItem = 232, // 玩家列表更新 (S->C)

    // 旁观者系统
    SetCamera = 233, // 设置摄像机实体 (S->C)，用于旁观者跟踪

    // 实体拴绳链接包 (S->C)
    SetEntityLink = 234, // 设置实体拴绳链接 (S->C)

    // 方块事件包 (S->C)
    BlockEvent = 235, // 方块事件（箱子开合、活塞动画等）

    // 告示牌编辑器打开包 (S->C)
    OpenSignEditor = 236, // 通知客户端打开告示牌编辑器

    // 方块实体数据同步包 (S->C)
    BlockEntityData = 237, // 同步方块实体数据（告示牌文本等）

    // 背包相关包 (双向)
    ContainerContent = 300,    // 容器内容同步 (S->C)
    ContainerSlot = 301,       // 单个槽位更新 (S->C)
    ContainerClick = 302,      // 容器点击 (C->S)
    CloseContainer = 303,      // 关闭容器 (双向)
    OpenContainer = 304,       // 打开容器 (S->C)
    PlayerInventory = 305,     // 玩家背包同步 (S->C)
    HotbarSelect = 306,        // 快捷栏选择 (C->S)
    HotbarSet = 307,           // 快捷栏设置 (S->C)
    PlayerAbilities = 308,     // 玩家能力同步 (S->C)
    ServerDifficulty = 310,    // 难度同步 (S->C)
    OpenPlayerInventory = 311, // 请求打开玩家背包容器 (C->S)

    // 声音相关包 (S->C)
    PlaySound = 400,       // 播放声音
    StopSound = 401,       // 停止声音
    PlaySoundEffect = 402, // 播放声音效果（实体/方块等）
    MovingSound = 403,     // 移动声音（跟随实体）
    WorldEvent = 404,      // 世界事件（音效/粒子效果）

    // 玩家经验包 (S->C)
    SetExperience = 500,      // 同步玩家经验
    SpawnExperienceOrb = 501, // 生成经验球

    // 粒子包 (S->C)
    Particle = 510, // 粒子生成

    // 爆炸包 (S->C)
    Explosion = 511, // 爆炸事件

    // 标题包 (S->C)
    Title = 520, // 标题显示

    // 世界边界包 (S->C)
    WorldBorder = 530, // 世界边界同步

    // Boss 栏包 (S->C)
    BossInfo = 535, // Boss 栏同步

    // 地图包 (S->C)
    MapData = 550, // 地图数据更新

    // 成就包 (S->C)
    AdvancementInfo = 540,      // 成就信息同步
    SelectAdvancementTab = 541, // 成就标签页选择

    // 成就包 (C->S)
    SeenAdvancements = 600, // 成就界面操作

    // 记分板包 (S->C)
    ScoreboardObjective = 700, // 目标同步 (创建/移除/更新)
    UpdateScore = 701,         // 分数更新 (设置/移除)
    DisplayObjective = 702,    // 显示目标 (设置显示槽位)
    Teams = 703,               // 队伍同步 (创建/移除/更新/成员变更)
};

/**
 * @brief 数据包头结构
 *
 * 每个网络数据包都以固定的12字节头部开始，包含包大小、类型、标志位等信息。
 */
struct PacketHeader {
    u32 size;     // 数据包总大小 (包含头部)
    u16 type;     // 数据包类型 (PacketType)
    u16 flags;    // 标志位
    u16 reserved; // 保留
    u16 padding;  // 填充 (确保头部大小为12字节)
};

static_assert(sizeof(PacketHeader) == 12, "PacketHeader should be 12 bytes");

/**
 * @brief 数据包基类
 *
 * 所有网络数据包的抽象基类，定义了序列化和反序列化接口。
 * 派生类必须实现 serialize() 和 deserialize() 方法。
 */
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

/**
 * @brief 心跳包
 *
 * 用于维持连接活跃状态，客户端和服务端双向发送。
 * 包含时间戳用于计算往返延迟。
 */
class KeepAlivePacket : public Packet {
public:
    KeepAlivePacket()
        : Packet(PacketType::KeepAlive)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u64 timestamp() const { return m_timestamp; }
    void setTimestamp(u64 ts) { m_timestamp = ts; }

private:
    u64 m_timestamp = 0;
};

/**
 * @brief 断开连接包
 *
 * 用于通知对方断开连接，包含断开原因的文本说明。
 */
class DisconnectPacket : public Packet {
public:
    DisconnectPacket()
        : Packet(PacketType::Disconnect)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    const std::string& reason() const { return m_reason; }
    void setReason(const std::string& reason) { m_reason = reason; }

private:
    std::string m_reason;
};

// 辅助函数
constexpr size_t PACKET_HEADER_SIZE = sizeof(PacketHeader);
constexpr u16 PACKET_FLAG_COMPRESSED = 0x0001;
constexpr u16 PACKET_FLAG_ENCRYPTED = 0x0002;
constexpr u16 PACKET_FLAG_RELIABLE = 0x0004;

} // namespace mc::network
