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

#include "common/core/Types.hpp"
#include "common/world/gen/density/DensityFunctions.hpp" // MarkerType / SplinePoint / CubicSpline
#include "common/world/gen/noise/NormalNoise.hpp"

#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace mc::world::gen::density {
class DensityFunction;
class Beardifier;
} // namespace mc::world::gen::density

namespace mc::world::gen::density::ast {

// ============================================================================
// 辅助类型
// ============================================================================

/// 坐标轴标识（CoordinateNode 用）。
enum class Axis : u8 { X, Y, Z };

/// Marker 类型（复用 DensityFunctions.hpp 的 MarkerType，避免重复定义）。
using MarkerType = ::mc::world::gen::density::MarkerType;

/// WeirdScaledSampler 稀有度类型（Type1 maxRarity=2.0 / Type2 maxRarity=3.0）。
enum class WeirdType : u8 { Type1, Type2 };

// ============================================================================
// binary 节点（两子节点运算）— 对齐 DFC AbstractBinaryNode
// ============================================================================

/// 二元运算节点抽象基类。持 left/right 子节点。
/// relaxedEquals = class + left/right relaxedEquals（无额外字段；MaxShort/MinShort 子类修正）。
class BinaryNode : public AstNode {
public:
    BinaryNode(Ptr left, Ptr right)
        : m_left(std::move(left))
        , m_right(std::move(right))
    {}

    [[nodiscard]] const Ptr& left() const { return m_left; }
    [[nodiscard]] const Ptr& right() const { return m_right; }

    [[nodiscard]] std::vector<Ptr> children() const override { return {m_left, m_right}; }

    /// 可交换性（TreeNormalization 把常量规范到左边用）。Add/Mul/Div 可交换，Min/Max 不可。
    [[nodiscard]] virtual bool canSwapOperandsSafely() const { return true; }

    /// 用新子节点重建同类节点（transform 重建用）。
    [[nodiscard]] virtual Ptr newInstance(Ptr left, Ptr right) const = 0;

    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

protected:
    Ptr m_left;
    Ptr m_right;
};

#define MC_AST_BINARY_NODE(Name, CanSwap)                                  \
    class Name##Node final : public BinaryNode {                           \
    public:                                                                \
        Name##Node(Ptr left, Ptr right)                                    \
            : BinaryNode(std::move(left), std::move(right))                \
        {}                                                                 \
        [[nodiscard]] AstNodeKind kind() const override                    \
        {                                                                  \
            return AstNodeKind::Name;                                      \
        }                                                                  \
        [[nodiscard]] bool canSwapOperandsSafely() const override          \
        {                                                                  \
            return CanSwap;                                                \
        }                                                                  \
        [[nodiscard]] Ptr newInstance(Ptr left, Ptr right) const override; \
        [[nodiscard]] Ptr transform(AstTransformer& t) const override;     \
    }

MC_AST_BINARY_NODE(Add, true);
MC_AST_BINARY_NODE(Mul, true);
MC_AST_BINARY_NODE(Div, true);
MC_AST_BINARY_NODE(Max, false);
MC_AST_BINARY_NODE(Min, false);
#undef MC_AST_BINARY_NODE

/// 带右操作数上界短路的 Max（对齐 DFC MaxShortNode）。
/// compute = right.maxValue() <= rightMax ? max(left, right) : max(left, rightMax)
/// 修正 DFC bug：rightMax 纳入 relaxedEquals 比较（标量内联到指令操作数）。
class MaxShortNode final : public BinaryNode {
public:
    MaxShortNode(Ptr left, Ptr right, f64 rightMax)
        : BinaryNode(std::move(left), std::move(right))
        , m_rightMax(rightMax)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::MaxShort; }
    [[nodiscard]] bool canSwapOperandsSafely() const override { return false; }
    [[nodiscard]] f64 rightMax() const { return m_rightMax; }
    [[nodiscard]] Ptr newInstance(Ptr left, Ptr right) const override;
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    f64 m_rightMax;
};

/// 带右操作数下界短路的 Min（对齐 DFC MinShortNode）。
/// 修正 DFC bug：rightMin 纳入 relaxedEquals 比较。
class MinShortNode final : public BinaryNode {
public:
    MinShortNode(Ptr left, Ptr right, f64 rightMin)
        : BinaryNode(std::move(left), std::move(right))
        , m_rightMin(rightMin)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::MinShort; }
    [[nodiscard]] bool canSwapOperandsSafely() const override { return false; }
    [[nodiscard]] f64 rightMin() const { return m_rightMin; }
    [[nodiscard]] Ptr newInstance(Ptr left, Ptr right) const override;
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    f64 m_rightMin;
};

// ============================================================================
// unary 节点（单子节点运算）— 对齐 DFC AbstractUnaryNode
// ============================================================================

/// 一元运算节点抽象基类。持 operand 子节点。
class UnaryNode : public AstNode {
public:
    explicit UnaryNode(Ptr operand)
        : m_operand(std::move(operand))
    {}

    [[nodiscard]] const Ptr& operand() const { return m_operand; }

    [[nodiscard]] std::vector<Ptr> children() const override { return {m_operand}; }
    [[nodiscard]] virtual Ptr newInstance(Ptr operand) const = 0;

    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

protected:
    Ptr m_operand;
};

#define MC_AST_UNARY_NODE(Name)                                        \
    class Name##Node final : public UnaryNode {                        \
    public:                                                            \
        explicit Name##Node(Ptr operand)                               \
            : UnaryNode(std::move(operand))                            \
        {}                                                             \
        [[nodiscard]] AstNodeKind kind() const override                \
        {                                                              \
            return AstNodeKind::Name;                                  \
        }                                                              \
        [[nodiscard]] Ptr newInstance(Ptr operand) const override;     \
        [[nodiscard]] Ptr transform(AstTransformer& t) const override; \
    }

MC_AST_UNARY_NODE(Abs);
MC_AST_UNARY_NODE(Square);
MC_AST_UNARY_NODE(Cube);
MC_AST_UNARY_NODE(Squeeze);
MC_AST_UNARY_NODE(Sqrt);
MC_AST_UNARY_NODE(Sin);
MC_AST_UNARY_NODE(Cos);
MC_AST_UNARY_NODE(Floor);
MC_AST_UNARY_NODE(Ceil);
#undef MC_AST_UNARY_NODE

/// NegMul 节点（对齐 DFC NegMulNode）。compute = input <= 0 ? input * negMul : input。
/// 修正 DFC bug：negMul 纳入 relaxedEquals 比较（标量内联到指令操作数）。
class NegMulNode final : public UnaryNode {
public:
    NegMulNode(Ptr operand, f64 negMul)
        : UnaryNode(std::move(operand))
        , m_negMul(negMul)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::NegMul; }
    [[nodiscard]] f64 negMul() const { return m_negMul; }
    [[nodiscard]] Ptr newInstance(Ptr operand) const override;
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    f64 m_negMul;
};

// ============================================================================
// leaf 节点（无子节点）
// ============================================================================

/// F64 常量。relaxedEquals/equals 均按 value 比较（标量内联到指令）。
class ConstantNode final : public AstNode {
public:
    explicit ConstantNode(f64 value)
        : m_value(value)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Constant; }
    [[nodiscard]] f64 value() const { return m_value; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    f64 m_value;
};

/// F32 常量（样条 FixedFloatFunction）。returnType=F32。
class ConstantF32Node final : public AstNode {
public:
    explicit ConstantF32Node(f32 value)
        : m_value(value)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::ConstantF32; }
    [[nodiscard]] ReturnType returnType() const override { return ReturnType::F32; }
    [[nodiscard]] f32 value() const { return m_value; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    f32 m_value;
};

/// 坐标轴节点（X/Y/Z）。relaxedEquals/equals 按 axis 比较。
class CoordinateNode final : public AstNode {
public:
    explicit CoordinateNode(Axis axis)
        : m_axis(axis)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Coordinate; }
    [[nodiscard]] Axis axis() const { return m_axis; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Axis m_axis;
};

/// YClampedGradient。relaxedEquals/equals 按 fromY/toY/fromValue/toValue 全比值。
class YClampedGradientNode final : public AstNode {
public:
    YClampedGradientNode(i32 fromY, i32 toY, f64 fromValue, f64 toValue)
        : m_fromY(fromY)
        , m_toY(toY)
        , m_fromValue(fromValue)
        , m_toValue(toValue)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::YClampedGradient; }
    [[nodiscard]] i32 fromY() const { return m_fromY; }
    [[nodiscard]] i32 toY() const { return m_toY; }
    [[nodiscard]] f64 fromValue() const { return m_fromValue; }
    [[nodiscard]] f64 toValue() const { return m_toValue; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    i32 m_fromY;
    i32 m_toY;
    f64 m_fromValue;
    f64 m_toValue;
};

/// EndIslands（末地岛屿）。持 EndIslands 实例裸指针（非拥有，维度级 DF 树在 RandomState 存活期
/// 覆盖 AST 生命周期，同生命周期安全）。relaxedEquals 按 instance 比值（EndIslands 持 SimplexNoise，
/// 不同 seed 产生不同结果，须按实例区分）。
/// 注：正常路径 EndIslands 总被 SharedTopology 包装，McToAst 在边界发 SharedSubtreeRefNode，
/// 裸 EndIslandsNode 仅在未共享时退化路径产生。
class EndIslandsNode final : public AstNode {
public:
    explicit EndIslandsNode(const ::mc::world::gen::density::DensityFunction* endIslands)
        : m_endIslands(endIslands)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::EndIslands; }
    [[nodiscard]] const ::mc::world::gen::density::DensityFunction* endIslands() const { return m_endIslands; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    const ::mc::world::gen::density::DensityFunction* m_endIslands;
};

// ============================================================================
// noise 节点（噪声采样）
// ============================================================================

/// 通用移位噪声节点（覆盖 Noise/ShiftedNoise/Shift/ShiftA/ShiftB）。
/// 坐标变换在 McToAst 层展开为 inputX/inputY/inputZ 子 AST（Add/Mul/Coordinate 组合）。
/// noise 实例为裸指针（非拥有，NormalNoise 由 RandomState 持 shared_ptr 跨区块共享，生命周期覆盖 AST），
/// relaxedEquals 按 noise 实例地址比值（不同噪声参数产生不同结果）。
class GenericShiftedNoiseNode final : public AstNode {
public:
    GenericShiftedNoiseNode(Ptr inputX, Ptr inputY, Ptr inputZ, const ::mc::world::gen::noise::NormalNoise* noise)
        : m_inputX(std::move(inputX))
        , m_inputY(std::move(inputY))
        , m_inputZ(std::move(inputZ))
        , m_noise(noise)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::GenericShiftedNoise; }
    [[nodiscard]] const Ptr& inputX() const { return m_inputX; }
    [[nodiscard]] const Ptr& inputY() const { return m_inputY; }
    [[nodiscard]] const Ptr& inputZ() const { return m_inputZ; }
    [[nodiscard]] const ::mc::world::gen::noise::NormalNoise* noise() const { return m_noise; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_inputX, m_inputY, m_inputZ}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_inputX;
    Ptr m_inputY;
    Ptr m_inputZ;
    const ::mc::world::gen::noise::NormalNoise* m_noise;
};

/// WeirdScaledSampler。compute = abs(noise(x/rarity, y/rarity, z/rarity)) * rarity，
/// rarity = getRarity(input)（Type1/Type2 不同 rarity 表）。持 input 子节点 + noise 裸指针。
class WeirdScaledSamplerNode final : public AstNode {
public:
    WeirdScaledSamplerNode(Ptr input, const ::mc::world::gen::noise::NormalNoise* noise, WeirdType type)
        : m_input(std::move(input))
        , m_noise(noise)
        , m_type(type)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::WeirdScaledSampler; }
    [[nodiscard]] const Ptr& input() const { return m_input; }
    [[nodiscard]] const ::mc::world::gen::noise::NormalNoise* noise() const { return m_noise; }
    [[nodiscard]] WeirdType type() const { return m_type; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_input}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_input;
    const ::mc::world::gen::noise::NormalNoise* m_noise;
    WeirdType m_type;
};

/// BlendedNoise（旧式三层 Perlin 噪声）。持 BlendedNoise 实例裸指针（非拥有，维度级 DF 树在
/// RandomState 存活期覆盖 AST 生命周期，同生命周期安全）。relaxedEquals 按 instance 比值
/// （BlendedNoise 持 main/min/max 三层 PerlinNoise，不同 seed/参数产生不同密度，须按实例区分；
/// 主世界 0.25/0.125/80/160/8、下界 0.25/0.375/80/60/8、末地 0.25/0.25/80/160/4 各不同）。
/// 专用节点而非 DelegateNode 退化的目的：让 JIT trampoline 持具体类型 const BlendedNoise*，
/// 编译器对 final 类 BlendedNoise 的虚调用 compute() 去虚化为直接 call，消除 vtable 间接。
class BlendedNoiseNode final : public AstNode {
public:
    explicit BlendedNoiseNode(const ::mc::world::gen::density::DensityFunction* blendedNoise)
        : m_blendedNoise(blendedNoise)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::BlendedNoise; }
    [[nodiscard]] const ::mc::world::gen::density::DensityFunction* blendedNoise() const { return m_blendedNoise; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    const ::mc::world::gen::density::DensityFunction* m_blendedNoise;
};

// TODO: MappedNoise 节点在阶段 2 McToAst 接入时按需补充（出现频率低，可先走 DelegateNode 退化，
// 确认主世界/下界/末地噪声树覆盖后再定是否需要专用节点）。

// ============================================================================
// spline 节点
// ============================================================================

/// 样条节点。locationFunction 是输入轴子 AST；每个控制点的 value 若是嵌套子样条，
/// 递归转为子 SplineNode（作为额外 children）。locations/derivatives 是标量数组（比值）。
/// returnType=F32（MC 样条走 float 算术）。
class SplineNode final : public AstNode {
public:
    /// 控制点：标量 location/derivative + value（f64 常量或嵌套子样条 AST 索引）。
    struct Point {
        f64 location;
        f64 derivative;
        std::variant<f64, Ptr> value; // f64 常量 或 嵌套子样条 AST
    };

    SplineNode(Ptr locationFunction, std::vector<Point> points)
        : m_locationFunction(std::move(locationFunction))
        , m_points(std::move(points))
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Spline; }
    [[nodiscard]] ReturnType returnType() const override { return ReturnType::F32; }
    [[nodiscard]] const Ptr& locationFunction() const { return m_locationFunction; }
    [[nodiscard]] const std::vector<Point>& points() const { return m_points; }
    [[nodiscard]] std::vector<Ptr> children() const override;
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_locationFunction;
    std::vector<Point> m_points;
};

// ============================================================================
// control 节点（条件选择）
// ============================================================================

/// RangeChoice。input∈[minInclusive, maxExclusive)→whenInRange，否则 whenOutOfRange。
/// 标量 minInclusive/maxExclusive 比值，子树 relaxedEquals。
class RangeChoiceNode final : public AstNode {
public:
    RangeChoiceNode(Ptr input, f64 minInclusive, f64 maxExclusive, Ptr whenInRange, Ptr whenOutOfRange)
        : m_input(std::move(input))
        , m_minInclusive(minInclusive)
        , m_maxExclusive(maxExclusive)
        , m_whenInRange(std::move(whenInRange))
        , m_whenOutOfRange(std::move(whenOutOfRange))
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::RangeChoice; }
    [[nodiscard]] const Ptr& input() const { return m_input; }
    [[nodiscard]] f64 minInclusive() const { return m_minInclusive; }
    [[nodiscard]] f64 maxExclusive() const { return m_maxExclusive; }
    [[nodiscard]] const Ptr& whenInRange() const { return m_whenInRange; }
    [[nodiscard]] const Ptr& whenOutOfRange() const { return m_whenOutOfRange; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_input, m_whenInRange, m_whenOutOfRange}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_input;
    f64 m_minInclusive;
    f64 m_maxExclusive;
    Ptr m_whenInRange;
    Ptr m_whenOutOfRange;
};

/// Clamp。compute = std::clamp(input, min, max)。min/max 标量比值，input 子树 relaxedEquals。
class ClampNode final : public AstNode {
public:
    ClampNode(Ptr input, f64 min, f64 max)
        : m_input(std::move(input))
        , m_min(min)
        , m_max(max)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Clamp; }
    [[nodiscard]] const Ptr& input() const { return m_input; }
    [[nodiscard]] f64 min() const { return m_min; }
    [[nodiscard]] f64 max() const { return m_max; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_input}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_input;
    f64 m_min;
    f64 m_max;
};

/// Lerp。compute = delta<=0?start : delta>=1?end : start+delta*(end-start)。
class LerpNode final : public AstNode {
public:
    LerpNode(Ptr delta, Ptr start, Ptr end)
        : m_delta(std::move(delta))
        , m_start(std::move(start))
        , m_end(std::move(end))
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Lerp; }
    [[nodiscard]] const Ptr& delta() const { return m_delta; }
    [[nodiscard]] const Ptr& start() const { return m_start; }
    [[nodiscard]] const Ptr& end() const { return m_end; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_delta, m_start, m_end}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_delta;
    Ptr m_start;
    Ptr m_end;
};

// ============================================================================
// marker / beardifier 节点（AST 保留 Marker 语义，区块级原地替换）
// ============================================================================

/// Marker 占位节点。维度级编译产物含 MarkerNode，区块级 newInstance 时原地替换为
/// per-chunk 缓存实现（对齐 DFC c2me$withDelegate）。持被包装的 delegate 子 AST。
/// relaxedEquals = class + markerType 比值 + delegate relaxedEquals（占位实例忽略）。
///
/// m_delegateMinValue/m_delegateMaxValue 是编译期元数据（取自原 DF wrapped 的 minValue/maxValue，
/// Marker 透传故 == delegate 的 min/max），供 BytecodeGen 把 delegate 子树独立编译为子求值器时
/// 记录到 CompiledDensityFunction（经 Adapter→缓存类 minValue/maxValue 消费）。不参与
/// relaxedEquals/equals/hashCode（纯元数据，不影响拓扑去重——两 Marker 即便 min/max 不同，
/// 只要 type+delegate relaxedEquals 相同就共享同段子程序，min/max 差异不改变指令序列）。
class MarkerNode final : public AstNode {
public:
    MarkerNode(MarkerType type, Ptr delegate, f64 delegateMinValue, f64 delegateMaxValue)
        : m_type(type)
        , m_delegate(std::move(delegate))
        , m_delegateMinValue(delegateMinValue)
        , m_delegateMaxValue(delegateMaxValue)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Marker; }
    [[nodiscard]] MarkerType markerType() const { return m_type; }
    [[nodiscard]] const Ptr& delegate() const { return m_delegate; }
    [[nodiscard]] f64 delegateMinValue() const { return m_delegateMinValue; }
    [[nodiscard]] f64 delegateMaxValue() const { return m_delegateMaxValue; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_delegate}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    MarkerType m_type;
    Ptr m_delegate;
    f64 m_delegateMinValue;
    f64 m_delegateMaxValue;
};

/// Beardifier 节点（Delegate 系）。持原版 Beardifier 实例引用，区块期注入结构数据。
/// relaxedEquals 按 class 比较（Beardifier 实例区块特定，编译期不比值）。
class BeardifierNode final : public AstNode {
public:
    BeardifierNode() = default;

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Beardifier; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;
};

// ============================================================================
// Cubium 特有节点
// ============================================================================

/// SharedSubtreeRef 节点（SharedTopology 边界）。
/// Cubium 已有的纯拓扑子树跨区块共享优化在 AST 层的体现：内部子树独立编译为一个可复用
/// 求值器，本节点持其索引，父节点通过索引调用。relaxedEquals 按 subTreeId 比值
/// （不同 SharedTopology 子树有不同 id，避免错误合并）。
///
/// m_inner 持内部子树转成的 AST（McToAst 在 SharedTopology 边界把 inner() 递归转换），
/// 供 BytecodeGen 独立编译为子求值器。m_inner 不纳入 children()——内部子树是共享不可变的，
/// 不参与 OptoPasses 重写（保持跨区块共享语义；优化 inner 留 TODO）。relaxedEquals 仍只比
/// subTreeId（subTreeId 唯一标识一棵共享子树，inner 必然相同）。
class SharedSubtreeRefNode final : public AstNode {
public:
    SharedSubtreeRefNode(u64 subTreeId, Ptr inner)
        : m_subTreeId(subTreeId)
        , m_inner(std::move(inner))
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::SharedSubtreeRef; }
    [[nodiscard]] u64 subTreeId() const { return m_subTreeId; }
    [[nodiscard]] const Ptr& inner() const { return m_inner; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    u64 m_subTreeId;
    Ptr m_inner;
};

/// FindTopSurface 节点（循环搜索）。从 floor(upperBound/cellH)*cellH 向下逐 cellH 步
/// 找首个 density>0 的 Y。持 density/upperBound 子节点 + lowerBound/cellHeight 标量。
class FindTopSurfaceNode final : public AstNode {
public:
    FindTopSurfaceNode(Ptr density, Ptr upperBound, i32 lowerBound, i32 cellHeight)
        : m_density(std::move(density))
        , m_upperBound(std::move(upperBound))
        , m_lowerBound(lowerBound)
        , m_cellHeight(cellHeight)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::FindTopSurface; }
    [[nodiscard]] const Ptr& density() const { return m_density; }
    [[nodiscard]] const Ptr& upperBound() const { return m_upperBound; }
    [[nodiscard]] i32 lowerBound() const { return m_lowerBound; }
    [[nodiscard]] i32 cellHeight() const { return m_cellHeight; }
    [[nodiscard]] std::vector<Ptr> children() const override { return {m_density, m_upperBound}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    Ptr m_density;
    Ptr m_upperBound;
    i32 m_lowerBound;
    i32 m_cellHeight;
};

// ============================================================================
// delegate 节点（未识别 DF 退化）
// ============================================================================

/// Delegate 节点。包原版 DensityFunction 裸指针（非拥有，维度级 DF 树覆盖 AST 生命周期），
/// 运行时回调 compute。保证未适配的 DF 仍可求值。
/// relaxedEquals 按 class 比较（typeid 动态类型，实例忽略，编译为求值器字段）。
/// equals 严格按实例地址比较（跨 root 公共子树提取用）。
class DelegateNode final : public AstNode {
public:
    explicit DelegateNode(const ::mc::world::gen::density::DensityFunction* densityFunction)
        : m_densityFunction(densityFunction)
    {}

    [[nodiscard]] AstNodeKind kind() const override { return AstNodeKind::Delegate; }
    [[nodiscard]] const ::mc::world::gen::density::DensityFunction* densityFunction() const
    {
        return m_densityFunction;
    }
    [[nodiscard]] std::vector<Ptr> children() const override { return {}; }
    [[nodiscard]] Ptr transform(AstTransformer& t) const override;
    [[nodiscard]] bool relaxedEquals(const AstNode& other) const override;
    [[nodiscard]] size_t relaxedHashCode() const override;
    [[nodiscard]] bool equals(const AstNode& other) const override;

private:
    const ::mc::world::gen::density::DensityFunction* m_densityFunction;
};

} // namespace mc::world::gen::density::ast
