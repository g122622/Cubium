/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp"

#include <memory>

namespace mc::world::gen::density::ast {

/**
 * @brief 编译产物 → DensityFunction 适配器（阶段5）
 *
 * 把 CompiledDensityFunction（扁平指令序列求值器）包装为 DensityFunction 子类，
 * 让缓存类（NoiseInterpolator/CellCache/CacheOnce/FlatCache/Cache2D）的 filler
 * （unique_ptr<DensityFunction>）可持编译产物。这是编译产物与 OOP 接口的必要桥接——
 * CompiledDensityFunction（扁平指令序列）与 DensityFunction（多态表达式树）是两个体系，
 * 缓存类 filler 需要 DensityFunction 接口，故用 Adapter 桥接（非兼容性 adapter，
 * 符合 CODE_CONVENTIONS 一步到位新代码）。
 *
 * 持 shared_ptr<CompiledDensityFunction>：区块级求值器可能被多个缓存对象引用
 * （如嵌套 Marker 的 delegate 子树被多级缓存共享），shared_ptr 安全。
 *
 * compute → eval；minValue/maxValue → CompiledDensityFunction 编译期记录值；
 * mapAll → 区块级求值器不可变（Marker 已在 newInstance 替换为缓存对象），无再 mapAll 必要，
 * 返回共享同一 compiled 的新 Adapter。
 */
class CompiledDensityFunctionAdapter final : public DensityFunction {
public:
    explicit CompiledDensityFunctionAdapter(std::shared_ptr<CompiledDensityFunction> compiled)
        : m_compiled(std::move(compiled))
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        return m_compiled->eval(blockX, blockY, blockZ);
    }

    [[nodiscard]] f64 minValue() const override { return m_compiled->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_compiled->maxValue(); }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& /*visitor*/) const override
    {
        // 区块级求值器不可变（Marker 已在 newInstance 替换为缓存对象），无再 mapAll 必要。
        // 返回共享同一 compiled 的新 Adapter。
        return std::make_unique<CompiledDensityFunctionAdapter>(m_compiled);
    }

    /// 被包装的编译产物。
    [[nodiscard]] const std::shared_ptr<CompiledDensityFunction>& compiled() const { return m_compiled; }

private:
    std::shared_ptr<CompiledDensityFunction> m_compiled;
};

} // namespace mc::world::gen::density::ast
