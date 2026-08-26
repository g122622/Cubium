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

#include "common/world/gen/density/ast/AstNode.hpp"

#include <memory>

namespace mc::world::gen::density::ast {

/**
 * @brief AST 优化 pass 集合（效仿 C2ME DFC OptoPasses）
 *
 * 对 McToAst 产出的 AST 跑三个重写 pass 到不动点，产出数值等价但更精简的 AST：
 *
 * 1. TreeNormalization：可交换二元节点（Add/Mul/Div，canSwapOperandsSafely=true）把常量
 *    规范到左边。为 FoldConstants 的"常量在左"约定铺路。Min/Max/MaxShort/MinShort 不可交换。
 * 2. FoldConstants：常量折叠。两常量二元运算合并、x+0/x*1 单位消除、x*0 归零、一元折叠、
 *    Marker 的 delegate 折成常量则整 Marker 折叠、Lerp/Clamp/Div 常量折叠。
 *    -0.0 符号处理对齐 DFC ZeroUtils：仅当 Add 左操作数常量为 -0.0 时才消去（+0.0 + x 会抹掉
 *    x 的 -0.0 符号，与原 x + 0.0 语义不同，故不消）。
 * 3. BranchElimination：RangeChoice 输入为常量时按区间选支；两支 relaxedEquals 相等则消分支。
 *
 * 不动点驱动：每轮全树自底向上 transform 一次（AstNode::transform 递归 + persistence sharing），
 * 三 pass 串行；若本轮根节点引用未变则收敛。对齐 DFC OptoPasses.optimize0。
 *
 * 不实现 DFC 的 CacheElimination：那是 OCL 离线编译专用（剥 FlatCache 内层缓存），Cubium 不做
 * OCL，且 Cubium 的 Marker 分层语义与 DFC CacheLikeNode 不同（MarkerNode 在区块级原地替换）。
 */
class OptoPasses {
public:
    /// AST 节点指针别名。
    using Ptr = AstNode::Ptr;

    /**
     * @brief 对 AST 跑 TreeNormalization → FoldConstants → BranchElimination 到不动点。
     * @param root 待优化 AST 根节点，不可为 null。
     * @return 优化后的 AST 根节点（若无可优化则与入参同一引用，persistence sharing）。
     */
    [[nodiscard]] static Ptr optimize(Ptr root);
};

} // namespace mc::world::gen::density::ast
