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
#include "common/world/gen/density/ast/AstNodes.hpp"

#include "common/core/Types.hpp"

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace mc::world::gen::density {
class DensityFunction;
} // namespace mc::world::gen::density

namespace mc::world::gen::density::ast {

/**
 * @brief DensityFunction 树 → AST 转换器（效仿 C2ME DFC McToAst）
 *
 * 把现有 DensityFunction 表达式树转成 AST 中间表示。转换在维度级（create 期 buildRouterFromTemplate
 * 绑定 + 共享化之后的 m_router、含 Marker 的树）一次性进行，产出的 AST 不可变，供后续优化 pass
 * 与求值器编译使用。
 *
 * 机制（对齐 DFC）：FrontendRegistry 按 DensityFunction 的精确动态类型（C++ 用 std::type_index
 * 替代 Java getClass()）匹配注册的 AstEmitter。各 DF 子类 → AST 节点映射在 initialize() 注册。
 * 无匹配类型退化为 DelegateNode（包原版 DF 运行时回调 compute），保证未适配 DF 仍可求值——
 * 这是渐进式迁移的安全网，退化的 DF 数量可在日志中观测。
 *
 * 坐标展开：噪声类 DF（Noise/ShiftedNoise/Shift/ShiftA/ShiftB/MappedNoise）的坐标变换
 * （blockX*xzScale + shiftX 等）在转换期展开为 Add/Mul/Coordinate 子 AST，使坐标变换可参与
 * 常量折叠等优化 pass（对齐 DFC GenericShiftedNoiseNode 用法）。
 *
 * Marker 分层：Marker DF → MarkerNode（保留 MarkerType + delegate 子 AST）。维度级编译产物含
 * MarkerNode 占位，区块级 newInstance 时原地替换为 per-chunk 缓存实现（对齐 DFC c2me$withDelegate）。
 *
 * SharedTopology 分层：SharedTopology DF → SharedSubtreeRefNode（内部子树独立编译为可复用求值器，
 * 父节点索引调用），保留 Cubium 既有的纯拓扑跨区块共享优化。
 */
class McToAst {
public:
    /// AST 产出节点类型。
    using Ptr = AstNode::Ptr;

    /// 转换入口：把一棵 DensityFunction 树转成 AST。df 不可为 null。
    [[nodiscard]] static Ptr convert(const DensityFunction& df);

private:
    /// 单条转换规则：给定 DF 实例引用，返回 AST 节点。
    using Emitter = std::function<Ptr(const DensityFunction&)>;

    /// 类型索引 → 转换规则表（对齐 DFC FrontendRegistry，C++ 用 type_index 替代 getClass）。
    /// 静态局部 map，首次访问时由 initializeBuiltin() 填充。
    static std::unordered_map<std::type_index, Emitter>& registry();

    /// 注册全部内置 DF 子类 → AST 节点的转换规则（填充给定 registry）。
    static void initializeBuiltin(std::unordered_map<std::type_index, Emitter>& reg);

    /// 内部递归转换（注册表查不到则 DelegateNode 退化）。
    [[nodiscard]] static Ptr toAst(const DensityFunction& df);
};

} // namespace mc::world::gen::density::ast
