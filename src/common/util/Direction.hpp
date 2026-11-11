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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "math/MathUtils.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
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
 * @brief 轴向平面，对应原版 `Direction.Plane`
 *
 * 把六个方向按面分成水平与垂直两组：
 *
 *   HORIZONTAL = {NORTH, EAST, SOUTH, WEST}，对应轴 {X, Z}，长度 4
 *   VERTICAL   = {UP, DOWN}，对应轴 {Y}，长度 2
 *
 * 水平面的顺序是 NORTH→EAST→SOUTH→WEST（顺时针一圈），
 * 不是 NORTH→SOUTH→WEST→EAST。该顺序在按平面随机取方向时会影响结果。
 */
enum class Plane : u8 { Horizontal = 0, Vertical = 1 };

/**
 * @brief 旋转枚举（用于结构旋转）
 *
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
 */
enum class Mirror : u8 {
    None = 0,      ///< 无镜像
    LeftRight = 1, ///< 左右镜像：沿 Z 轴翻转（南北互换），对应原版 `INVERT_Z`
    FrontBack = 2  ///< 前后镜像：沿 X 轴翻转（东西互换），对应原版 `INVERT_X`
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
 * @brief 把另一个旋转叠加到本旋转上，对应原版 `Rotation.getRotated`
 *
 * 本质是四元群的加法：四个旋转映射到 0/1/2/3，相加后对 4 取模。
 * 原版用四层 switch 手工写死 16 种组合，这里用取模得到同样结果——
 * **枚举值必须保持 0/1/2/3 的顺序**，否则这个等价关系不成立。
 */
inline Rotation getRotated(Rotation self, Rotation other)
{
    return add(self, other);
}

/**
 * @brief 旋转一个环形索引，对应原版 `Rotation.rotate(int, int)`
 *
 * 注意用的是 **C++ 的 `%`（向零截断）而不是 `positiveModulo`**——原版这里就是
 * 普通 `%`，因此值为负时结果也可能为负。另外 `modulus * 3 / 4` 是**先乘后除**，
 * 不能写成 `modulus / 4 * 3`（对 5、6 这类模数结果不同）。
 */
inline i32 rotate(Rotation rot, i32 value, i32 modulus)
{
    switch (rot) {
        case Rotation::Clockwise90:
            return (value + modulus / 4) % modulus;
        case Rotation::Clockwise180:
            return (value + modulus / 2) % modulus;
        case Rotation::CounterClockwise90:
            return (value + modulus * 3 / 4) % modulus;
        default:
            return value;
    }
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
    if (idx >= 6) {
        return 0;
    }
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
    if (idx >= 6) {
        return 0;
    }
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
    if (idx >= 6) {
        return 0;
    }
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
    if (idx >= 6) {
        return Axis::Y;
    }
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
    if (idx >= 6) {
        return AxisDirection::Positive;
    }
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
 * @brief 获取某坐标轴的两个方向
 *
 * 对齐 vanilla net.minecraft.core.Direction.Axis#getDirections（返回轴的正负两向）。
 * 顺序与 AxisDirection 语义一致：[负向, 正向]。
 *
 * @param axis 坐标轴
 * @param outNeg 输出轴负方向
 * @param outPos 输出轴正方向
 */
inline void getAxisDirections(Axis axis, Direction& outNeg, Direction& outPos)
{
    switch (axis) {
        case Axis::X:
            outNeg = Direction::West;
            outPos = Direction::East;
            return;
        case Axis::Z:
            outNeg = Direction::North;
            outPos = Direction::South;
            return;
        case Axis::Y:
        default:
            outNeg = Direction::Down;
            outPos = Direction::Up;
            return;
    }
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
    if (dir == Direction::North) {
        return Direction::East;
    }
    if (dir == Direction::East) {
        return Direction::South;
    }
    if (dir == Direction::South) {
        return Direction::West;
    }
    if (dir == Direction::West) {
        return Direction::North;
    }
    return dir;
}

/**
 * @brief 水平面逆时针旋转，对应原版 `Direction.getCounterClockWise()`
 *
 * 竖直方向（Up/Down）原样返回——它们不在水平面内，没有顺/逆时针之分。
 */
inline Direction getCounterClockWise(Direction dir)
{
    if (dir == Direction::North) {
        return Direction::West;
    }
    if (dir == Direction::West) {
        return Direction::South;
    }
    if (dir == Direction::South) {
        return Direction::East;
    }
    if (dir == Direction::East) {
        return Direction::North;
    }
    return dir;
}

/**
 * @brief 水平面顺时针旋转的别名，对应原版 `Direction.getClockWise()`
 */
inline Direction getClockWise(Direction dir)
{
    return rotateY(dir);
}

/**
 * @brief 逆时针旋转 (仅水平方向)
 */
inline Direction rotateYCCW(Direction dir)
{
    if (dir == Direction::North) {
        return Direction::West;
    }
    if (dir == Direction::West) {
        return Direction::South;
    }
    if (dir == Direction::South) {
        return Direction::East;
    }
    if (dir == Direction::East) {
        return Direction::North;
    }
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
    if (idx >= 6) {
        return "none";
    }
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
    // 初值必须是 Java 的 Float.MIN_VALUE（1.4e-45，最小**次正规**正数），
    // 不是 C++ 的 numeric_limits<f32>::min()（1.175e-38，最小**正规**正数）。
    //
    // 两者差 7 个数量级：C++ 那一侧会让 1e-40 这类次正规分量的点积被判为「不够大」，
    // 从而错误地停在 North；Java 那侧则会正确选中对应方向。
    //
    // 零向量时所有点积都是 0，0 > 1.4e-45 为假，于是保持 North——这正是原版行为。
    constexpr f32 JAVA_FLOAT_MIN_VALUE = 1.401298464324817e-45f;
    f32 maxDot = JAVA_FLOAT_MIN_VALUE;
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
    if (dy < 0 && dx == 0 && dz == 0) {
        return Direction::Down;
    }
    if (dy > 0 && dx == 0 && dz == 0) {
        return Direction::Up;
    }
    if (dz < 0 && dx == 0 && dy == 0) {
        return Direction::North;
    }
    if (dz > 0 && dx == 0 && dy == 0) {
        return Direction::South;
    }
    if (dx < 0 && dy == 0 && dz == 0) {
        return Direction::West;
    }
    if (dx > 0 && dy == 0 && dz == 0) {
        return Direction::East;
    }
    return Direction::None;
}

/**
 * @brief 判断实体的偏航角是否大致朝向该方向
 *
 * 将偏航角转换为视线方向向量，与该方向的法向量做点积。
 * 点积 > 0 表示实体的视线方向与该方向同侧。
 *
 * @param yaw 实体的偏航角（度），MC 约定：0=南，90=西，180=北，-90（或 270）=东；
 *            视线单位向量为 (-sin yaw, 0, cos yaw)
 * @return true 如果实体大致朝向该方向
 */
inline bool isFacingAngle(Direction dir, f32 yaw)
{
    // 必须用查表的 mthSin / mthCos，不能换成 std::sin / std::cos。
    //
    // 查表在 90° 的整数倍上返回**精确**的 0 或 ±1，而 libm 会给出 ±4e-8 量级的残差：
    //
    //   yaw=90   Mth.cos = 0            std::cos = -4.4e-8
    //   yaw=180  Mth.sin = 0            std::sin = -8.7e-8
    //   yaw=270  Mth.cos = 0            std::cos = +1.2e-8
    //
    // 点积判定是 `> 0`，这些残差会让「正好正交」的情形翻成 true（或反过来）。
    // 原版走查表，所以 90° 倍数上一律得到 false。
    const f32 rad = math::toRadians(yaw);
    const f32 sinYaw = -math::mthSin(rad);
    const f32 cosYaw = math::mthCos(rad);
    return static_cast<f32>(xOffset(dir)) * sinYaw + static_cast<f32>(zOffset(dir)) * cosYaw > 0.0f;
}

/**
 * @brief 方向的 y-Rot（实体朝向角），对应原版 `Direction.getYRot(Direction)`
 *
 * **与 `toYRot` 不是同一个方法，EAST 的取值不同：**
 *
 * | 方向   | toYRot | getYRot |
 * |--------|--------|---------|
 * | NORTH  | 180    | 180     |
 * | SOUTH  | 0      | 0       |
 * | WEST   | 90     | 90      |
 * | EAST   | 270    | **-90** |
 * | DOWN/UP | 0     | **抛出/断言** |
 *
 * 270 与 -90 在角度上等价（差 360），但作为**返回值字面量**不同：
 * 写进 NBT、比较、作为数组下标时都会得到不同结果。
 *
 * `toYRot` 走的是 `(data2d & 3) * 90`，天然落在 [0, 270]；
 * `getYRot` 是手写 switch，EAST 取 -90。
 *
 * 垂直方向在原版会抛 `IllegalStateException`——调用方不该对 DOWN/UP 求 y-Rot。
 * 这里返回 0 并断言。
 */
[[nodiscard]] inline f32 getYRot(Direction dir)
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
            // 原版对垂直方向抛异常，调用方不该走到这里。
            MC_ASSERT_RELEASE_MSG(false, "getYRot: vertical direction has no y-rotation");
            return 0.0f;
    }
}

/**
 * @brief 三维数据值，对应原版 `Direction.get3DDataValue()`
 *
 * 原版的 data3d 与枚举序号完全一致（DOWN=0 … EAST=5），所以直接取枚举值即可。
 * None 不是合法方向，返回 -1。
 */
[[nodiscard]] inline i32 get3DDataValue(Direction dir)
{
    return dir == Direction::None ? -1 : static_cast<i32>(dir);
}

/**
 * @brief 二维数据值，对应原版 `Direction.get2DDataValue()`
 *
 * 天空光/朝向相关的水平编码：SOUTH=0、WEST=1、NORTH=2、EAST=3。
 *
 * **与三维数据值的顺序完全不同**——三维是 N=2/S=3/W=4/E=5，二维是 S=0/W=1/N=2/E=3。
 * 两者不能互相替代。垂直方向在二维编码中没有定义，返回 -1。
 */
[[nodiscard]] inline i32 get2DDataValue(Direction dir)
{
    switch (dir) {
        case Direction::South:
            return 0;
        case Direction::West:
            return 1;
        case Direction::North:
            return 2;
        case Direction::East:
            return 3;
        default:
            return -1;
    }
}

/**
 * @brief 将水平方向转换为实体偏航角（yaw）
 *
 * 对应 MC Java 1.21.11 `Direction.toYRot()`，用于命令方块/命令方块矿车等
 * 无实体朝向的命令源执行 `^` 局部坐标命令时构造 CommandSourceStack 的 rotation。
 * 视线单位向量为 (-sin yaw, 0, cos yaw)，故 yaw=0 朝南、90 朝西、180 朝北、270 朝东
 * （与 fromYRot 互逆，见其注释）。
 *
 * @param dir 水平方向（DOWN/UP 返回 0，命令方块 facing 不会是垂直方向）
 * @return 偏航角（度）
 */
inline f32 toYRot(Direction dir)
{
    // 原版实现是 (data2d & 3) * 90，data2d 为：DOWN/UP = -1、SOUTH = 0、WEST = 1、NORTH = 2、EAST = 3。
    //
    // 因此 DOWN/UP 得到 (-1 & 3) * 90 = 3 * 90 = 270，不是 0。
    // -1 在补码下全部 32 位都是 1，按低 2 位截取正好是 3。
    const i32 data2d = get2DDataValue(dir);
    return static_cast<f32>((data2d & 3) * 90);
}

/**
 * @brief 由三维数据值反查方向，对应原版 `Direction.from3DDataValue(int)`
 *
 * 原版实现是 `BY_3D_DATA[Mth.abs(v % BY_3D_DATA.length)]`——**取模环回**，
 * 不是越界返回默认值：
 *
 *   from3DDataValue(6)  → DOWN    （6 % 6 = 0）
 *   from3DDataValue(7)  → UP
 *   from3DDataValue(-1) → UP      （-1 % 6 = -1，abs 后为 1）
 *
 * 负值方向与正值**不对称**，因为 Java/C++ 的 `%` 结果符号跟随被除数，
 * `abs` 之后 -1 落到 1 而不是 5。这是原版的实际行为，不能「修正」成对称环回。
 */
[[nodiscard]] inline Direction from3DDataValue(i32 value)
{
    // 顺序即枚举顺序（data3d 与序号一致）。
    constexpr Direction BY_3D_DATA[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    return BY_3D_DATA[static_cast<size_t>(std::abs(value % 6))];
}

/**
 * @brief 由二维数据值反查水平方向，对应原版 `Direction.from2DDataValue(int)`
 *
 * 二维表**只含水平方向**，长度 4（顺序为 SOUTH=0、WEST=1、NORTH=2、EAST=3），
 * 因此取模用的是 4 而不是 6：
 *
 *   from2DDataValue(4) → SOUTH   （4 % 4 = 0）
 *   from2DDataValue(-1) → WEST   （-1 % 4 = -1，abs 后为 1）
 *
 * **永远返回水平方向**，不会返回 UP/DOWN。
 */
[[nodiscard]] inline Direction from2DDataValue(i32 value)
{
    constexpr Direction BY_2D_DATA[] = {Direction::South, Direction::West, Direction::North, Direction::East};
    return BY_2D_DATA[static_cast<size_t>(std::abs(value % 4))];
}

/**
 * @brief 方向的单位向量（整数），对应原版 `Direction.getUnitVec3i()`
 *
 * 六个方向各对应一个分量取 ±1、其余为 0 的向量。None 返回零向量。
 */
[[nodiscard]] inline Vector3i getUnitVec3i(Direction dir)
{
    return Vector3i(xOffset(dir), yOffset(dir), zOffset(dir));
}

/**
 * @brief 方向的单位向量（f32），对应原版 `Direction.getUnitVec3f()`
 *
 * 原版返回 JOML 的 `Vector3fc`，本项目的对应类型是 `Vector3f`。
 */
[[nodiscard]] inline Vector3f getUnitVec3f(Direction dir)
{
    return Vector3f(static_cast<f32>(xOffset(dir)), static_cast<f32>(yOffset(dir)), static_cast<f32>(zOffset(dir)));
}

/**
 * @brief 方向的单位向量（f64），对应原版 `Direction.getUnitVec3()`
 *
 * 原版返回 `Vec3`（f64 分量）；本项目的 `Vector3d` 与之对应。
 */
[[nodiscard]] inline Vector3d getUnitVec3(Direction dir)
{
    return Vector3d(static_cast<f64>(xOffset(dir)), static_cast<f64>(yOffset(dir)), static_cast<f64>(zOffset(dir)));
}

/**
 * @brief 按视线方向把六个方向从「最接近」到「最远」排序，对应原版 `Direction.orderedByNearest`
 *
 * 视线向量是 `(-sin yaw·cos pitch, -sin pitch, cos yaw·cos pitch)`。实现不直接算点积，
 * 而是先取三个轴上分量的**绝对值**（`f6`/`f7`/`f8`），再按大小关系分三种情况把
 * 「主轴方向 → 次轴方向 → 第三轴方向」排好，最后补上各自的反向。
 *
 * 返回值恒为 6 项，且第 i 项与第 5-i 项互为反向——这是 `makeDirectionArray` 的构造方式决定的。
 *
 * @param yawDeg 视线偏航角（度），0=南、90=西、180=北、270=东
 * @param pitchDeg 视线俯仰角（度），正=俯视、负=仰视
 */
[[nodiscard]] inline std::array<Direction, 6> orderedByNearest(f32 yawDeg, f32 pitchDeg)
{
    // 注意 f1 取负：原版 `-viewYRot`。yaw 的正负方向与「东为 +X」相反。
    const f32 pitchRad = pitchDeg * math::DEG_TO_RAD;
    const f32 yawRad = -yawDeg * math::DEG_TO_RAD;

    const f32 sinPitch = math::mthSin(pitchRad);
    const f32 cosPitch = math::mthCos(pitchRad);
    const f32 sinYaw = math::mthSin(yawRad);
    const f32 cosYaw = math::mthCos(yawRad);

    const bool eastward = sinYaw > 0.0f;
    const bool upward = sinPitch < 0.0f;
    const bool southward = cosYaw > 0.0f;

    // 取绝对值：比较的是各轴分量的大小，与正负无关。
    const f32 absSinYaw = eastward ? sinYaw : -sinYaw;
    const f32 absSinPitch = upward ? -sinPitch : sinPitch;
    const f32 absCosYaw = southward ? cosYaw : -cosYaw;

    // 水平分量还要乘上 cos(pitch)，才能与竖直分量在同一尺度上比较。
    const f32 horizontalEast = absSinYaw * cosPitch;
    const f32 horizontalSouth = absCosYaw * cosPitch;

    const Direction xDir = eastward ? Direction::East : Direction::West;
    const Direction yDir = upward ? Direction::Up : Direction::Down;
    const Direction zDir = southward ? Direction::South : Direction::North;

    const auto makeArray = [](Direction first, Direction second, Direction third) -> std::array<Direction, 6> {
        return {first, second, third, opposite(third), opposite(second), opposite(first)};
    };

    if (absSinYaw > absCosYaw) {
        // X 轴分量最大
        if (absSinPitch > horizontalEast) {
            return makeArray(yDir, xDir, zDir);
        }
        return horizontalSouth > absSinPitch ? makeArray(xDir, zDir, yDir) : makeArray(xDir, yDir, zDir);
    }
    // Z 轴分量最大
    if (absSinPitch > horizontalSouth) {
        return makeArray(yDir, zDir, xDir);
    }
    return horizontalEast > absSinPitch ? makeArray(zDir, xDir, yDir) : makeArray(zDir, yDir, xDir);
}

/**
 * @brief 按方向的轴取对应分量，对应原版 `Direction.Axis.choose(int, int, int)`
 *
 * 参数顺序是 (x, y, z)；取方向的轴再按轴选分量。例如 EAST 的轴是 X，返回 x。
 *
 * 注意原版的 `choose` 定义在
 * **`Direction.Axis`** 接口上，不是 `Direction` 的实例方法。
 */
[[nodiscard]] inline i32 choose(Direction dir, i32 x, i32 y, i32 z)
{
    switch (getAxis(dir)) {
        case Axis::X:
            return x;
        case Axis::Y:
            return y;
        case Axis::Z:
            return z;
        default:
            return x;
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
            // 左右镜像（沿 Z 轴）：0->8, 1->7, 2->6, ..., 8->0, 9->15, ...
            return ((max / 2) - rotation + max) % max;
        case Mirror::FrontBack:
            // 前后镜像（沿 X 轴）：0->0, 1->15, 2->14, ..., 7->9, 8->8
            return (max - rotation) % max;
        case Mirror::None:
        default:
            return rotation;
    }
}

/**
 * @brief 由偏航角取水平方向
 *
 * 对应 MC 1.21.11 `Direction.fromYRot(double)`：
 *   from2DDataValue(floor(yaw / 90 + 0.5) & 3)，其中 2D 数据值按 South=0、West=1、North=2、East=3
 * 循环（MC 的 2D 数据值是 Direction 枚举值的 (数据值 >> 1)，North=2→1… 即 South/Up/North/Down 之外
 * 的 West、East 分别落在 1 与 3）。视线单位向量为 (-sin yaw, 0, cos yaw)，因此 yaw=0 朝南、
 * yaw=90 朝西、yaw=180 朝北、yaw=270 朝东。
 *
 * 注意：旧实现把 1/3 当成 East/West，使 yaw=90 得 East、yaw=270 得 West——与视线方向正好相反，
 * 会让熔炉、织布机、切石机、中继器、按钮等所有「按玩家朝向决定 facing」的方块朝向整体翻转。
 * 非有限输入（NaN / ±inf）在强转整数时结果未定义，与原版对非法输入未做约定的状态一致；
 * 调用方须保证 yaw 为有限值（实体偏航角恒为有限值）。
 *
 * @param yaw 偏航角（度），MC 说明见 isFacingAngle
 * @return 水平方向
 */
inline Direction fromYRot(f64 yaw)
{
    // MC: Direction.from2DDataValue 取 2D 数据值，与 3D 枚举值的对应关系为：
    // 2D 的 South=0、West=1、North=2、East=3（2D 数据值是 3D 枚举值的一半取整）。
    // 结合 Direction 的 3D 值布局（Down=0、Up=1、North=2、South=3、West=4、East=5），
    // 2D 数据值 d 对应 3D 值：d=0→3(South)、d=1→4(West)、d=2→2(North)、d=3→5(East)。
    static constexpr i32 TWO_D_TO_THREE_D[4] = {3, 4, 2, 5};
    // 扇区下标 = floor(yaw / 90 + 0.5) & 3，与原版同式。
    //
    // 必须用 volatile 中转一次：`-ffast-math` 允许把 `yaw / 90.0` 改写成
    // `yaw * (1 / 90.0)`，而 1/90 无法用二进制精确表示。yaw = -45 时近似结果略小于
    // -0.5，加 0.5 后落到零的负侧，floor 得 -1，`-1 & 3` 变成 3（East）——原版是
    // South。yaw 为 45° 的奇数倍（±45、±135、±225、±315）全部会踩到，表现为「按玩家
    // 朝向放置的方块在斜 45° 时朝反方向」。
    //
    // volatile 让这次除法在 double 下精确完成、不被改写，并且不引入任何数值改动：
    // -45 / 90 精确等于 -0.5，加 0.5 精确等于 +0.0，floor 得 0。
    const volatile f64 quarter = yaw / 90.0;
    const i32 data = static_cast<i32>(std::floor(static_cast<f64>(quarter) + 0.5)) & 3;
    const i32 value = TWO_D_TO_THREE_D[data];
    return static_cast<Direction>(value);
}

/**
 * @brief 绕指定轴顺时针旋转 90°，对应原版 `Direction.getClockWise(Axis)`
 *
 * 方向位于该轴上时返回自身（绕自身轴旋转不变）；否则在该轴的垂直平面内转 90°。
 *
 *   绕 X：DOWN→SOUTH、UP→NORTH、NORTH→DOWN、SOUTH→UP
 *   绕 Y：NORTH→EAST、SOUTH→WEST、WEST→NORTH、EAST→SOUTH（等同 getClockWise(dir)）
 *   绕 Z：DOWN→WEST、UP→EAST、WEST→UP、EAST→DOWN
 *
 * 绕 X 与绕 Z 的映射并不对称：绕 X 是 DOWN→SOUTH，绕 Z 是 DOWN→WEST。
 */
[[nodiscard]] inline Direction getClockWise(Axis axis, Direction dir)
{
    switch (axis) {
        case Axis::X:
            switch (dir) {
                case Direction::Down:
                    return Direction::South;
                case Direction::Up:
                    return Direction::North;
                case Direction::North:
                    return Direction::Down;
                case Direction::South:
                    return Direction::Up;
                default:
                    return dir;
            }
        case Axis::Y:
            if (dir == Direction::Up || dir == Direction::Down) {
                return dir;
            }
            return getClockWise(dir);
        default:
            switch (dir) {
                case Direction::Down:
                    return Direction::West;
                case Direction::Up:
                    return Direction::East;
                case Direction::West:
                    return Direction::Up;
                case Direction::East:
                    return Direction::Down;
                default:
                    return dir;
            }
    }
}

/**
 * @brief 绕指定轴逆时针旋转 90°，对应原版 `Direction.getCounterClockWise(Axis)`
 *
 * 与 `getClockWise(Axis, dir)` 的映射方向相反：
 *
 *   绕 X：DOWN→NORTH、UP→SOUTH、NORTH→UP、SOUTH→DOWN
 *   绕 Y：NORTH→WEST、SOUTH→EAST、WEST→SOUTH、EAST→NORTH
 *   绕 Z：DOWN→EAST、UP→WEST、WEST→DOWN、EAST→UP
 */
[[nodiscard]] inline Direction getCounterClockWise(Axis axis, Direction dir)
{
    switch (axis) {
        case Axis::X:
            switch (dir) {
                case Direction::Down:
                    return Direction::North;
                case Direction::Up:
                    return Direction::South;
                case Direction::North:
                    return Direction::Up;
                case Direction::South:
                    return Direction::Down;
                default:
                    return dir;
            }
        case Axis::Y:
            if (dir == Direction::Up || dir == Direction::Down) {
                return dir;
            }
            return getCounterClockWise(dir);
        default:
            switch (dir) {
                case Direction::Down:
                    return Direction::East;
                case Direction::Up:
                    return Direction::West;
                case Direction::West:
                    return Direction::Down;
                case Direction::East:
                    return Direction::Up;
                default:
                    return dir;
            }
    }
}

} // namespace Directions

/**
 * @brief 旋转一个朝向，对应原版 `Rotation.rotate(Direction)`
 *
 * 竖直方向（Up/Down）不受水平旋转影响，原样返回。
 */
inline Direction rotate(Rotation rot, Direction dir)
{
    // 竖直方向不在水平面内，任何水平旋转都不改变它。
    // 这一条必须最先判断：CW180 走的是 opposite()，若不先拦下会把 Up/Down 互换。
    if (Directions::getAxis(dir) == Axis::Y) {
        return dir;
    }
    switch (rot) {
        case Rotation::None:
            return dir;
        case Rotation::Clockwise90:
            return Directions::getClockWise(dir);
        case Rotation::Clockwise180:
            return Directions::opposite(dir);
        case Rotation::CounterClockwise90:
            return Directions::getCounterClockWise(dir);
    }
    return dir;
}

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
    if (name == "x") {
        return Axis::X;
    }
    if (name == "y") {
        return Axis::Y;
    }
    if (name == "z") {
        return Axis::Z;
    }
    return std::nullopt;
}

inline std::string toString(Axis axis)
{
    const char* names[] = {"x", "y", "z"};
    return names[static_cast<size_t>(axis)];
}

/**
 * @brief 轴的正方向，对应原版 `Direction.Axis.getPositive()`
 *
 * X→EAST、Y→UP、Z→SOUTH。
 */
[[nodiscard]] inline Direction getPositive(Axis axis)
{
    switch (axis) {
        case Axis::X:
            return Direction::East;
        case Axis::Y:
            return Direction::Up;
        default:
            return Direction::South;
    }
}

/**
 * @brief 轴所属的平面，对应原版 `Direction.Axis.getPlane()`
 *
 * X、Z 属于水平面，Y 属于垂直面。
 */
[[nodiscard]] inline Plane getPlane(Axis axis)
{
    return axis == Axis::Y ? Plane::Vertical : Plane::Horizontal;
}

/**
 * @brief 按轴取对应分量，对应原版 `Direction.Axis.choose(int, int, int)`
 *
 * X 取 x，Y 取 y，Z 取 z。
 */
[[nodiscard]] inline i32 choose(Axis axis, i32 x, i32 y, i32 z)
{
    switch (axis) {
        case Axis::X:
            return x;
        case Axis::Y:
            return y;
        default:
            return z;
    }
}

/**
 * @brief 轴的负方向，对应原版 `Direction.Axis.getNegative()`
 *
 * X→WEST、Y→DOWN、Z→NORTH。
 */
[[nodiscard]] inline Direction getNegative(Axis axis)
{
    switch (axis) {
        case Axis::X:
            return Direction::West;
        case Axis::Y:
            return Direction::Down;
        default:
            return Direction::North;
    }
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
    // 原版 `AxisCycle.cycle(x, y, z, axis)` 的语义是把 (x, y, z) **重排后**再取 axis 分量：
    //
    //   NONE     → choose(x, y, z)
    //   FORWARD  → choose(z, x, y)
    //   BACKWARD → choose(y, z, x)
    //
    // FORWARD 与 BACKWARD 曾经写反（FORWARD 返回 (y, z, x)），
    // 导致 VoxelShape::forAllEdges 输出的棱端点顺序与原版相反。
    switch (cycle) {
        case AxisCycle::NONE:
            return (axis == Axis::X) ? x : (axis == Axis::Y) ? y : z;
        case AxisCycle::FORWARD:
            // (z, x, y)
            if (axis == Axis::X) {
                return z;
            }
            if (axis == Axis::Y) {
                return x;
            }
            return y;
        case AxisCycle::BACKWARD:
            // (y, z, x)
            if (axis == Axis::X) {
                return y;
            }
            if (axis == Axis::Y) {
                return z;
            }
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

/**
 * @brief 按三个分量的绝对值取主导轴方向，对应原版 `Direction.getNearest(int, int, int, Direction)`
 *
 * 取绝对值最大的那个分量所在的轴；若有并列（含全零），返回 `fallback`。
 *
 * **判定用的是严格大于**，所以任意两个分量的绝对值相等时都落到 `fallback`，
 * 而不是按 X > Y > Z 的优先级挑选。
 *
 * @param x X 分量
 * @param y Y 分量
 * @param z Z 分量
 * @param fallback 并列或全零时返回的方向
 * @return 主导轴方向，或 fallback
 */
[[nodiscard]] inline Direction getNearest(i32 x, i32 y, i32 z, Direction fallback)
{
    const i32 absX = std::abs(x);
    const i32 absY = std::abs(y);
    const i32 absZ = std::abs(z);
    if (absX > absZ && absX > absY) {
        return x < 0 ? Direction::West : Direction::East;
    }
    if (absZ > absX && absZ > absY) {
        return z < 0 ? Direction::North : Direction::South;
    }
    if (absY > absX && absY > absZ) {
        return y < 0 ? Direction::Down : Direction::Up;
    }
    return fallback;
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
    static Direction getOpposite(Direction dir)
    {
        return Directions::opposite(dir);
    }

    static Axis getAxis(Direction dir)
    {
        return Directions::getAxis(dir);
    }

    static AxisDirection getAxisDirection(Direction dir)
    {
        return Directions::getAxisDirection(dir);
    }

    static i32 getStepX(Direction dir)
    {
        return Directions::xOffset(dir);
    }

    static i32 getStepY(Direction dir)
    {
        return Directions::yOffset(dir);
    }

    static i32 getStepZ(Direction dir)
    {
        return Directions::zOffset(dir);
    }

    static size_t ordinal(Direction dir)
    {
        return static_cast<size_t>(dir);
    }
};

} // namespace mc
