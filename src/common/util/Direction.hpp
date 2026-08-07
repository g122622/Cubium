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

#include "../core/Types.hpp"
#include "math/MathUtils.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mc {

/**
 * @brief 方向枚举
 *
 * 表示六个基本方向，参考 net.minecraft.util.Direction
 *
 * 索引顺序: DOWN=0, UP=1, NORTH=2, SOUTH=3, WEST=4, EAST=5
 * 水平顺序: SOUTH=0, WEST=1, NORTH=2, EAST=3
 */
enum class Direction : u8 {
    Down = 0,  // Y- (下)
    Up = 1,    // Y+ (上)
    North = 2, // Z- (北)
    South = 3, // Z+ (南)
    West = 4,  // X- (西)
    East = 5,  // X+ (东)
    None = 255 // 无方向/无效
};

/**
 * @brief 坐标轴枚举
 */
enum class Axis : u8 { X = 0, Y = 1, Z = 2 };

/**
 * @brief 旋转枚举（用于结构旋转）
 *
 * 参考: net.minecraft.util.Rotation
 */
enum class Rotation : u8 {
    None = 0,              ///< 无旋转
    Clockwise90 = 1,       ///< 顺时针90度
    Clockwise180 = 2,      ///< 180度
    CounterClockwise90 = 3 ///< 逆时针90度（顺时针270度）
};

/**
 * @brief 镜像枚举（用于结构镜像）
 *
 * 参考: net.minecraft.util.Mirror
 */
enum class Mirror : u8 {
    None = 0,      ///< 无镜像
    LeftRight = 1, ///< 左右镜像（X轴）
    FrontBack = 2  ///< 前后镜像（Z轴）
};

/**
 * @brief 轴方向枚举
 */
enum class AxisDirection : u8 { Positive = 0, Negative = 1 };

/**
 * @brief Rotation工具函数
 */
namespace Rotations {
/**
 * @brief 将旋转枚举转换为角度
 */
constexpr i32 toDegrees(Rotation rot)
{
    switch (rot) {
        case Rotation::None:
            return 0;
        case Rotation::Clockwise90:
            return 90;
        case Rotation::Clockwise180:
            return 180;
        case Rotation::CounterClockwise90:
            return 270;
    }
    return 0;
}

/**
 * @brief 旋转角度相加
 */
inline Rotation add(Rotation a, Rotation b)
{
    i32 sum = static_cast<i32>(a) + static_cast<i32>(b);
    return static_cast<Rotation>(sum % 4);
}

/**
 * @brief 获取旋转的逆
 */
constexpr Rotation getInverse(Rotation rot)
{
    switch (rot) {
        case Rotation::None:
            return Rotation::None;
        case Rotation::Clockwise90:
            return Rotation::CounterClockwise90;
        case Rotation::Clockwise180:
            return Rotation::Clockwise180;
        case Rotation::CounterClockwise90:
            return Rotation::Clockwise90;
    }
    return Rotation::None;
}
} // namespace Rotations

/**
 * @brief Mirror工具函数
 */
namespace Mirrors {
/**
 * @brief 获取镜像的逆（镜像的逆是自身）
 */
constexpr Mirror getInverse(Mirror mir)
{
    return mir; // 镜像是自逆的
}
} // namespace Mirrors

/**
 * @brief Direction工具函数
 */
namespace Directions {
constexpr size_t COUNT = 6;
constexpr size_t HORIZONTAL_COUNT = 4;

/**
 * @brief 获取所有方向
 */
inline std::array<Direction, 6> all()
{
    return {Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
}

/**
 * @brief 获取所有水平方向 (NORTH, EAST, SOUTH, WEST)
 */
inline std::array<Direction, 4> horizontal()
{
    return {Direction::North, Direction::East, Direction::South, Direction::West};
}

/**
 * @brief 获取相反方向
 * @param dir 方向
 * @return 相反方向，如果Direction::None则返回Direction::None
 */
inline Direction opposite(Direction dir)
{
    // 处理 Direction::None 或无效值
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) {
        return Direction::None;
    }
    const Direction opposites[] = {
        Direction::Up,    // Down -> Up
        Direction::Down,  // Up -> Down
        Direction::South, // North -> South
        Direction::North, // South -> North
        Direction::East,  // West -> East
        Direction::West   // East -> West
    };
    return opposites[idx];
}

/**
 * @brief 获取方向的X偏移
 * @return X偏移，无效方向返回0
 */
inline i32 xOffset(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) return 0;
    const i32 offsets[] = {0, 0, 0, 0, -1, 1};
    return offsets[idx];
}

/**
 * @brief 获取方向的Y偏移
 * @return Y偏移，无效方向返回0
 */
inline i32 yOffset(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) return 0;
    const i32 offsets[] = {-1, 1, 0, 0, 0, 0};
    return offsets[idx];
}

/**
 * @brief 获取方向的Z偏移
 * @return Z偏移，无效方向返回0
 */
inline i32 zOffset(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) return 0;
    const i32 offsets[] = {0, 0, -1, 1, 0, 0};
    return offsets[idx];
}

/**
 * @brief 获取方向的坐标轴
 * @return 坐标轴，无效方向返回Axis::Y
 */
inline Axis getAxis(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) return Axis::Y;
    const Axis axes[] = {
        Axis::Y,
        Axis::Y, // Down, Up
        Axis::Z,
        Axis::Z, // North, South
        Axis::X,
        Axis::X // West, East
    };
    return axes[idx];
}

/**
 * @brief 获取方向的轴方向
 * @return 轴方向，无效方向返回AxisDirection::Positive
 */
inline AxisDirection getAxisDirection(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) return AxisDirection::Positive;
    const AxisDirection dirs[] = {
        AxisDirection::Negative,
        AxisDirection::Positive, // Down, Up
        AxisDirection::Negative,
        AxisDirection::Positive, // North, South
        AxisDirection::Negative,
        AxisDirection::Positive // West, East
    };
    return dirs[idx];
}

/**
 * @brief 判断方向是否水平
 */
inline bool isHorizontal(Direction dir)
{
    return dir == Direction::North || dir == Direction::South || dir == Direction::West || dir == Direction::East;
}

/**
 * @brief 判断方向是否垂直
 */
inline bool isVertical(Direction dir)
{
    return dir == Direction::Up || dir == Direction::Down;
}

/**
 * @brief 顺时针旋转 (仅水平方向)
 */
inline Direction rotateY(Direction dir)
{
    if (dir == Direction::North) return Direction::East;
    if (dir == Direction::East) return Direction::South;
    if (dir == Direction::South) return Direction::West;
    if (dir == Direction::West) return Direction::North;
    return dir;
}

/**
 * @brief 逆时针旋转 (仅水平方向)
 */
inline Direction rotateYCCW(Direction dir)
{
    if (dir == Direction::North) return Direction::West;
    if (dir == Direction::West) return Direction::South;
    if (dir == Direction::South) return Direction::East;
    if (dir == Direction::East) return Direction::North;
    return dir;
}

/**
 * @brief 从名称获取方向
 */
inline std::optional<Direction> fromName(std::string_view name)
{
    static const std::unordered_map<std::string, Direction> nameMap = {{"down", Direction::Down},
        {"up", Direction::Up},
        {"north", Direction::North},
        {"south", Direction::South},
        {"west", Direction::West},
        {"east", Direction::East}};
    auto it = nameMap.find(std::string(name));
    return it != nameMap.end() ? std::optional<Direction>(it->second) : std::nullopt;
}

/**
 * @brief 获取方向名称
 */
inline std::string toString(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    if (idx >= 6) return "none";
    const char* names[] = {"down", "up", "north", "south", "west", "east"};
    return names[idx];
}

/**
 * @brief 获取方向索引 (0-5)
 * @return 索引值，Direction::None 返回 255
 */
inline size_t index(Direction dir)
{
    return static_cast<size_t>(dir);
}

/**
 * @brief 检查方向是否有效
 */
inline bool isValid(Direction dir)
{
    const size_t idx = static_cast<size_t>(dir);
    return idx < 6;
}

/**
 * @brief 从索引获取方向
 */
inline Direction fromIndex(size_t idx)
{
    return static_cast<Direction>(idx % 6);
}

/**
 * @brief 将Direction转换为BlockFace
 */
inline BlockFace toBlockFace(Direction dir)
{
    switch (dir) {
        case Direction::Down:
            return BlockFace::Bottom;
        case Direction::Up:
            return BlockFace::Top;
        case Direction::North:
            return BlockFace::North;
        case Direction::South:
            return BlockFace::South;
        case Direction::West:
            return BlockFace::West;
        case Direction::East:
            return BlockFace::East;
        default:
            return BlockFace::Bottom;
    }
}

/**
 * @brief 从轴向和轴方向获取方向
 */
inline Direction fromAxisAndDirection(Axis axis, AxisDirection axisDir)
{
    if (axis == Axis::X) {
        return axisDir == AxisDirection::Positive ? Direction::East : Direction::West;
    } else if (axis == Axis::Y) {
        return axisDir == AxisDirection::Positive ? Direction::Up : Direction::Down;
    } else {
        return axisDir == AxisDirection::Positive ? Direction::South : Direction::North;
    }
}

/**
 * @brief 从向量获取方向
 */
inline Direction fromVector(f32 x, f32 y, f32 z)
{
    f32 maxDot = -1.0f;
    Direction result = Direction::North;

    const Direction dirs[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    const i32 xOffs[] = {0, 0, 0, 0, -1, 1};
    const i32 yOffs[] = {-1, 1, 0, 0, 0, 0};
    const i32 zOffs[] = {0, 0, -1, 1, 0, 0};

    for (size_t i = 0; i < 6; ++i) {
        f32 dot = x * static_cast<f32>(xOffs[i]) + y * static_cast<f32>(yOffs[i]) + z * static_cast<f32>(zOffs[i]);
        if (dot > maxDot) {
            maxDot = dot;
            result = dirs[i];
        }
    }
    return result;
}

/**
 * @brief 从增量获取方向
 *
 * @param dx X增量 (-1, 0, 1)
 * @param dy Y增量 (-1, 0, 1)
 * @param dz Z增量 (-1, 0, 1)
 * @return 对应的方向，如果无效返回Direction::None
 */
inline Direction fromDelta(i32 dx, i32 dy, i32 dz)
{
    // 检查值是否在有效范围内
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || dz < -1 || dz > 1) {
        return Direction::None;
    }
    if (dy < 0 && dx == 0 && dz == 0) return Direction::Down;
    if (dy > 0 && dx == 0 && dz == 0) return Direction::Up;
    if (dz < 0 && dx == 0 && dy == 0) return Direction::North;
    if (dz > 0 && dx == 0 && dy == 0) return Direction::South;
    if (dx < 0 && dy == 0 && dz == 0) return Direction::West;
    if (dx > 0 && dy == 0 && dz == 0) return Direction::East;
    return Direction::None;
}

/**
 * @brief 判断实体的偏航角是否大致朝向该方向
 *
 * 将偏航角转换为视线方向向量，与该方向的法向量做点积。
 * 点积 > 0 表示实体的视线方向与该方向同侧。
 *
 * @param dir 水平方向（仅对水平方向有效）
 * @param yaw 实体的偏航角（度），MC 约定：0=南，90=西，180=北，-90=东
 * @return true 如果实体大致朝向该方向
 */
inline bool isFacingAngle(Direction dir, f32 yaw)
{
    if (!isHorizontal(dir)) {
        return false;
    }
    // 将偏航角转换为弧度，计算视线方向向量
    f32 rad = mc::math::toRadians(yaw);
    f32 sinYaw = -std::sin(rad);
    f32 cosYaw = std::cos(rad);
    // 方向的法向量与视线方向向量的点积
    return static_cast<f32>(xOffset(dir)) * sinYaw + static_cast<f32>(zOffset(dir)) * cosYaw > 0.0f;
}

/**
 * @brief 将水平方向转换为实体偏航角（yaw）
 *
 * 对应 MC Java 1.21.11 `Direction.toYRot()`，用于命令方块/命令方块矿车等
 * 无实体朝向的命令源执行 `^` 局部坐标命令时构造 CommandSourceStack 的 rotation。
 * MC 约定：0=南，90=西，180=北，-90=东（与 isFacingAngle 注释一致）。
 *
 * @param dir 水平方向（DOWN/UP 返回 0，命令方块 facing 不会是垂直方向）
 * @return 偏航角（度）
 */
inline f32 toYRot(Direction dir)
{
    switch (dir) {
        case Direction::North:
            return 180.0f;
        case Direction::South:
            return 0.0f;
        case Direction::West:
            return 90.0f;
        case Direction::East:
            return -90.0f;
        default:
            // DOWN/UP：vanilla 命令方块 facing 不会是垂直方向，返回 0 作兜底
            return 0.0f;
    }
}

/**
 * @brief 旋转方向
 *
 * 根据旋转类型旋转水平方向。
 *
 * @param dir 方向（仅水平方向有效）
 * @param rotation 旋转类型
 * @return 旋转后的方向
 */
inline Direction rotateDirection(Direction dir, Rotation rotation)
{
    if (!isHorizontal(dir)) {
        return dir;
    }

    switch (rotation) {
        case Rotation::None:
            return dir;
        case Rotation::Clockwise90:
            return rotateY(dir);
        case Rotation::Clockwise180:
            return opposite(dir);
        case Rotation::CounterClockwise90:
            return rotateYCCW(dir);
        default:
            return dir;
    }
}

/**
 * @brief 将镜像转换为旋转
 *
 * 根据镜像类型和原始朝向计算旋转角度。
 *
 * @param mirror 镜像类型
 * @param dir 原始朝向
 * @return 旋转类型
 */
inline Rotation mirrorToRotation(Mirror mirror, Direction dir)
{
    switch (mirror) {
        case Mirror::LeftRight:
            // 左右镜像：南北朝向需要180度旋转
            if (dir == Direction::North || dir == Direction::South) {
                return Rotation::Clockwise180;
            }
            return Rotation::None;
        case Mirror::FrontBack:
            // 前后镜像：东西朝向需要180度旋转
            if (dir == Direction::East || dir == Direction::West) {
                return Rotation::Clockwise180;
            }
            return Rotation::None;
        case Mirror::None:
        default:
            return Rotation::None;
    }
}

/**
 * @brief 旋转整数旋转值
 *
 * 用于告示牌、旗帜等具有16个旋转值的方块。
 *
 * @param rotation 当前旋转值 (0-15)
 * @param rot 旋转类型
 * @param max 最大旋转值 (默认16)
 * @return 旋转后的旋转值
 */
inline i32 rotateRotation(i32 rotation, Rotation rot, i32 max = 16)
{
    switch (rot) {
        case Rotation::Clockwise90:
            return (rotation + 4) % max;
        case Rotation::Clockwise180:
            return (rotation + 8) % max;
        case Rotation::CounterClockwise90:
            return (rotation + max - 4) % max;
        case Rotation::None:
        default:
            return rotation;
    }
}

/**
 * @brief 镜像整数旋转值
 *
 * 用于告示牌、旗帜等具有16个旋转值的方块。
 *
 * @param rotation 当前旋转值 (0-15)
 * @param mirror 镜像类型
 * @param max 最大旋转值 (默认16)
 * @return 镜像后的旋转值
 */
inline i32 mirrorRotation(i32 rotation, Mirror mirror, i32 max = 16)
{
    switch (mirror) {
        case Mirror::LeftRight:
            // 左右镜像：关于南北轴镜像
            // 0->0, 1->15, 2->14, ..., 7->9, 8->8
            return (max - rotation) % max;
        case Mirror::FrontBack:
            // 前后镜像：关于东西轴镜像
            // 4->4, 5->3, 6->2, ..., 12->12, 13->11
            return ((max / 2) - rotation + max) % max;
        case Mirror::None:
        default:
            return rotation;
    }
}
} // namespace Directions

/**
 * @brief Axis工具函数
 */
namespace Axes {
constexpr size_t COUNT = 3;

inline std::array<Axis, 3> all()
{
    return {Axis::X, Axis::Y, Axis::Z};
}

inline std::optional<Axis> fromName(std::string_view name)
{
    if (name == "x") return Axis::X;
    if (name == "y") return Axis::Y;
    if (name == "z") return Axis::Z;
    return std::nullopt;
}

inline std::string toString(Axis axis)
{
    const char* names[] = {"x", "y", "z"};
    return names[static_cast<size_t>(axis)];
}

inline bool isHorizontal(Axis axis)
{
    return axis == Axis::X || axis == Axis::Z;
}

inline bool isVertical(Axis axis)
{
    return axis == Axis::Y;
}
} // namespace Axes

/**
 * @brief 轴循环枚举
 *
 * 用于在 X、Y、Z 轴之间循环变换坐标。
 * 参考 MC AxisCycle。
 *
 * NONE: X -> X, Y -> Y, Z -> Z (无变换)
 * FORWARD: X -> Y, Y -> Z, Z -> X (向前循环)
 * BACKWARD: X -> Z, Y -> X, Z -> Y (向后循环)
 */
enum class AxisCycle : u8 {
    NONE = 0,    ///< 无变换
    FORWARD = 1, ///< 向前循环 X->Y->Z->X
    BACKWARD = 2 ///< 向后循环 X->Z->Y->X
};

/**
 * @brief AxisCycle工具函数
 */
namespace AxisCycles {
constexpr size_t COUNT = 3;

/**
 * @brief 获取所有轴循环
 */
inline std::array<AxisCycle, 3> all()
{
    return {AxisCycle::NONE, AxisCycle::FORWARD, AxisCycle::BACKWARD};
}

/**
 * @brief 循环轴
 * @param cycle 循环类型
 * @param axis 输入轴
 * @return 循环后的轴
 */
inline Axis cycle(AxisCycle cycle, Axis axis)
{
    const size_t idx = static_cast<size_t>(axis);
    switch (cycle) {
        case AxisCycle::NONE:
            return axis;
        case AxisCycle::FORWARD:
            return static_cast<Axis>((idx + 1) % 3);
        case AxisCycle::BACKWARD:
            return static_cast<Axis>((idx + 2) % 3);
    }
    return axis;
}

/**
 * @brief 循环坐标
 * @param cycle 循环类型
 * @param x X坐标
 * @param y Y坐标
 * @param z Z坐标
 * @param axis 目标轴
 * @return 在目标轴上的坐标值
 */
inline i32 cycle(AxisCycle cycle, i32 x, i32 y, i32 z, Axis axis)
{
    switch (cycle) {
        case AxisCycle::NONE:
            return (axis == Axis::X) ? x : (axis == Axis::Y) ? y : z;
        case AxisCycle::FORWARD:
            // X->Y, Y->Z, Z->X
            if (axis == Axis::X) return z;
            if (axis == Axis::Y) return x;
            return y;
        case AxisCycle::BACKWARD:
            // X->Z, Y->X, Z->Y
            if (axis == Axis::X) return y;
            if (axis == Axis::Y) return z;
            return x;
    }
    return 0;
}

/**
 * @brief 获取逆向循环
 */
inline AxisCycle inverse(AxisCycle cycle)
{
    switch (cycle) {
        case AxisCycle::NONE:
            return AxisCycle::NONE;
        case AxisCycle::FORWARD:
            return AxisCycle::BACKWARD;
        case AxisCycle::BACKWARD:
            return AxisCycle::FORWARD;
    }
    return AxisCycle::NONE;
}

/**
 * @brief 获取两个轴之间的循环
 * @param from 源轴
 * @param to 目标轴
 * @return 循环类型
 */
inline AxisCycle between(Axis from, Axis to)
{
    const size_t fromIdx = static_cast<size_t>(from);
    const size_t toIdx = static_cast<size_t>(to);

    if (fromIdx == toIdx) {
        return AxisCycle::NONE;
    }

    // 计算循环步数
    const size_t diff = (toIdx + 3 - fromIdx) % 3;
    if (diff == 1) {
        return AxisCycle::FORWARD;
    } else {
        return AxisCycle::BACKWARD;
    }
}
} // namespace AxisCycles

/**
 * @brief Direction扩展方法
 *
 * 提供类似 Java Direction 类的成员方法。
 */
inline Direction getOpposite(Direction dir)
{
    return Directions::opposite(dir);
}

inline Axis getAxis(Direction dir)
{
    return Directions::getAxis(dir);
}

inline AxisDirection getAxisDirection(Direction dir)
{
    return Directions::getAxisDirection(dir);
}

inline i32 getStepX(Direction dir)
{
    return Directions::xOffset(dir);
}

inline i32 getStepY(Direction dir)
{
    return Directions::yOffset(dir);
}

inline i32 getStepZ(Direction dir)
{
    return Directions::zOffset(dir);
}

inline size_t ordinal(Direction dir)
{
    return static_cast<size_t>(dir);
}

/**
 * @brief 获取近似最近的方向
 */
inline Direction getApproximateNearest(f32 x, f32 y, f32 z)
{
    return Directions::fromVector(x, y, z);
}

} // namespace mc

// 为 Direction 枚举添加成员方法风格的访问
namespace mc {

/**
 * @brief Direction 扩展方法包装器
 *
 * 允许使用 dir.getOpposite() 风格的调用
 */
struct DirectionExt {
    static Direction getOpposite(Direction dir) { return Directions::opposite(dir); }
    static Axis getAxis(Direction dir) { return Directions::getAxis(dir); }
    static AxisDirection getAxisDirection(Direction dir) { return Directions::getAxisDirection(dir); }
    static i32 getStepX(Direction dir) { return Directions::xOffset(dir); }
    static i32 getStepY(Direction dir) { return Directions::yOffset(dir); }
    static i32 getStepZ(Direction dir) { return Directions::zOffset(dir); }
    static size_t ordinal(Direction dir) { return static_cast<size_t>(dir); }
};

} // namespace mc
