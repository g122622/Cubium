#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <optional>

namespace mc {

// ============================================================================
// 基础类型定义
// ============================================================================

// 整数类型
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// 浮点类型
using f32 = float;
using f64 = double;

static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

// 字符类型
using char8 = char;
using char16 = char16_t;
using char32 = char32_t;

// 字符串类型
using String = std::string;
using StringView = std::string_view;

// 尺寸类型
using Size = std::size_t;
using PtrDiff = std::ptrdiff_t;

// ============================================================================
// 向量类型（前向声明）
// ============================================================================

// 简单向量类型，用于命令系统等
// TODO 全部移除，统一使用Z:\mc_dev\branch1\minecraft-reborn\src\common\util\math\Vector3.hpp中的Vector3<T>模板类
struct Vector3i {
    i32 x, y, z;
    Vector3i() : x(0), y(0), z(0) {}
    Vector3i(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {}
    bool operator==(const Vector3i& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vector3i& other) const { return !(*this == other); }
};

struct Vector3d {
    f64 x, y, z;
    Vector3d() : x(0), y(0), z(0) {}
    Vector3d(f64 x, f64 y, f64 z) : x(x), y(y), z(z) {}
    bool operator==(const Vector3d& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vector3d& other) const { return !(*this == other); }
    Vector3d operator-(const Vector3d& other) const { return Vector3d(x - other.x, y - other.y, z - other.z); }
};

struct Vector3f {
    f32 x, y, z;
    Vector3f() : x(0), y(0), z(0) {}
    Vector3f(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    bool operator==(const Vector3f& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vector3f& other) const { return !(*this == other); }
};

struct Vector2f {
    f32 x, y;
    Vector2f() : x(0), y(0) {}
    Vector2f(f32 x, f32 y) : x(x), y(y) {}
    bool operator==(const Vector2f& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vector2f& other) const { return !(*this == other); }
};

// ============================================================================
// 游戏特定类型
// ============================================================================

// 区块坐标类型
using ChunkCoord = i32;

// 方块坐标类型（区块内）
using BlockCoord = i32;

// 世界高度
using WorldHeight = i32;

// 实体ID类型
using EntityId = u64;

// 无效实体ID常量
inline constexpr EntityId INVALID_ENTITY_ID = 0;

// 物品ID类型
using ItemId = u16;

// 生物群系ID类型
using BiomeId = u16;

// 维度ID类型
using DimensionId = i32;

// 玩家ID类型
using PlayerId = u64;

// ============================================================================
// 游戏常量
// ============================================================================

namespace constants {

// 区块尺寸
inline constexpr i32 CHUNK_WIDTH = 16;
inline constexpr i32 CHUNK_HEIGHT = 256;
inline constexpr i32 CHUNK_SECTION_HEIGHT = 16;
inline constexpr i32 CHUNK_SECTIONS = CHUNK_HEIGHT / CHUNK_SECTION_HEIGHT;

// 世界边界
inline constexpr i32 MIN_BUILD_HEIGHT = 0;
inline constexpr i32 MAX_BUILD_HEIGHT = CHUNK_HEIGHT;

// 方块状态
inline constexpr u16 MAX_BLOCK_STATES = 16;

// 实体限制
inline constexpr Size MAX_ENTITIES_PER_CHUNK = 1024;
inline constexpr Size MAX_PLAYERS = 256;

// 网络
inline constexpr u16 DEFAULT_PORT = 19132;
inline constexpr Size MAX_PACKET_SIZE = 2 * 1024 * 1024; // 2MB
inline constexpr Size PACKET_BUFFER_SIZE = 64 * 1024;    // 64KB

// 游戏刻
inline constexpr f32 TICK_RATE = 20.0f;          // 每秒20刻
inline constexpr f32 TICK_DURATION = 1.0f / 20.0f; // 每刻50ms

} // namespace constants

// ============================================================================
// 枚举类型
// ============================================================================

// 注意: DimensionType 类在 world/dimension/DimensionType.hpp 中定义
// DimensionId 类型别名已定义，用于标识维度

/**
 * @brief 游戏模式
 */
enum class GameMode : u8 {
    Survival = 0,
    Creative = 1,
    Adventure = 2,
    Spectator = 3,
    NotSet = 255    // 未设置（用于命令等场景）
};

/**
 * @brief 难度等级
 */
enum class Difficulty : u8 {
    Peaceful = 0,
    Easy = 1,
    Normal = 2,
    Hard = 3
};

/**
 * @brief 方块朝向
 */
enum class BlockFace : u8 {
    Bottom = 0, // Y-
    Top = 1,    // Y+
    North = 2,  // Z-
    South = 3,  // Z+
    West = 4,   // X-
    East = 5    // X+
};

/**
 * @brief 方块形状（用于碰撞）
 */
enum class BlockShape : u8 {
    Empty = 0,
    Full = 1,
    Partial = 2,
    Custom = 3
};

/**
 * @brief 手部（用于交互）
 *
 * 参考 MC 1.16.5 Hand
 */
enum class Hand : u8 {
    MainHand = 0,   ///< 主手
    OffHand = 1     ///< 副手
};

/**
 * @brief 手侧（左/右手）
 *
 * 表示玩家实际使用的手侧（主手设置）。
 * 与 Hand 的区别：Hand 表示主手或副手槽位，
 * HandSide 表示物理的左/右手。
 *
 * 参考 MC 1.16.5 HandSide
 */
enum class HandSide : u8 {
    Left = 0,   ///< 左手
    Right = 1   ///< 右手
};

// ============================================================================
// 方块ID类型
// ============================================================================
//
// 方块ID现在通过 BlockRegistry 动态分配，不再使用固定枚举。
// 使用 Block* 指针或 ResourceLocation 来引用方块。
// 参考 MC 1.16.5 的 Registry.register() 模式。
//
// 地形生成等场景应使用 VanillaBlocks::STONE 等静态指针，
// 或通过 BlockRegistry::getBlock(ResourceLocation) 获取方块。
// ============================================================================

} // namespace mc
