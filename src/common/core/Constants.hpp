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

#include "Types.hpp"
#include <cmath>
#include <limits>

namespace mc {

// ============================================================================
// 游戏常量
// ============================================================================

namespace game {

// 玩家尺寸 (MC 1.16.5 PlayerEntity.java:114-115)
// 注意：Player.hpp 中有相同的常量定义，用于 Player 类内部使用
// 此处常量供非 Player 类使用，Player 类应使用自己的静态常量
constexpr f32 PLAYER_HEIGHT = 1.8f;       // 玩家站立高度
constexpr f32 PLAYER_EYE_HEIGHT = 1.62f;  // 玩家眼睛高度
constexpr f32 PLAYER_WIDTH = 0.6f;        // 玩家宽度
constexpr f32 PLAYER_SNEAK_HEIGHT = 1.5f; // 玩家潜行高度
constexpr f32 PLAYER_SWIM_HEIGHT = 0.6f;  // 玩家游泳/鞘翅高度
constexpr f32 PLAYER_SLEEP_HEIGHT = 0.2f; // 玩家睡觉/死亡高度

// 玩家生命值
constexpr f32 PLAYER_MAX_HEALTH = 20.0f;
constexpr f32 PLAYER_MAX_AIR = 300.0f;

// 光照
constexpr i32 MAX_LIGHT_LEVEL = 15;
constexpr i32 MIN_LIGHT_LEVEL = 0;

// 时间
constexpr i32 DAY_LENGTH_TICKS = 24000;
constexpr i32 DAY_LENGTH_SECONDS = 1200; // 20分钟
constexpr i32 TICKS_PER_SECOND = 20;     // 每秒20刻

/**
 * @brief 秒转换为刻
 * @param seconds 秒数
 * @return 刻数
 */
inline constexpr i32 secondsToTicks(i32 seconds)
{
    return seconds * TICKS_PER_SECOND;
}

} // namespace game

// ============================================================================
// 网络常量
// ============================================================================

namespace network {

// 协议版本 (MC 1.16.5 = 754)
// 注意：当前项目目标是 MC 1.16.5，协议版本应为 754
constexpr i32 PROTOCOL_VERSION = 754; // MC 1.16.5

// 端口 (Java版)
constexpr u16 DEFAULT_PORT = 25565;      // Java版默认端口
constexpr u16 DEFAULT_RCON_PORT = 25575; // RCON 默认端口

// 数据包限制
constexpr Size MAX_PACKET_SIZE = 2097152; // 2MB
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
// 高度限制
constexpr i32 MIN_BUILD_HEIGHT = 0;
constexpr i32 MAX_BUILD_HEIGHT = 256;

// 区块尺寸
constexpr i32 CHUNK_WIDTH = 16;
constexpr i32 CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT;
constexpr i32 CHUNK_SECTION_HEIGHT = 16;
constexpr i32 CHUNK_SECTIONS = CHUNK_HEIGHT / CHUNK_SECTION_HEIGHT;
constexpr i32 CHUNK_VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;

// 区块块尺寸（区块内的位偏移）
constexpr i32 CHUNK_SHIFT = 4; // log2(16) = 4
constexpr i32 SECTION_SHIFT = 4;
constexpr i32 CHUNK_MASK = CHUNK_WIDTH - 1;

// 海平面高度 (MC 1.16.5 DimensionSettings.java:114)
constexpr i32 SEA_LEVEL = 63;

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

// 实体状态
enum class EntityStatus : u8 { Valid = 0, Dead = 1, Removed = 2 };

// 追踪距离 (单位：区块)
// 注意：MC 1.16.5 中追踪距离在 EntityType 中定义，每个实体类型不同
// 这里保留一些常用的默认值供参考
// 玩家追踪距离：32 区块 = 512 格
// 普通生物追踪距离：8-10 区块 = 128-160 格
// 物品追踪距离：6 区块 = 96 格
constexpr i32 DEFAULT_ENTITY_TRACKING_RANGE_CHUNKS = 8; // 默认实体追踪距离（区块）
constexpr i32 PLAYER_TRACKING_RANGE_CHUNKS = 32;        // 玩家追踪距离（区块）

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
