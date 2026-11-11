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

#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/base/types/BlockPos.hpp"

namespace mc {

// 前向声明
class BlockState;
class IWorld;

/**
 * @brief 方块支撑类型
 *
 * 对应 MC 1.21.11 net.minecraft.world.level.block.SupportType，
 * 用于判断方块面是否足够坚固以支撑其他方块（火把、灯笼、钟、压力板、铁轨等）。
 *
 * 三种支撑类型：
 * - Full：方块面投影必须覆盖整个 1×1 面（最严格）
 * - Center：方块面投影必须包含中心 2×2 像素到 10×10 像素的"中心柱"区域
 * - Rigid：方块面投影必须覆盖 1×1 面除中心 12×12 像素柱以外的外环区域
 *
 * 判定基于方块的 BlockSupportShape（支撑形状），默认等于碰撞形状。
 * 某些方块（如泥巴、灵魂沙）的碰撞形状比完整方块矮，但支撑形状是完整方块。
 *
 */
class SupportType {
public:
    /**
     * @brief 判定函数签名
     *
     * @param state 待判定的方块状态
     * @param world 世界接口（用于查询方块的支撑形状）
     * @param pos 方块位置
     * @param direction 检查的面方向
     * @return 是否提供该方向的支撑
     */
    using IsSupportingFn = bool (*)(const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief 带位置偏移的判定函数签名
     */
    using IsSupportingAtFn = bool (*)(const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief Full 支撑类型
     *
     * 方块面投影必须覆盖整个 1×1 面。
     * 对应 MC Java 的 SupportType.FULL。
     */
    static const SupportType Full;

    /**
     * @brief Center 支撑类型
     *
     * 方块面投影必须包含中心 2×2 像素柱（直径 2、高度 0-10 像素）。
     * 对应 MC Java 的 SupportType.CENTER。
     * 用于火把、灯笼、钟等悬挂方块的支撑判定。
     */
    static const SupportType Center;

    /**
     * @brief Rigid 支撑类型
     *
     * 方块面投影必须覆盖 1×1 面除中心 12×12 像素柱（直径 12、高度 0-16 像素）以外的外环。
     * 对应 MC Java 的 SupportType.RIGID。
     * 用于铁轨、压力板等需要外环支撑的方块。
     */
    static const SupportType Rigid;

    /**
     * @brief 构造支撑类型
     * @param fn 判定函数
     */
    constexpr SupportType(IsSupportingFn fn, IsSupportingAtFn fnAt) noexcept
        : m_fn(fn)
        , m_fnAt(fnAt)
    {}

    /**
     * @brief 判断方块是否在指定方向提供该类型的支撑
     *
     * @param state 待判定的方块状态
     * @param world 世界接口
     * @param pos 方块位置
     * @param direction 检查的面方向
     * @return 如果提供支撑返回 true
     */
    [[nodiscard]] bool isSupporting(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction) const
    {
        return m_fn(state, world, pos, direction);
    }

    /**
     * @brief 世界查询专用：支撑形状带上位置偏移后再判定
     *
     * 支撑形状取 `state.getBlockSupportShapeAt(pos)`，使带 `offsetType` 的方块
     * （竹子、花、滴水石锥）先叠加坐标哈希偏移。无位置的 `isSupporting` 只用于
     * 「同一状态恒得到同一答案」的场景，世界里的附着判定必须走本重载，否则
     * 中心的 2×2 柱会因忽略偏移而判定翻转。
     *
     * @param state 待判定的方块状态
     * @param world 世界接口
     * @param pos 方块位置
     * @param direction 检查的面方向
     * @return 如果提供支撑返回 true
     */
    [[nodiscard]] bool isSupportingAt(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction) const
    {
        return m_fnAt(state, world, pos, direction);
    }

private:
    IsSupportingFn m_fn;
    IsSupportingAtFn m_fnAt;
};

namespace support_type_detail {

/**
 * @brief 获取 CENTER 支撑形状（Block.column(2.0, 0.0, 10.0) 等价物）
 *
 * MC Java 中 Block.column(diameter, minY, maxY) = box(8-d/2, minY, 8-d/2, 8+d/2, maxY, 8+d/2)（像素坐标）。
 * column(2.0, 0.0, 10.0) = box(7, 0, 7, 9, 10, 9) 像素 = box(7/16, 0, 7/16, 9/16, 10/16, 9/16) 方块坐标。
 */
[[nodiscard]] const VoxelShape& centerSupportShape();

/**
 * @brief 获取 RIGID 支撑形状（外环）
 *
 * MC Java 中 RIGID_SUPPORT_SHAPE = Shapes.join(Shapes.block(), Block.column(12.0, 0.0, 16.0), BooleanOp.ONLY_FIRST)。
 * 即完整方块减去中心 12×12 像素柱（直径 12、高度 0-16 像素）。
 * column(12.0, 0.0, 16.0) = box(2, 0, 2, 14, 16, 14) 像素 = box(2/16, 0, 2/16, 14/16, 1, 14/16) 方块坐标。
 */
[[nodiscard]] const VoxelShape& rigidSupportShape();

/**
 * @brief Full 支撑判定
 *
 * 方块面投影必须覆盖整个 1×1 面。
 * 等价于 Block::isFaceFull(blockSupportShape, direction)。
 */
[[nodiscard]] bool isSupportingFull(const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

/**
 * @brief Center 支撑判定
 *
 * 方块支撑形状在指定方向的面投影必须包含 CENTER_SUPPORT_SHAPE。
 * 等价于 !Shapes.joinIsNotEmpty(faceShape, CENTER_SUPPORT_SHAPE, BooleanOp.ONLY_SECOND)。
 */
[[nodiscard]] bool isSupportingCenter(const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

/**
 * @brief Rigid 支撑判定
 *
 * 方块支撑形状在指定方向的面投影必须包含 RIGID_SUPPORT_SHAPE（外环）。
 * 等价于 !Shapes.joinIsNotEmpty(faceShape, RIGID_SUPPORT_SHAPE, BooleanOp.ONLY_SECOND)。
 */
[[nodiscard]] bool isSupportingRigid(const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

/// 带位置偏移的 Full 判定（支撑形状取 `getBlockSupportShapeAt(pos)`）
[[nodiscard]] bool isSupportingFullAt(const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

/// 带位置偏移的 Center 判定（支撑形状取 `getBlockSupportShapeAt(pos)`）
[[nodiscard]] bool isSupportingCenterAt(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

/// 带位置偏移的 Rigid 判定（支撑形状取 `getBlockSupportShapeAt(pos)`）
[[nodiscard]] bool isSupportingRigidAt(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction direction);

} // namespace support_type_detail

} // namespace mc
