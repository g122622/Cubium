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
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"

#include <functional>

namespace mc {

class BlockState;
class BlockPos;

/**
 * @brief 滴水石（Pointed Dripstone / Large Dripstone / Dripstone Cluster）共享工具
 *
 * 提供：
 *  - 高度公式 getDripstoneHeight（0.384 常数）
 *  - 方块分类谓词 isEmptyOrWater / isEmptyOrWaterOrLava / isNeitherEmptyNorWater
 *  - 基座判定 isDripstoneBase / isDripstoneBaseOrLava
 *  - 圆形嵌入石头判定 isCircleMostlyEmbeddedInStone
 *  - 建柱 buildBaseToTipColumn / growPointedDripstone
 *  - 滴水石块铺底 placeDripstoneBlockIfPossible
 *  - 尖端方块状态构造 createPointedDripstone
 */
class DripstoneUtils {
public:
    /**
     * @brief 给定半径处的滴石高度
     *
     * radius<=bluntness 时钳到 bluntness，避免 log(<=0)。
     */
    [[nodiscard]] static double getDripstoneHeight(double radius, double scale, double heightScale, double bluntness);

    /**
     * @brief 圆周采样判定圆周是否大多嵌入石头（非空/水/岩浆）
     */
    [[nodiscard]] static bool isCircleMostlyEmbeddedInStone(IWorld& world, const BlockPos& pos, i32 radius);

    [[nodiscard]] static bool isEmptyOrWater(IWorld& world, const BlockPos& pos);
    [[nodiscard]] static bool isEmptyOrWaterOrLava(IWorld& world, const BlockPos& pos);

    [[nodiscard]] static bool isDripstoneBaseOrLava(const BlockState* state);
    [[nodiscard]] static bool isDripstoneBase(const BlockState* state);
    [[nodiscard]] static bool isEmptyOrWater(const BlockState* state);
    [[nodiscard]] static bool isNeitherEmptyNorWater(const BlockState* state);
    [[nodiscard]] static bool isEmptyOrWaterOrLava(const BlockState* state);

    /**
     * @brief 沿 direction 方向从底到尖构建一列滴石方块状态
     *
     * @param direction 尖端朝向（柱从底向尖端生长方向）
     * @param height 列高
     * @param merge 尖端是否为合并态（TIP_MERGE）
     * @param emitter 接收每个方块状态的回调（按底→尖顺序）
     */
    static void buildBaseToTipColumn(
        Direction direction, i32 height, bool merge, std::function<void(const BlockState&)> emitter);

    /**
     * @brief 从 pos 起沿 direction 生长一列滴石
     *
     * 仅当 pos 朝反方向邻居为滴石基座时才生长；放置时按当前格是否含水设置 WATERLOGGED。
     */
    static void growPointedDripstone(IWorld& world, const BlockPos& pos, Direction direction, i32 height, bool merge);

    /**
     * @brief 若 pos 处方块可被滴水石块替换则放置 DRIPSTONE_BLOCK
     * @return 是否放置
     */
    static bool placeDripstoneBlockIfPossible(IWorld& world, const BlockPos& pos);

    /**
     * @brief 构造指定朝向与厚度的尖端滴石方块状态
     */
    [[nodiscard]] static const BlockState* createPointedDripstone(
        Direction direction, BlockStateProperties::DripstoneThickness thickness);
};

} // namespace mc
