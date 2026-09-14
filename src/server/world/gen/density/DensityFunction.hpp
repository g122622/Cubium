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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
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
#include <memory>

namespace mc::world::gen::density {

/**
 * @brief 密度函数接口
 *
 * MC 1.18+ 引入的密度函数系统，用于地形生成和气候参数计算。
 * 每个密度函数接收方块坐标 (x, y, z)，返回一个密度值。
 *
 * 密度函数可以组合（加、乘、钳制等），形成表达式树。
 * NoiseRouter 持有 15 个密度函数引用，其中 6 个用于 Climate.Sampler。
 *
 * MC 1.21 Visitor/mapAll 机制：
 * NoiseChunk 构造时使用 mapAll(wrapVisitor) 遍历整棵密度函数树，
 * 将 Marker 类型替换为 NoiseChunk 本地实现（NoiseInterpolator、CellCache 等）。
 *
 * mapAll 所有权语义：
 * - 调用方持有 unique_ptr<DensityFunction>
 * - mapAll 返回一个新的 unique_ptr，可能指向原对象（visitor 不替换）或新对象
 * - 复合函数先对子函数递归 mapAll，再用新子函数重建自身，最后传给 visitor.apply()
 * - 叶子函数 clone 自身传给 visitor.apply()，确保所有权清晰
 */
class DensityFunction {
public:
    /**
     * @brief 密度函数树的访问者
     *
     * MC 1.21 对应 DensityFunction.Visitor。
     * mapAll 递归遍历子函数后，对每个节点调用 visitor.apply()。
     * NoiseChunk 使用此机制将 Marker 替换为插值/缓存实现。
     */
    class Visitor {
    public:
        virtual ~Visitor() = default;

        /**
         * @brief 对密度函数节点应用变换
         * @param function 待变换的密度函数（转移所有权）
         * @return 变换后的密度函数（可以是原对象或新对象）
         */
        [[nodiscard]] virtual std::unique_ptr<DensityFunction> apply(std::unique_ptr<DensityFunction> function) = 0;
    };

    virtual ~DensityFunction() = default;

    /**
     * @brief 在指定方块坐标处计算密度值
     */
    [[nodiscard]] virtual f64 compute(i32 blockX, i32 blockY, i32 blockZ) const = 0;

    /** 密度函数的最小可能值 */
    [[nodiscard]] virtual f64 minValue() const = 0;

    /** 密度函数的最大可能值 */
    [[nodiscard]] virtual f64 maxValue() const = 0;

    /**
     * @brief 递归遍历密度函数树，对每个节点应用 visitor
     *
     * MC 1.21 对应 DensityFunction.mapAll(Visitor)。
     * 复合密度函数应先对子函数递归调用 mapAll，再对自身调用 visitor.apply()。
     * 叶子节点的默认实现：clone 自身并传给 visitor.apply()。
     *
     * 注意：所有子类必须 override 此方法。
     * 叶子节点（无子函数）可使用 DENSITY_FUNCTION_MAP_ALL_LEAF 宏。
     * 复合节点（有子函数）应先对子函数递归 mapAll，再重建自身并 apply visitor。
     *
     * @param visitor 访问者
     * @return 变换后的密度函数（可能是新对象）
     */
    [[nodiscard]] virtual std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const = 0;
};

/**
 * @brief 叶子密度函数 mapAll 实现宏
 *
 * 叶子节点没有子函数，直接 clone 自身传给 visitor.apply()。
 * 使用方式：在叶子类中写 DENSITY_FUNCTION_MAP_ALL_LEAF(ClassName, constructor_args)
 */
#define DENSITY_FUNCTION_MAP_ALL_LEAF(ClassName, ...)                                      \
    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override \
    {                                                                                      \
        return visitor.apply(std::make_unique<ClassName>(__VA_ARGS__));                    \
    }

} // namespace mc::world::gen::density
