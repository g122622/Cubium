#pragma once

#include "Types.hpp"
#include <cmath>
#include <limits>

namespace mc {

// ============================================================================
// 游戏常量
// ============================================================================

namespace game {

// 重力加速度 (blocks/tick²)
constexpr f32 GRAVITY = 0.08f;

// 玩家尺寸
constexpr f32 PLAYER_HEIGHT = 1.8f;
constexpr f32 PLAYER_EYE_HEIGHT = 1.62f;
constexpr f32 PLAYER_WIDTH = 0.6f;
constexpr f32 PLAYER_SNEAK_HEIGHT = 1.5f;

// 玩家生命值
constexpr f32 PLAYER_MAX_HEALTH = 20.0f;
constexpr f32 PLAYER_MAX_AIR = 300.0f;

// 光照
constexpr i32 MAX_LIGHT_LEVEL = 15;
constexpr i32 MIN_LIGHT_LEVEL = 0;

// 时间
constexpr i32 DAY_LENGTH_TICKS = 24000;
constexpr i32 DAY_LENGTH_SECONDS = 1200; // 20分钟

} // namespace game

// ============================================================================
// 网络常量
// ============================================================================

namespace network {

// 协议版本
constexpr i32 PROTOCOL_VERSION = 765; // 1.20.4

// 端口
constexpr u16 DEFAULT_PORT = 19132;
constexpr u16 DEFAULT_RCON_PORT = 25575;

// 数据包限制
constexpr Size MAX_PACKET_SIZE = 2097152;  // 2MB
constexpr Size MAX_UNCOMPRESSED_SIZE = 2097152;
constexpr Size MIN_COMPRESSION_THRESHOLD = 256;

// 超时
constexpr u32 CONNECT_TIMEOUT_MS = 30000;
constexpr u32 READ_TIMEOUT_MS = 30000;
constexpr u32 WRITE_TIMEOUT_MS = 30000;

// 心跳
constexpr u32 KEEP_ALIVE_INTERVAL_MS = 15000;
constexpr u32 KEEP_ALIVE_TIMEOUT_MS = 30000;

// 速率限制
constexpr u32 MAX_PACKETS_PER_SECOND = 1000;
constexpr u32 MAX_LOGIN_ATTEMPTS = 5;

} // namespace network

// ============================================================================
// 世界常量
// ============================================================================

namespace world {

// 区块尺寸
constexpr i32 CHUNK_WIDTH = 16;
constexpr i32 CHUNK_HEIGHT = 256;
constexpr i32 CHUNK_SECTION_HEIGHT = 16;
constexpr i32 CHUNK_SECTIONS = CHUNK_HEIGHT / CHUNK_SECTION_HEIGHT;
constexpr i32 CHUNK_VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;

// 区块块尺寸（区块内的位偏移）
constexpr i32 CHUNK_SHIFT = 4; // log2(16) = 4
constexpr i32 SECTION_SHIFT = 4;
constexpr i32 CHUNK_MASK = CHUNK_WIDTH - 1;

// 高度限制
constexpr i32 MIN_BUILD_HEIGHT = 0;
constexpr i32 MAX_BUILD_HEIGHT = CHUNK_HEIGHT;
constexpr i32 SEA_LEVEL = 62;

// 区块加载
constexpr i32 CHUNK_LOAD_RADIUS = 10;
constexpr i32 CHUNK_UNLOAD_RADIUS = 12;
constexpr i32 MAX_CHUNKS_LOADED = 1024;

// 世界生成
constexpr i64 WORLD_SEED_DEFAULT = 0;
constexpr i32 SPAWN_CHUNK_RADIUS = 11;

// 方块更新
constexpr i32 BLOCK_UPDATE_RADIUS = 16;

} // namespace world

// ============================================================================
// 实体常量
// ============================================================================

namespace entity {

// 限制
constexpr Size MAX_ENTITIES_PER_CHUNK = 1024;
constexpr Size MAX_PLAYERS = 256;
constexpr Size MAX_ENTITIES = 65536;

// 实体类型ID（旧枚举，用于网络同步）
// 注意：新代码应使用 mc::entity::EntityType 类和 mc::entity::EntityTypeId
enum class LegacyEntityTypeId : u32 {
    None = 0,
    Player = 1,
    Zombie = 2,
    Skeleton = 3,
    Creeper = 4,
    Spider = 5,
    Enderman = 6,
    // ... 更多实体类型
};

// 实体状态
enum class EntityStatus : u8 {
    Valid = 0,
    Dead = 1,
    Removed = 2
};

// 追踪距离
constexpr i32 ENTITY_TRACKING_RANGE = 128;
constexpr i32 PLAYER_TRACKING_RANGE = 128;

} // namespace entity

} // namespace mc

// ============================================================================
// 爆炸常量
// ============================================================================

namespace mc::game::explosion {

// 射线追踪参数
constexpr i32 RAY_GRID_SIZE = 16;
constexpr f32 RAY_STEP_SIZE = 0.3f;

// 强度衰减公式系数
constexpr f32 RESISTANCE_COEFFICIENT = 0.3f;
constexpr f32 INITIAL_STRENGTH_MIN = 0.7f;
constexpr f32 INITIAL_STRENGTH_RANGE = 0.6f;

// 实体影响范围系数
constexpr f32 ENTITY_RANGE_MULTIPLIER = 2.0f;

// 伤害公式系数
constexpr f32 DAMAGE_MULTIPLIER = 7.0f;

// 火焰生成概率
constexpr f32 FIRE_SPAWN_CHANCE = 0.333f;

// 爆炸音量和音调
constexpr f32 EXPLOSION_VOLUME = 4.0f;
constexpr f32 EXPLOSION_PITCH_BASE = 0.7f;
constexpr f32 EXPLOSION_PITCH_RANGE = 0.2f;

// 默认爆炸半径
constexpr f32 TNT_RADIUS = 4.0f;
constexpr f32 CREEPER_RADIUS = 3.0f;
constexpr f32 CHARGED_CREEPER_RADIUS_MULTIPLIER = 2.0f;
constexpr f32 GHAST_FIREBALL_RADIUS = 1.0f;
constexpr f32 WITHER_SKULL_RADIUS = 1.0f;
constexpr f32 WITHER_SPAWN_RADIUS = 7.0f;
constexpr f32 END_CRYSTAL_RADIUS = 6.0f;
constexpr f32 BED_RADIUS = 5.0f;

} // namespace mc::game::explosion

namespace mc {
namespace capacity {

// 缓冲区
constexpr Size DEFAULT_BUFFER_SIZE = 4096;
constexpr Size PACKET_BUFFER_SIZE = 65536;

// 容器初始容量
constexpr Size ENTITY_LIST_INITIAL = 64;
constexpr Size CHUNK_MAP_INITIAL = 256;
constexpr Size PLAYER_LIST_INITIAL = 16;

// 字符串
constexpr Size MAX_PLAYER_NAME_LENGTH = 16;
constexpr Size MAX_CHAT_MESSAGE_LENGTH = 256;
constexpr Size MAX_COMMAND_LENGTH = 32500;
constexpr Size MAX_PATH_LENGTH = 256;

} // namespace capacity

} // namespace mc
