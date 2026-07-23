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

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

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

// 尺寸类型
using Size = std::size_t;
using PtrDiff = std::ptrdiff_t;

// ============================================================================
// 向量类型（前向声明）
// ============================================================================
// Vector2 和 Vector3 模板类定义在 src/common/util/math/Vector2.hpp 和 Vector3.hpp
// 完整定义需要包含对应的头文件

// 前向声明
namespace math {
template <typename T>
class Vector2;
template <typename T>
class Vector3;
} // namespace math

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
using EntityInstanceId = u64;

// 无效实体ID常量
inline constexpr EntityInstanceId INVALID_ENTITY_ID = 0;

// 物品ID类型
// 网络层按 VarInt 编码，u32 容量足够且与注册表分配的下标类型一致。
using ItemId = u32;

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

// 注意：区块尺寸常量已移至 mc::world 命名空间 (Constants.hpp)
// 这里保留向后兼容的别名，新代码应使用 mc::world 中的定义

// 方块状态
inline constexpr u16 MAX_BLOCK_STATES = 16;

// 游戏刻
inline constexpr f32 TICK_RATE = 20.0f;            // 每秒20刻
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
    NotSet = 255 // 未设置（用于命令等场景）
};

/**
 * @brief 难度等级
 */
enum class Difficulty : u8 { Peaceful = 0, Easy = 1, Normal = 2, Hard = 3 };

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
enum class BlockShape : u8 { Empty = 0, Full = 1, Partial = 2, Custom = 3 };

/**
 * @brief 手部（用于交互）
 *
 * 参考 MC 1.16.5 Hand
 */
enum class Hand : u8 {
    MainHand = 0, ///< 主手
    OffHand = 1   ///< 副手
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
    Left = 0, ///< 左手
    Right = 1 ///< 右手
};

/**
 * @brief 生物属性类型
 *
 * 用于附魔（如亡灵杀手、节肢杀手）对特定生物类型造成额外伤害。
 *
 * 参考 MC 1.16.5 CreatureAttribute
 */
enum class CreatureAttribute : u8 {
    Undefined = 0, ///< 未定义
    Undead = 1,    ///< 亡灵（僵尸、骷髅、凋灵等）
    Arthropod = 2, ///< 节肢动物（蜘蛛、末影螨、蠹虫等）
    Illager = 3,   ///< 灾厄村民（掠夺者、唤魔者等）
    Water = 4      ///< 水生生物（守卫者、鱿鱼等）
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
