/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom this Software is
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

#include "common/world/gen/density/ast/AstNode.hpp"
#include "common/world/gen/density/ast/CompiledDensityFunction.hpp"

#include <memory>

namespace mc::world::gen::density::ast {

/**
 * @brief AST → 扁平指令序列编译器（阶段4）
 *
 * 把优化后的 AST 树编译成 CompiledDensityFunction（扁平指令序列求值器）。
 * 递归遍历 AST，每个节点编译为一段 Op 顺序内联（无 CALL/RET，cache 友好），
 * 结果写一个编译期分配的寄存器 slot。
 *
 * 常量折叠到调用点：ConstantNode 不生成独立 LOAD_CONST 指令，而是返回常量值供父节点
 * 直接内联到 imm 字段（对齐 DFC ValuesMethodDef.isConst）。
 *
 * 子树引用：样条嵌套子样条、SharedSubtreeRef、FindTopSurface.density 独立编译为子
 * CompiledDensityFunction，父序列用 SHARED_SUBTREE_CALL/SPLINE/FIND_TOP_SURFACE 指令调度。
 *
 * 子树去重：同结构子树（relaxedEquals）共享同一段 Op + 同一寄存器。阶段4 先做地址级
 * 共享（同一 shared_ptr<AstNode> 共享），relaxedEquals 跨实例合并留 TODO。
 *
 * Marker：MARKER 指令透传 delegate 结果（维度级占位），区块级 newInstance 据 markerType
 * 原地替换缓存段（阶段5）。
 */
class BytecodeGen {
public:
    /// 把 AST 树编译成求值器。root 为优化后的 AST 根节点。
    /// minValue/maxValue 从原 DensityFunction 取（编译期记录到 CompiledDensityFunction，
    /// 供 CompiledDensityFunctionAdapter::minValue/maxValue 返回）。子求值器编译（样条/
    /// SharedSubtree/FindTopSurface/Marker delegate）传哨兵值（经 eval 调用不经 Adapter，
    /// min/max 无人消费）；Marker delegate 例外，传 MarkerNode 记录的 delegate min/max。
    [[nodiscard]] static std::shared_ptr<CompiledDensityFunction> compile(
        const AstNode::Ptr& root, f64 minValue, f64 maxValue);

private:
    // 实现见 BytecodeGen.cpp（编译上下文 + 各节点 emit 方法）。
};

} // namespace mc::world::gen::density::ast
