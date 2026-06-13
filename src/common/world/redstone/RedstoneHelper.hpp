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

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include <algorithm>

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
class IInventory;

namespace world {
namespace redstone {

/**
 * @brief 红石辅助工具函数
 *
 * 提供红石系统常用的辅助函数。
 */
class RedstoneHelper {
public:
    /**
     * @brief 检查方块是否是实体方块（可以传导信号）
     *
     * 实体方块的特征：
     * - 是固体
     * - 是不透明的
     * - 不是空气
     *
     * @param state 方块状态
     * @return true 如果是实体方块
     */
    [[nodiscard]] static bool isNormalCube(const BlockState& state);

    /**
     * @brief 检查方块是否可以连接红石
     *
     * 可以连接红石的方块：
     * - 红石线
     * - 红石火把
     * - 中继器
     * - 比较器
     * - 其他可以输出红石信号的方块
     *
     * @param state 方块状态
     * @return true 如果可以连接
     */
    [[nodiscard]] static bool canConnectRedstone(const BlockState& state);

    /**
     * @brief 检查方块是否是红石导体
     *
     * 红石导体可以传导弱信号。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @return true 如果是红石导体
     */
    [[nodiscard]] static bool isRedstoneConductor(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 获取指定位置实体的最大比较器信号强度
     *
     * 遍历指定位置的实体，返回最大的 getComparatorOutput() 值。
     *
     * 通用实体信号查询工具。当前使用场景：
     * - DetectorRailBlock 通过 getComparatorInputOverride() 直接查询实体信号，
     *   但它有优先级排序逻辑（命令方块矿车 > 容器矿车 > 普通矿车），因此
     *   不使用此函数。
     * - RedstoneComparatorBlock 通过 _findItemFrame() 专门检测物品展示框，
     *   不使用此函数。
     * - 此函数适用于不需要优先级排序的通用场景，如压力板等需要获取
     *   任意实体信号强度的场合。
     *
     * @param world 世界引用
     * @param pos 检测位置（使用 1x1x1 AABB）
     * @return i32 信号强度 0-15
     */
    [[nodiscard]] static i32 getEntitySignal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 计算容器的红石信号强度
     *
     * 基于容器填充率计算信号强度，公式：
     *   fillRatio = sum(stack.count / stack.maxStackSize) / containerSize
     *   signal = floor(fillRatio * 14) + (nonEmptySlots > 0 ? 1 : 0)
     * 结果范围为 0-15。
     *
     * 参考: net.minecraft.world.inventory.AbstractContainerMenu.getRedstoneSignalFromContainer
     *
     * @param inventory 容器接口
     * @return i32 信号强度 0-15
     */
    [[nodiscard]] static i32 calcRedstoneFromInventory(const IInventory& inventory);

    /**
     * @brief 检查方向是否是水平方向
     *
     * @param dir 方向
     * @return true 如果是水平方向（北、东、南、西）
     */
    [[nodiscard]] static bool isHorizontal(Direction dir) { return Directions::isHorizontal(dir); }

    /**
     * @brief 检查方向是否是垂直方向
     *
     * @param dir 方向
     * @return true 如果是垂直方向（上、下）
     */
    [[nodiscard]] static bool isVertical(Direction dir) { return Directions::isVertical(dir); }

    /**
     * @brief 计算红石信号衰减后的强度
     *
     * @param strength 原始强度 (0-15)
     * @param distance 传输距离
     * @return i32 衰减后的强度 (最小为0)
     */
    [[nodiscard]] static i32 attenuate(i32 strength, i32 distance) noexcept { return std::max(0, strength - distance); }

    /**
     * @brief 限制红石信号强度在有效范围内
     *
     * @param strength 信号强度
     * @return i32 限制后的强度 (0-15)
     */
    [[nodiscard]] static i32 clamp(i32 strength) noexcept { return std::clamp(strength, MIN_POWER, MAX_POWER); }

    /// 红石信号最小强度
    static constexpr i32 MIN_POWER = 0;

    /// 红石信号最大强度
    static constexpr i32 MAX_POWER = 15;
};

} // namespace redstone
} // namespace world
} // namespace mc
