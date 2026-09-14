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
#include <memory>
#include <vector>

namespace mc::world::gen::density::ast {
/// AST 节点类型标识。用于编译期 switch 分发（指令生成）与 relaxedEquals 类型二分。
/// 顺序按 DFC common/ast 分类组织：binary / unary / leaf / noise / spline / control / marker / 特有。
enum class AstNodeKind : u8 {
    // binary（两子节点运算）
    Add,
    Mul,
    Div, // 由 Mapped(Invert)=1/x 转换而来，DFC 拆成 Div(Constant(1), input)
    Max,
    Min,
    MaxShort, // 带右操作数上界短路的 Max（对齐 DFC MaxShortNode，持 rightMax 标量）
    MinShort, // 带右操作数下界短路的 Min（持 rightMin 标量）
    // unary（单子节点运算）
    Abs,
    Square,
    Cube,
    Squeeze,
    Sqrt,
    Sin,
    Cos,
    Floor,
    Ceil,
    NegMul, // 持 negMul 系数，compute = input <= 0 ? input*negMul : input
    // leaf（无子节点）
    Constant,
    ConstantF32, // F32 常量（样条 FixedFloatFunction）
    Coordinate,  // X/Y/Z 坐标轴
    YClampedGradient,
    EndIslands,
    // noise（噪声采样，持 shared_ptr<const NormalNoise>）
    GenericShiftedNoise, // 覆盖 Noise/ShiftedNoise/Shift/ShiftA/ShiftB（坐标变换在 AST 层展开）
    WeirdScaledSampler,
    BlendedNoise, // 旧式三层 Perlin（OldBlendedNoise）
    MappedNoise,  // Noise + fromValue/toValue 重映射（可拆成 GenericShiftedNoise+Add+Mul，此处保留语义节点）
    // spline（样条）
    Spline, // 持 locationFunction + locations/derivatives/values，二分查找 + Hermite
    // control（条件选择）
    RangeChoice,
    Clamp,
    Lerp, // 三子：delta/start/end
    // marker / cache（AST 保留 Marker 语义，区块级原地替换）
    Marker,     // 6 种 MarkerType 占位（Interpolated/CacheOnce/CacheAllInCell/FlatCache/Cache2D/BeardifierMarker）
    Beardifier, // Beardifier 实例（Delegate 系，运行时回调）
    // Cubium 特有节点
    SharedSubtreeRef, // SharedTopology 边界：内部子树独立编译为可复用求值器，父节点索引调用
    FindTopSurface,   // 循环搜索节点（Cubium 特有，DFC 无对应）
    // delegate（未识别 DF 退化）
    Delegate, // 包原版 DensityFunction 运行时回调 compute
};

/// 求值返回类型。MC 样条路径用 F32（float 算术），其余 F64。
enum class ReturnType : u8 {
    F64,
    F32,
};

class AstNode;

/// AST 单节点重写接口（对齐 DFC AstTransformer）。
/// 各优化 pass 实现此接口，对单个节点做局部重写；transform 自底向上递归调用。
class AstTransformer {
public:
    virtual ~AstTransformer() = default;

    /// 对单个节点应用重写。返回新节点（未改动的子树原样保留引用，persistence sharing）。
    [[nodiscard]] virtual std::shared_ptr<const AstNode> transform(std::shared_ptr<const AstNode> node) = 0;
};

/**
 * @brief AST 节点抽象基类
 *
 * 效仿 C2ME DFC 的 AST 节点体系（common/ast/AstNode）。AST 是 DensityFunction 树的
 * 中间表示，与现有 DensityFunction 类解耦：McToAst 把 DF 树转成 AST，优化 pass 重写 AST，
 * 最后编译成扁平指令序列求值器。
 *
 * 节点不可变（const 字段 + shared_ptr<const> 持有），子节点用 shared_ptr<const AstNode>
 * 共享所有权——这使 persistence sharing（transform 时未改动的子树原样保留引用）与
 * relaxedEquals 子树去重（同结构子树共享同一编译产物）成为可能。
 *
 * 继承 std::enable_shared_from_this：transform 是 const 成员，需在子节点未变时返回指向
 * *this 的 shared_ptr 以保留原引用（persistence sharing 的 C++ 正确实现，避免每轮 pass
 * 重建所有节点）。要求节点恒由 shared_ptr 持有（McToAst 产物全程 make_shared，满足）。
 *
 * relaxedEquals/relaxedHashCode 语义（DFC 核心，子树去重的基础）：
 * - "会被编译为求值器字段"的数据（噪声实例、缓存对象、Delegate 原版 DF）只按 class 类型比较，
 *   不比值——因为编译时这些作为运行期字段传入，不同实例只要类型相同就生成同一段指令序列。
 * - "内联到指令"的纯标量（坐标、阈值、常量值、缩放系数）必须按值比较。
 *
 * 注：DFC 的 MaxShortNode/MinShortNode/NegMulNode 在基类 relaxedEquals 里漏比了 rightMax/
 * rightMin/negMul 标量（疑似 bug），靠"标量作编译类字段"兜底。Cubium 扁平指令序列须显式
 * 把这些标量作指令操作数，故本实现把它们纳入 relaxedEquals 比较（修正 DFC bug，避免去重
 * 错误合并不同 rightMax/negMul 的节点）。
 */
class AstNode : public std::enable_shared_from_this<AstNode> {
public:
    using Ptr = std::shared_ptr<const AstNode>;

    virtual ~AstNode() = default;

    /// 节点类型标识（编译期 switch 分发用）。
    [[nodiscard]] virtual AstNodeKind kind() const = 0;

    /// 求值返回类型（默认 F64，F32 仅 ConstantF32/Spline 等）。
    [[nodiscard]] virtual ReturnType returnType() const { return ReturnType::F64; }

    /// 子节点列表（transform 自底向上递归遍历用）。叶子节点返回空。
    [[nodiscard]] virtual std::vector<Ptr> children() const = 0;

    /**
     * @brief 自底向上递归 transform。
     *
     * 先递归 transform 所有子节点；若所有子节点引用未变（== 比较）则把 this 传给 transformer，
     * 否则用新子节点重建后再传给 transformer。未改动的子树原样保留引用（persistence sharing）。
     */
    [[nodiscard]] virtual Ptr transform(AstTransformer& transformer) const = 0;

    /// 宽松相等：拓扑 + 节点类型 + 内联标量比较，编译期字段（噪声/缓存/Delegate）只比类型。
    /// 用于子树去重——同 relaxedEquals 的子树可共享同一段编译产物。
    [[nodiscard]] virtual bool relaxedEquals(const AstNode& other) const = 0;

    /// 宽松哈希：与 relaxedEquals 配套，仅纳入 relaxedEquals 比较的字段。
    [[nodiscard]] virtual size_t relaxedHashCode() const = 0;

    /// 严格相等（全字段比较，含编译期字段实例身份）。用于跨 root 公共子树提取（TreeUtils）。
    [[nodiscard]] virtual bool equals(const AstNode& other) const = 0;
};

} // namespace mc::world::gen::density::ast
