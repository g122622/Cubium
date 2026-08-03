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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <climits>

namespace mc {

// 前向声明
class IWorld;
namespace world::chunk {
class IChunk;
}
using world::chunk::IChunk;
class BlockState;
class CollisionShape;

// ============================================================================
// 方向位集常量 (Direction Bitset)
// 用于光照传播时快速跳过反向方向
// ============================================================================

/**
 * @brief 方向位集枚举
 *
 * 每个方向用一个位表示，可以组合多个方向。
 * 顺序与 Direction 枚举一致：
 * - DOWN=0, UP=1, NORTH=2, SOUTH=3, WEST=4, EAST=5
 *
 * 用途：
 * 1. 在光照传播队列中编码传播方向
 * 2. 快速获取相反方向（避免从光源方向传播回来）
 * 3. 批量处理多个方向
 */
enum DirectionBit : u8 {
    DIR_NONE = 0,
    DIR_DOWN = 1 << 0,                                            // Y- (下)
    DIR_UP = 1 << 1,                                              // Y+ (上)
    DIR_NORTH = 1 << 2,                                           // Z- (北)
    DIR_SOUTH = 1 << 3,                                           // Z+ (南)
    DIR_WEST = 1 << 4,                                            // X- (西)
    DIR_EAST = 1 << 5,                                            // X+ (东)
    DIR_ALL = 0x3F,                                               // 所有6个方向
    DIR_HORIZONTAL = DIR_NORTH | DIR_SOUTH | DIR_WEST | DIR_EAST, // 水平4方向
    DIR_VERTICAL = DIR_DOWN | DIR_UP                              // 垂直2方向
};

/**
 * @brief 方向位集工具函数
 */
namespace DirectionBits {
/**
 * @brief 将 Direction 转换为 DirectionBit
 */
[[nodiscard]] inline constexpr DirectionBit fromDirection(Direction dir)
{
    return static_cast<DirectionBit>(1 << static_cast<u8>(dir));
}

/**
 * @brief 获取相反方向的位集
 *
 * 利用方向编码的对称性：
 * - Down(0) ↔ Up(1)     -> 位0和位1互换
 * - North(2) ↔ South(3) -> 位2和位3互换
 * - West(4) ↔ East(5)   -> 位4和位5互换
 *
 * 通过位操作高效实现：偶数位左移1，奇数位右移1
 */
[[nodiscard]] inline constexpr DirectionBit opposite(DirectionBit bits)
{
    // 交换相邻位：0↔1, 2↔3, 4↔5
    // 偶数位 (0,2,4) 左移1位
    // 奇数位 (1,3,5) 右移1位
    return static_cast<DirectionBit>(((bits & 0x15) << 1) | // 010101 -> 偶数位左移
        ((bits & 0x2A) >> 1)                                // 101010 -> 奇数位右移
    );
}

/**
 * @brief 获取除了指定方向外的所有方向
 */
[[nodiscard]] inline constexpr DirectionBit allExcept(DirectionBit bits)
{
    return static_cast<DirectionBit>(DIR_ALL & ~bits);
}

/**
 * @brief 获取除了指定方向及其反方向外的所有方向
 */
[[nodiscard]] inline constexpr DirectionBit allExceptOpposite(DirectionBit bits)
{
    return static_cast<DirectionBit>(DIR_ALL & ~opposite(bits));
}

/**
 * @brief 检查是否包含指定方向
 */
[[nodiscard]] inline constexpr bool hasDirection(DirectionBit bits, Direction dir)
{
    return (bits & fromDirection(dir)) != 0;
}

/**
 * @brief 添加方向到位集
 */
[[nodiscard]] inline constexpr DirectionBit addDirection(DirectionBit bits, Direction dir)
{
    return static_cast<DirectionBit>(bits | fromDirection(dir));
}

/**
 * @brief 移除方向从位集
 */
[[nodiscard]] inline constexpr DirectionBit removeDirection(DirectionBit bits, Direction dir)
{
    return static_cast<DirectionBit>(bits & ~fromDirection(dir));
}

/**
 * @brief 获取位集中方向的数量
 */
[[nodiscard]] inline u32 count(DirectionBit bits)
{
// 使用编译器内置的 popcount（单指令实现）
#if defined(__GNUC__) || defined(__clang__)
    return static_cast<u32>(__builtin_popcount(static_cast<u32>(bits)));
#elif defined(_MSC_VER)
    return static_cast<u32>(__popcnt(static_cast<u32>(bits)));
#else
    // 回退到标准实现
    u32 c = 0;
    u32 b = bits;
    while (b) {
        c += b & 1;
        b >>= 1;
    }
    return c;
#endif
}

/**
 * @brief 从位集提取第一个方向
 * @return 第一个方向，如果为空返回 Direction::None
 */
[[nodiscard]] inline Direction firstDirection(DirectionBit bits)
{
    if (bits == DIR_NONE) {
        return Direction::None;
    }
    // 找到最低位的1
    for (u32 i = 0; i < 6; ++i) {
        if (bits & (1 << i)) {
            return static_cast<Direction>(i);
        }
    }
    return Direction::None;
}

/**
 * @brief 预计算的方向检查数组
 *
 * 预计算每个方向位集对应的方向数组。
 * 这样可以在传播时直接遍历数组，而不用遍历所有6个方向再检查是否在位集中。
 */
struct DirectionArray {
    Direction dirs[6];
    u32 count;
};

/**
 * @brief 获取位集对应的方向数组
 */
[[nodiscard]] inline DirectionArray toDirections(DirectionBit bits)
{
    DirectionArray result{};
    result.count = 0;

    if (bits & DIR_DOWN) result.dirs[result.count++] = Direction::Down;
    if (bits & DIR_UP) result.dirs[result.count++] = Direction::Up;
    if (bits & DIR_NORTH) result.dirs[result.count++] = Direction::North;
    if (bits & DIR_SOUTH) result.dirs[result.count++] = Direction::South;
    if (bits & DIR_WEST) result.dirs[result.count++] = Direction::West;
    if (bits & DIR_EAST) result.dirs[result.count++] = Direction::East;

    return result;
}
} // namespace DirectionBits

/**
 * @brief 光照引擎工具类
 *
 * 提供光照引擎共享的工具方法。
 */
class LightEngineUtils {
public:
    // ========================================================================
    // 常量
    // ========================================================================

    /** 根节点位置标记（用于光源） */
    static constexpr i64 ROOT_POS = LONG_MAX;

    /** 所有6个方向（用于遍历相邻方块） */
    static constexpr Direction ALL_DIRECTIONS[6] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

    /** 水平4个方向 */
    static constexpr Direction HORIZONTAL_DIRECTIONS[4] = {
        Direction::North, Direction::South, Direction::West, Direction::East};

    // ========================================================================
    // 位置编码
    // ========================================================================

    /**
     * @brief 世界位置编码
     *
     * 编码格式: X(26位) | Z(26位) | Y(12位)
     * 支持 X/Z: ±30,000,000 (约 ±2^25)
     * 支持 Y: -2048 到 +2047
     */
    [[nodiscard]] static constexpr i64 packPos(i32 x, i32 y, i32 z)
    {
        // 26位掩码
        constexpr i64 XZ_MASK = (1LL << 26) - 1;
        constexpr i64 Y_MASK = (1LL << 12) - 1;
        // 位移量
        constexpr i32 Y_BITS = 12;
        constexpr i32 Z_OFFSET = Y_BITS;      // 12
        constexpr i32 X_OFFSET = Y_BITS + 26; // 38

        return ((static_cast<i64>(x) & XZ_MASK) << X_OFFSET) | ((static_cast<i64>(y) & Y_MASK)) |
            ((static_cast<i64>(z) & XZ_MASK) << Z_OFFSET);
    }

    /**
     * @brief 从BlockPos编码位置
     */
    [[nodiscard]] static constexpr i64 packPos(const BlockPos& pos) { return packPos(pos.x, pos.y, pos.z); }

    /**
     * @brief 世界位置解码
     */
    static constexpr void unpackPos(i64 packed, i32& x, i32& y, i32& z)
    {
        // 26位掩码
        constexpr i64 XZ_MASK = (1LL << 26) - 1;
        constexpr i64 Y_MASK = (1LL << 12) - 1;
        constexpr i32 Y_BITS = 12;

        // 解码并自动符号扩展（使用算术右移）
        // X: 取高26位，通过左移0位后右移38位实现符号扩展
        x = static_cast<i32>(packed >> (Y_BITS + 26));
        // Y: 取低12位，通过左移52位后右移52位实现符号扩展
        y = static_cast<i32>((packed << (64 - Y_BITS)) >> (64 - Y_BITS));
        // Z: 取中间26位，需要提取后符号扩展
        z = static_cast<i32>((packed >> Y_BITS) & XZ_MASK);
        // Z需要手动符号扩展（26位到32位）
        z = (z << 6) >> 6;
    }

    /**
     * @brief 位置偏移
     */
    [[nodiscard]] static i64 offsetPos(i64 pos, Direction dir)
    {
        i32 x, y, z;
        unpackPos(pos, x, y, z);

        switch (dir) {
            case Direction::Down:
                --y;
                break;
            case Direction::Up:
                ++y;
                break;
            case Direction::North:
                --z;
                break;
            case Direction::South:
                ++z;
                break;
            case Direction::West:
                --x;
                break;
            case Direction::East:
                ++x;
                break;
            default:
                break;
        }

        return packPos(x, y, z);
    }

    /**
     * @brief 世界位置转区块段位置
     */
    [[nodiscard]] static i64 worldToSectionPos(i64 worldPos);

    /**
     * @brief 从编码位置提取NibbleArray索引
     *
     * @param packed 编码位置
     * @param x 输出：局部X坐标 (0-15)
     * @param localY 输出：局部Y坐标 (0-15)
     * @param z 输出：局部Z坐标 (0-15)
     */
    static constexpr void extractNibbleIndices(i64 packed, i32& x, i32& localY, i32& z)
    {
        // X在高位，偏移38位；Z在中间，偏移12位；Y在低位
        x = static_cast<i32>((packed >> 38) & world::CHUNK_MASK);
        i32 y = static_cast<i32>(packed & 0xFFF);
        localY = y & world::CHUNK_MASK;
        z = static_cast<i32>((packed >> 12) & world::CHUNK_MASK);
    }

    // ========================================================================
    // 方块查询
    // ========================================================================

    /**
     * @brief 从区块获取方块及其透明度
     *
     * @param chunk 区块指针
     * @param worldPos 编码的世界位置
     * @param opacityOut 透明度输出（可选）
     * @return 方块状态指针，如果是空气返回nullptr
     */
    [[nodiscard]] static const BlockState* getBlockAndOpacity(const IChunk* chunk, i64 worldPos, i32* opacityOut);

    /**
     * @brief 获取方块的遮挡形状
     *
     * @param state 方块状态
     * @return 遮挡形状，如果是非固体方块返回空形状
     */
    [[nodiscard]] static const CollisionShape& getVoxelShape(const BlockState& state);

    // ========================================================================
    // 遮挡检测
    // ========================================================================

    /**
     * @brief 检查两个方块之间是否有完整的遮挡面
     *
     * 用于判断光线是否可以通过两个相邻方块的接触面。
     * 如果两个方块都是完整的固体方块，且有完全遮挡的面，则返回true。
     *
     * @param world 世界
     * @param stateA 第一个方块的状态
     * @param posA 第一个方块的位置
     * @param stateB 第二个方块的状态
     * @param posB 第二个方块的位置
     * @param dir 从A到B的方向
     * @param opacityA 方块A的光照透明度
     * @return 如果有完整遮挡面返回true
     */
    [[nodiscard]] static bool facesHaveOcclusion(IWorld* world,
        const BlockState& stateA,
        const BlockPos& posA,
        const BlockState& stateB,
        const BlockPos& posB,
        Direction dir,
        i32 opacityA);

    /**
     * @brief 检查方块是否阻挡特定方向的光线
     *
     * @param state 方块状态
     * @param dir 光线传播方向
     * @return 如果方块在该方向完全阻挡光线返回true
     */
    [[nodiscard]] static bool blocksLightInDirection(const BlockState& state, Direction dir);

    /**
     * @brief 计算光线从一个方块传播到相邻方块时的阻挡值
     *
     * 结合目标方块不透明度与两侧面遮挡形状，计算光线穿过边界时的有效阻挡。
     *
     * @param world 世界
     * @param sourceState 光线发出方块的状态
     * @param sourcePos 光线发出方块位置
     * @param targetState 光线进入方块的状态
     * @param targetPos 光线进入方块位置
     * @param dir 光线传播方向（从 source 指向 target）
     * @param targetOpacity 目标方块的不透明度缓存
     * @return 有效阻挡值，范围至少为 1
     */
    [[nodiscard]] static i32 getLightBlockInto(IWorld& world,
        const BlockState& sourceState,
        const BlockPos& sourcePos,
        const BlockState& targetState,
        const BlockPos& targetPos,
        Direction dir,
        i32 targetOpacity);

private:
    /**
     * @brief 检查碰撞形状是否在指定方向完全遮挡
     */
    [[nodiscard]] static bool _shapeFullyOccludesFace(const CollisionShape& shape, Direction face);
};

} // namespace mc
