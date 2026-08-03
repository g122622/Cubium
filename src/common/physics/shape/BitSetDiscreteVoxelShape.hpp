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

#include "BooleanOp.hpp"
#include "DiscreteVoxelShape.hpp"
#include "Shapes.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <climits>
#include <vector>

namespace mc {

/**
 * @brief 基于 BitSet 的离散体素形状
 *
 * 使用更紧凑的位存储和更高效的布尔运算实现。
 * 提供静态 join 方法，用于执行两个 DiscreteVoxelShape 之间的布尔运算。
 *
 * 对应 MC Java 版的 BitSetDiscreteVoxelShape。
 */
class BitSetDiscreteVoxelShape : public DiscreteVoxelShape {
public:
    /**
     * @brief 构造指定大小的空形状
     */
    BitSetDiscreteVoxelShape(i32 xSize, i32 ySize, i32 zSize)
        : DiscreteVoxelShape(xSize, ySize, zSize)
    {}

    /**
     * @brief 对两个离散体素形状执行布尔运算
     *
     * 这是 VoxelShape 布尔运算的核心算法。使用三个 IndexMerger 来
     * 将两个形状的坐标系统一到一个合并的网格中，然后对每个合并后的
     * 体素单元应用布尔运算。
     *
     * @param shapeA 第一个形状
     * @param shapeB 第二个形状
     * @param xMerger X轴坐标合并器
     * @param yMerger Y轴坐标合并器
     * @param zMerger Z轴坐标合并器
     * @param op 布尔运算
     * @return 运算结果的新形状
     */
    [[nodiscard]] static DiscreteVoxelShape join(const DiscreteVoxelShape& shapeA,
        const DiscreteVoxelShape& shapeB,
        const Shapes::IndexMerger& xMerger,
        const Shapes::IndexMerger& yMerger,
        const Shapes::IndexMerger& zMerger,
        const BooleanOp& op)
    {
        // 结果形状的尺寸 = 各轴合并后的段数
        const i32 xSize = xMerger.size() - 1;
        const i32 ySize = yMerger.size() - 1;
        const i32 zSize = zMerger.size() - 1;

        // 创建结果形状
        DiscreteVoxelShape result(xSize, ySize, zSize);

        // 边界跟踪
        i32 xMin = xSize, yMin = ySize, zMin = zSize;
        i32 xMax = 0, yMax = 0, zMax = 0;
        bool anyFilled = false;

        // 三重循环：遍历所有合并后的体素单元
        (void)xMerger.forMergedIndexes([&](i32 xIdxA, i32 xIdxB, i32 xIdxMerged) -> bool {
            return yMerger.forMergedIndexes([&](i32 yIdxA, i32 yIdxB, i32 yIdxMerged) -> bool {
                return zMerger.forMergedIndexes([&](i32 zIdxA, i32 zIdxB, i32 zIdxMerged) -> bool {
                    // 查询两个形状在对应位置是否填充
                    // isFullWide 对越界索引返回 false
                    const bool inA = shapeA.isFullWide(xIdxA, yIdxA, zIdxA);
                    const bool inB = shapeB.isFullWide(xIdxB, yIdxB, zIdxB);

                    // 应用布尔运算
                    if (op.apply(inA, inB)) {
                        result.fill(xIdxMerged, yIdxMerged, zIdxMerged);

                        // 更新边界
                        xMin = std::min(xMin, xIdxMerged);
                        yMin = std::min(yMin, yIdxMerged);
                        zMin = std::min(zMin, zIdxMerged);
                        xMax = std::max(xMax, xIdxMerged + 1);
                        yMax = std::max(yMax, yIdxMerged + 1);
                        zMax = std::max(zMax, zIdxMerged + 1);
                        anyFilled = true;
                    }
                    return true;
                });
            });
        });

        // 设置边界缓存以避免后续重算
        if (anyFilled) {
            result.setBounds(xMin, yMin, zMin, xMax, yMax, zMax);
        }

        return result;
    }
};

} // namespace mc
