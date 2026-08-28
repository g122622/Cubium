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

#include "common/world/gen/density/ast/AstNodes.hpp"

#include <functional> // std::hash
#include <typeinfo>   // typeid（DelegateNode relaxedEquals 比原版 DF 动态类型）
#include <utility>

namespace mc::world::gen::density::ast {

// AstNode::Ptr 是类内 public 类型别名；命名空间作用域（自由函数与宏展开的成员定义）无法直接
// 通过继承访问，故在此命名空间层建立等价别名，供自由函数与宏展开的返回/参数类型使用。
// 各成员函数定义体内仍用此别名（命名空间层可见）。
using Ptr = std::shared_ptr<const AstNode>;

namespace {

/// boost::hash_combine 风格哈希组合（项目无统一工具，沿用 SectionKey.hpp:198 先例）。
void hashCombine(size_t& h, size_t value) noexcept
{
    h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
}

/// 便利重载：对任意可哈希类型组合。
template <typename T>
void hashCombine(size_t& h, const T& value) noexcept
{
    hashCombine(h, std::hash<T>{}(value));
}

/// f64 哈希：位级哈希（NaN 不参与 AST 语义故无需特殊处理）。
size_t hashF64(f64 v) noexcept
{
    return std::hash<f64>{}(v);
}

/// relaxedEquals 两子节点辅助：先比地址（同实例直接相等），不同则比 relaxedEquals。
bool childrenRelaxedEqual(const Ptr& a, const Ptr& b) noexcept
{
    if (a == b) {
        return true;
    }
    if (!a || !b) {
        return false;
    }
    return a->relaxedEquals(*b);
}

/// equals 两子节点辅助：同地址直接相等，否则严格 equals。
bool childrenStrictEqual(const Ptr& a, const Ptr& b) noexcept
{
    if (a == b) {
        return true;
    }
    if (!a || !b) {
        return false;
    }
    return a->equals(*b);
}

} // namespace

// ============================================================================
// BinaryNode（二元节点基类）
// ============================================================================
// transform 实现（persistence sharing）：先递归 transform 子节点；若所有子节点引用未变，
// 用 shared_from_this() 返回原节点引用交 transformer；否则用新子节点重建后再交 transformer。
// shared_from_this() 要求本对象由 shared_ptr 持有——AST 节点恒由 make_shared 构造，满足。

bool BinaryNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const BinaryNode&>(other);
    return childrenRelaxedEqual(m_left, o.m_left) && childrenRelaxedEqual(m_right, o.m_right);
}

size_t BinaryNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    if (m_left) {
        hashCombine(h, m_left->relaxedHashCode());
    }
    if (m_right) {
        hashCombine(h, m_right->relaxedHashCode());
    }
    return h;
}

bool BinaryNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const BinaryNode&>(other);
    return childrenStrictEqual(m_left, o.m_left) && childrenStrictEqual(m_right, o.m_right);
}

// 无标量二元节点（Add/Mul/Div/Max/Min）的 newInstance/transform。
#define MC_AST_BINARY_NODE_IMPL(Name)                                             \
    Ptr Name##Node::newInstance(Ptr left, Ptr right) const                        \
    {                                                                             \
        return std::make_shared<Name##Node>(std::move(left), std::move(right));   \
    }                                                                             \
    Ptr Name##Node::transform(AstTransformer& t) const                            \
    {                                                                             \
        Ptr newLeft = m_left ? m_left->transform(t) : m_left;                     \
        Ptr newRight = m_right ? m_right->transform(t) : m_right;                 \
        if (newLeft == m_left && newRight == m_right) {                           \
            return t.transform(shared_from_this());                               \
        }                                                                         \
        return t.transform(newInstance(std::move(newLeft), std::move(newRight))); \
    }

MC_AST_BINARY_NODE_IMPL(Add)
MC_AST_BINARY_NODE_IMPL(Mul)
MC_AST_BINARY_NODE_IMPL(Div)
MC_AST_BINARY_NODE_IMPL(Max)
MC_AST_BINARY_NODE_IMPL(Min)
#undef MC_AST_BINARY_NODE_IMPL

// ============================================================================
// MaxShortNode / MinShortNode（带标量的二元节点）
// 修正 DFC bug：rightMax/rightMin 纳入 relaxedEquals 与 hashCode（标量内联到指令操作数）。
// ============================================================================

Ptr MaxShortNode::newInstance(Ptr left, Ptr right) const
{
    return std::make_shared<MaxShortNode>(std::move(left), std::move(right), m_rightMax);
}

Ptr MaxShortNode::transform(AstTransformer& t) const
{
    Ptr newLeft = m_left ? m_left->transform(t) : m_left;
    Ptr newRight = m_right ? m_right->transform(t) : m_right;
    if (newLeft == m_left && newRight == m_right) {
        return t.transform(shared_from_this());
    }
    return t.transform(newInstance(std::move(newLeft), std::move(newRight)));
}

bool MaxShortNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const MaxShortNode&>(other);
    return m_rightMax == o.m_rightMax && childrenRelaxedEqual(m_left, o.m_left) &&
        childrenRelaxedEqual(m_right, o.m_right);
}

size_t MaxShortNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, hashF64(m_rightMax));
    if (m_left) {
        hashCombine(h, m_left->relaxedHashCode());
    }
    if (m_right) {
        hashCombine(h, m_right->relaxedHashCode());
    }
    return h;
}

bool MaxShortNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const MaxShortNode&>(other);
    return m_rightMax == o.m_rightMax && childrenStrictEqual(m_left, o.m_left) &&
        childrenStrictEqual(m_right, o.m_right);
}

Ptr MinShortNode::newInstance(Ptr left, Ptr right) const
{
    return std::make_shared<MinShortNode>(std::move(left), std::move(right), m_rightMin);
}

Ptr MinShortNode::transform(AstTransformer& t) const
{
    Ptr newLeft = m_left ? m_left->transform(t) : m_left;
    Ptr newRight = m_right ? m_right->transform(t) : m_right;
    if (newLeft == m_left && newRight == m_right) {
        return t.transform(shared_from_this());
    }
    return t.transform(newInstance(std::move(newLeft), std::move(newRight)));
}

bool MinShortNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const MinShortNode&>(other);
    return m_rightMin == o.m_rightMin && childrenRelaxedEqual(m_left, o.m_left) &&
        childrenRelaxedEqual(m_right, o.m_right);
}

size_t MinShortNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, hashF64(m_rightMin));
    if (m_left) {
        hashCombine(h, m_left->relaxedHashCode());
    }
    if (m_right) {
        hashCombine(h, m_right->relaxedHashCode());
    }
    return h;
}

bool MinShortNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const MinShortNode&>(other);
    return m_rightMin == o.m_rightMin && childrenStrictEqual(m_left, o.m_left) &&
        childrenStrictEqual(m_right, o.m_right);
}

// ============================================================================
// UnaryNode（一元节点基类）
// ============================================================================

bool UnaryNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const UnaryNode&>(other);
    return childrenRelaxedEqual(m_operand, o.m_operand);
}

size_t UnaryNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    if (m_operand) {
        hashCombine(h, m_operand->relaxedHashCode());
    }
    return h;
}

bool UnaryNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const UnaryNode&>(other);
    return childrenStrictEqual(m_operand, o.m_operand);
}

#define MC_AST_UNARY_NODE_IMPL(Name)                                      \
    Ptr Name##Node::newInstance(Ptr operand) const                        \
    {                                                                     \
        return std::make_shared<Name##Node>(std::move(operand));          \
    }                                                                     \
    Ptr Name##Node::transform(AstTransformer& t) const                    \
    {                                                                     \
        Ptr newOperand = m_operand ? m_operand->transform(t) : m_operand; \
        if (newOperand == m_operand) {                                    \
            return t.transform(shared_from_this());                       \
        }                                                                 \
        return t.transform(newInstance(std::move(newOperand)));           \
    }

MC_AST_UNARY_NODE_IMPL(Abs)
MC_AST_UNARY_NODE_IMPL(Square)
MC_AST_UNARY_NODE_IMPL(Cube)
MC_AST_UNARY_NODE_IMPL(Squeeze)
MC_AST_UNARY_NODE_IMPL(Sqrt)
MC_AST_UNARY_NODE_IMPL(Sin)
MC_AST_UNARY_NODE_IMPL(Cos)
MC_AST_UNARY_NODE_IMPL(Floor)
MC_AST_UNARY_NODE_IMPL(Ceil)
#undef MC_AST_UNARY_NODE_IMPL

// ============================================================================
// NegMulNode（带标量的一元节点）
// 修正 DFC bug：negMul 纳入 relaxedEquals 与 hashCode。
// ============================================================================

Ptr NegMulNode::newInstance(Ptr operand) const
{
    return std::make_shared<NegMulNode>(std::move(operand), m_negMul);
}

Ptr NegMulNode::transform(AstTransformer& t) const
{
    Ptr newOperand = m_operand ? m_operand->transform(t) : m_operand;
    if (newOperand == m_operand) {
        return t.transform(shared_from_this());
    }
    return t.transform(newInstance(std::move(newOperand)));
}

bool NegMulNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const NegMulNode&>(other);
    return m_negMul == o.m_negMul && childrenRelaxedEqual(m_operand, o.m_operand);
}

size_t NegMulNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, hashF64(m_negMul));
    if (m_operand) {
        hashCombine(h, m_operand->relaxedHashCode());
    }
    return h;
}

bool NegMulNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const NegMulNode&>(other);
    return m_negMul == o.m_negMul && childrenStrictEqual(m_operand, o.m_operand);
}

// ============================================================================
// ConstantNode（F64 常量）— 标量内联到指令，relaxedEquals/equals 均按值。
// ============================================================================

Ptr ConstantNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool ConstantNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    return m_value == static_cast<const ConstantNode&>(other).m_value;
}

size_t ConstantNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, hashF64(m_value));
    return h;
}

bool ConstantNode::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// ConstantF32Node（F32 常量）
// ============================================================================

Ptr ConstantF32Node::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool ConstantF32Node::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    return m_value == static_cast<const ConstantF32Node&>(other).m_value;
}

size_t ConstantF32Node::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, std::hash<f32>{}(m_value));
    return h;
}

bool ConstantF32Node::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// CoordinateNode（坐标轴）
// ============================================================================

Ptr CoordinateNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool CoordinateNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    return m_axis == static_cast<const CoordinateNode&>(other).m_axis;
}

size_t CoordinateNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, static_cast<u8>(m_axis));
    return h;
}

bool CoordinateNode::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// YClampedGradientNode — 全标量，relaxedEquals/equals 均按值。
// ============================================================================

Ptr YClampedGradientNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool YClampedGradientNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const YClampedGradientNode&>(other);
    return m_fromY == o.m_fromY && m_toY == o.m_toY && m_fromValue == o.m_fromValue && m_toValue == o.m_toValue;
}

size_t YClampedGradientNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, static_cast<i64>(m_fromY));
    hashCombine(h, static_cast<i64>(m_toY));
    hashCombine(h, hashF64(m_fromValue));
    hashCombine(h, hashF64(m_toValue));
    return h;
}

bool YClampedGradientNode::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// EndIslandsNode（持原版 EndIslands DF 裸指针）
// relaxedEquals 按 instance 比值（指针地址）：EndIslands 持 SimplexNoise，不同 seed 产生不同密度，不可合并。
// ============================================================================

Ptr EndIslandsNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool EndIslandsNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    return m_endIslands == static_cast<const EndIslandsNode&>(other).m_endIslands;
}

size_t EndIslandsNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, reinterpret_cast<size_t>(m_endIslands));
    return h;
}

bool EndIslandsNode::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// BlendedNoiseNode
// ============================================================================

Ptr BlendedNoiseNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool BlendedNoiseNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    return m_blendedNoise == static_cast<const BlendedNoiseNode&>(other).m_blendedNoise;
}

size_t BlendedNoiseNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, reinterpret_cast<size_t>(m_blendedNoise));
    return h;
}

bool BlendedNoiseNode::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// GenericShiftedNoiseNode（噪声采样）
// relaxedEquals：noise 实例按地址比值（不同噪声参数产生不同结果，不可合并）+ 坐标子树 relaxedEquals。
// ============================================================================

Ptr GenericShiftedNoiseNode::transform(AstTransformer& t) const
{
    Ptr newX = m_inputX ? m_inputX->transform(t) : m_inputX;
    Ptr newY = m_inputY ? m_inputY->transform(t) : m_inputY;
    Ptr newZ = m_inputZ ? m_inputZ->transform(t) : m_inputZ;
    if (newX == m_inputX && newY == m_inputY && newZ == m_inputZ) {
        return t.transform(shared_from_this());
    }
    return t.transform(
        std::make_shared<GenericShiftedNoiseNode>(std::move(newX), std::move(newY), std::move(newZ), m_noise));
}

bool GenericShiftedNoiseNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const GenericShiftedNoiseNode&>(other);
    return m_noise == o.m_noise && childrenRelaxedEqual(m_inputX, o.m_inputX) &&
        childrenRelaxedEqual(m_inputY, o.m_inputY) && childrenRelaxedEqual(m_inputZ, o.m_inputZ);
}

size_t GenericShiftedNoiseNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, reinterpret_cast<size_t>(m_noise));
    if (m_inputX) {
        hashCombine(h, m_inputX->relaxedHashCode());
    }
    if (m_inputY) {
        hashCombine(h, m_inputY->relaxedHashCode());
    }
    if (m_inputZ) {
        hashCombine(h, m_inputZ->relaxedHashCode());
    }
    return h;
}

bool GenericShiftedNoiseNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const GenericShiftedNoiseNode&>(other);
    return m_noise == o.m_noise && childrenStrictEqual(m_inputX, o.m_inputX) &&
        childrenStrictEqual(m_inputY, o.m_inputY) && childrenStrictEqual(m_inputZ, o.m_inputZ);
}

// ============================================================================
// WeirdScaledSamplerNode
// relaxedEquals：noise 实例按地址比值 + type 比值 + input 子树 relaxedEquals。
// ============================================================================

Ptr WeirdScaledSamplerNode::transform(AstTransformer& t) const
{
    Ptr newInput = m_input ? m_input->transform(t) : m_input;
    if (newInput == m_input) {
        return t.transform(shared_from_this());
    }
    return t.transform(std::make_shared<WeirdScaledSamplerNode>(std::move(newInput), m_noise, m_type));
}

bool WeirdScaledSamplerNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const WeirdScaledSamplerNode&>(other);
    return m_noise == o.m_noise && m_type == o.m_type && childrenRelaxedEqual(m_input, o.m_input);
}

size_t WeirdScaledSamplerNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, reinterpret_cast<size_t>(m_noise));
    hashCombine(h, static_cast<u8>(m_type));
    if (m_input) {
        hashCombine(h, m_input->relaxedHashCode());
    }
    return h;
}

bool WeirdScaledSamplerNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const WeirdScaledSamplerNode&>(other);
    return m_noise == o.m_noise && m_type == o.m_type && childrenStrictEqual(m_input, o.m_input);
}

// ============================================================================
// SplineNode（样条）
// children = locationFunction + 各控制点中嵌套子样条 AST。
// relaxedEquals：locationFunction + points 逐点比较（location/derivative 比值，
// value 若均为常量比常量值，若均为子样条比 relaxedEquals，类型不同不等）。
// ============================================================================

std::vector<Ptr> SplineNode::children() const
{
    std::vector<Ptr> result;
    result.reserve(m_points.size() + 1);
    if (m_locationFunction) {
        result.push_back(m_locationFunction);
    }
    for (const auto& p : m_points) {
        if (auto* child = std::get_if<Ptr>(&p.value)) {
            if (*child) {
                result.push_back(*child);
            }
        }
    }
    return result;
}

Ptr SplineNode::transform(AstTransformer& t) const
{
    Ptr newLoc = m_locationFunction ? m_locationFunction->transform(t) : m_locationFunction;
    std::vector<Point> newPoints = m_points;
    bool changed = (newLoc != m_locationFunction);
    for (size_t i = 0; i < newPoints.size(); ++i) {
        if (auto* child = std::get_if<Ptr>(&newPoints[i].value)) {
            if (*child) {
                Ptr transformed = (*child)->transform(t);
                if (transformed != *child) {
                    newPoints[i].value = transformed;
                    changed = true;
                }
            }
        }
    }
    if (!changed) {
        return t.transform(shared_from_this());
    }
    return t.transform(std::make_shared<SplineNode>(std::move(newLoc), std::move(newPoints)));
}

namespace {
bool splinePointsRelaxedEqual(const SplineNode::Point& a, const SplineNode::Point& b) noexcept
{
    if (a.location != b.location || a.derivative != b.derivative) {
        return false;
    }
    if (a.value.index() != b.value.index()) {
        return false;
    }
    if (auto* va = std::get_if<f64>(&a.value)) {
        return *va == std::get<f64>(b.value);
    }
    return childrenRelaxedEqual(std::get<Ptr>(a.value), std::get<Ptr>(b.value));
}

bool splinePointsStrictEqual(const SplineNode::Point& a, const SplineNode::Point& b) noexcept
{
    if (a.location != b.location || a.derivative != b.derivative) {
        return false;
    }
    if (a.value.index() != b.value.index()) {
        return false;
    }
    if (auto* va = std::get_if<f64>(&a.value)) {
        return *va == std::get<f64>(b.value);
    }
    return childrenStrictEqual(std::get<Ptr>(a.value), std::get<Ptr>(b.value));
}
} // namespace

bool SplineNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const SplineNode&>(other);
    if (m_points.size() != o.m_points.size()) {
        return false;
    }
    if (!childrenRelaxedEqual(m_locationFunction, o.m_locationFunction)) {
        return false;
    }
    for (size_t i = 0; i < m_points.size(); ++i) {
        if (!splinePointsRelaxedEqual(m_points[i], o.m_points[i])) {
            return false;
        }
    }
    return true;
}

size_t SplineNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    if (m_locationFunction) {
        hashCombine(h, m_locationFunction->relaxedHashCode());
    }
    for (const auto& p : m_points) {
        hashCombine(h, hashF64(p.location));
        hashCombine(h, hashF64(p.derivative));
        if (auto* v = std::get_if<f64>(&p.value)) {
            hashCombine(h, hashF64(*v));
        } else if (auto* child = std::get_if<Ptr>(&p.value)) {
            if (*child) {
                hashCombine(h, (*child)->relaxedHashCode());
            }
        }
    }
    return h;
}

bool SplineNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const SplineNode&>(other);
    if (m_points.size() != o.m_points.size()) {
        return false;
    }
    if (!childrenStrictEqual(m_locationFunction, o.m_locationFunction)) {
        return false;
    }
    for (size_t i = 0; i < m_points.size(); ++i) {
        if (!splinePointsStrictEqual(m_points[i], o.m_points[i])) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// RangeChoiceNode（条件选择）
// ============================================================================

Ptr RangeChoiceNode::transform(AstTransformer& t) const
{
    Ptr newInput = m_input ? m_input->transform(t) : m_input;
    Ptr newIn = m_whenInRange ? m_whenInRange->transform(t) : m_whenInRange;
    Ptr newOut = m_whenOutOfRange ? m_whenOutOfRange->transform(t) : m_whenOutOfRange;
    if (newInput == m_input && newIn == m_whenInRange && newOut == m_whenOutOfRange) {
        return t.transform(shared_from_this());
    }
    return t.transform(std::make_shared<RangeChoiceNode>(
        std::move(newInput), m_minInclusive, m_maxExclusive, std::move(newIn), std::move(newOut)));
}

bool RangeChoiceNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const RangeChoiceNode&>(other);
    return m_minInclusive == o.m_minInclusive && m_maxExclusive == o.m_maxExclusive &&
        childrenRelaxedEqual(m_input, o.m_input) && childrenRelaxedEqual(m_whenInRange, o.m_whenInRange) &&
        childrenRelaxedEqual(m_whenOutOfRange, o.m_whenOutOfRange);
}

size_t RangeChoiceNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, hashF64(m_minInclusive));
    hashCombine(h, hashF64(m_maxExclusive));
    if (m_input) {
        hashCombine(h, m_input->relaxedHashCode());
    }
    if (m_whenInRange) {
        hashCombine(h, m_whenInRange->relaxedHashCode());
    }
    if (m_whenOutOfRange) {
        hashCombine(h, m_whenOutOfRange->relaxedHashCode());
    }
    return h;
}

bool RangeChoiceNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const RangeChoiceNode&>(other);
    return m_minInclusive == o.m_minInclusive && m_maxExclusive == o.m_maxExclusive &&
        childrenStrictEqual(m_input, o.m_input) && childrenStrictEqual(m_whenInRange, o.m_whenInRange) &&
        childrenStrictEqual(m_whenOutOfRange, o.m_whenOutOfRange);
}

// ============================================================================
// ClampNode
// ============================================================================

Ptr ClampNode::transform(AstTransformer& t) const
{
    Ptr newInput = m_input ? m_input->transform(t) : m_input;
    if (newInput == m_input) {
        return t.transform(shared_from_this());
    }
    return t.transform(std::make_shared<ClampNode>(std::move(newInput), m_min, m_max));
}

bool ClampNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const ClampNode&>(other);
    return m_min == o.m_min && m_max == o.m_max && childrenRelaxedEqual(m_input, o.m_input);
}

size_t ClampNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, hashF64(m_min));
    hashCombine(h, hashF64(m_max));
    if (m_input) {
        hashCombine(h, m_input->relaxedHashCode());
    }
    return h;
}

bool ClampNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const ClampNode&>(other);
    return m_min == o.m_min && m_max == o.m_max && childrenStrictEqual(m_input, o.m_input);
}

// ============================================================================
// LerpNode
// ============================================================================

Ptr LerpNode::transform(AstTransformer& t) const
{
    Ptr newDelta = m_delta ? m_delta->transform(t) : m_delta;
    Ptr newStart = m_start ? m_start->transform(t) : m_start;
    Ptr newEnd = m_end ? m_end->transform(t) : m_end;
    if (newDelta == m_delta && newStart == m_start && newEnd == m_end) {
        return t.transform(shared_from_this());
    }
    return t.transform(std::make_shared<LerpNode>(std::move(newDelta), std::move(newStart), std::move(newEnd)));
}

bool LerpNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const LerpNode&>(other);
    return childrenRelaxedEqual(m_delta, o.m_delta) && childrenRelaxedEqual(m_start, o.m_start) &&
        childrenRelaxedEqual(m_end, o.m_end);
}

size_t LerpNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    if (m_delta) {
        hashCombine(h, m_delta->relaxedHashCode());
    }
    if (m_start) {
        hashCombine(h, m_start->relaxedHashCode());
    }
    if (m_end) {
        hashCombine(h, m_end->relaxedHashCode());
    }
    return h;
}

bool LerpNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const LerpNode&>(other);
    return childrenStrictEqual(m_delta, o.m_delta) && childrenStrictEqual(m_start, o.m_start) &&
        childrenStrictEqual(m_end, o.m_end);
}

// ============================================================================
// MarkerNode（6 种 MarkerType 占位 + delegate 子树）
// relaxedEquals：markerType 比值 + delegate relaxedEquals（占位实例忽略——同类型同 delegate 即可合并）。
// ============================================================================

Ptr MarkerNode::transform(AstTransformer& t) const
{
    Ptr newDelegate = m_delegate ? m_delegate->transform(t) : m_delegate;
    if (newDelegate == m_delegate) {
        return t.transform(shared_from_this());
    }
    // 重建保留 min/max 元数据（pass 重写不改变 delegate 的数值范围，只改变计算方式）。
    return t.transform(
        std::make_shared<MarkerNode>(m_type, std::move(newDelegate), m_delegateMinValue, m_delegateMaxValue));
}

bool MarkerNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const MarkerNode&>(other);
    return m_type == o.m_type && childrenRelaxedEqual(m_delegate, o.m_delegate);
}

size_t MarkerNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, static_cast<u8>(m_type));
    if (m_delegate) {
        hashCombine(h, m_delegate->relaxedHashCode());
    }
    return h;
}

bool MarkerNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const MarkerNode&>(other);
    return m_type == o.m_type && childrenStrictEqual(m_delegate, o.m_delegate);
}

// ============================================================================
// BeardifierNode（无字段，relaxedEquals 仅比 class）
// ============================================================================

Ptr BeardifierNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool BeardifierNode::relaxedEquals(const AstNode& other) const
{
    return kind() == other.kind();
}

size_t BeardifierNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    return h;
}

bool BeardifierNode::equals(const AstNode& other) const
{
    return kind() == other.kind();
}

// ============================================================================
// SharedSubtreeRefNode（Cubium 特有，SharedTopology 边界）
// relaxedEquals/equals 按 subTreeId 比值（不同 SharedTopology 子树有不同 id）。
// ============================================================================

Ptr SharedSubtreeRefNode::transform(AstTransformer& t) const
{
    // m_inner 是共享不可变子树，不参与 pass 重写（保持跨区块共享语义）。
    return t.transform(shared_from_this());
}

bool SharedSubtreeRefNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    return m_subTreeId == static_cast<const SharedSubtreeRefNode&>(other).m_subTreeId;
}

size_t SharedSubtreeRefNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, m_subTreeId);
    return h;
}

bool SharedSubtreeRefNode::equals(const AstNode& other) const
{
    return relaxedEquals(other);
}

// ============================================================================
// FindTopSurfaceNode（Cubium 特有，循环搜索）
// relaxedEquals：lowerBound/cellHeight 标量比值 + density/upperBound 子树 relaxedEquals。
// ============================================================================

Ptr FindTopSurfaceNode::transform(AstTransformer& t) const
{
    Ptr newDensity = m_density ? m_density->transform(t) : m_density;
    Ptr newUpper = m_upperBound ? m_upperBound->transform(t) : m_upperBound;
    if (newDensity == m_density && newUpper == m_upperBound) {
        return t.transform(shared_from_this());
    }
    return t.transform(
        std::make_shared<FindTopSurfaceNode>(std::move(newDensity), std::move(newUpper), m_lowerBound, m_cellHeight));
}

bool FindTopSurfaceNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const FindTopSurfaceNode&>(other);
    return m_lowerBound == o.m_lowerBound && m_cellHeight == o.m_cellHeight &&
        childrenRelaxedEqual(m_density, o.m_density) && childrenRelaxedEqual(m_upperBound, o.m_upperBound);
}

size_t FindTopSurfaceNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    hashCombine(h, static_cast<i64>(m_lowerBound));
    hashCombine(h, static_cast<i64>(m_cellHeight));
    if (m_density) {
        hashCombine(h, m_density->relaxedHashCode());
    }
    if (m_upperBound) {
        hashCombine(h, m_upperBound->relaxedHashCode());
    }
    return h;
}

bool FindTopSurfaceNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const FindTopSurfaceNode&>(other);
    return m_lowerBound == o.m_lowerBound && m_cellHeight == o.m_cellHeight &&
        childrenStrictEqual(m_density, o.m_density) && childrenStrictEqual(m_upperBound, o.m_upperBound);
}

// ============================================================================
// DelegateNode（包原版 DF，运行时回调 compute）
// relaxedEquals 按 class 比较（typeid）：不同 DF 子类类型不同则不等；同类型实例忽略身份差异
// （编译为求值器字段，运行期从区块参数取，只要类型同生成同段指令）。
// equals 严格按实例身份比较（跨 root 公共子树提取用）。
// ============================================================================

Ptr DelegateNode::transform(AstTransformer& t) const
{
    return t.transform(shared_from_this());
}

bool DelegateNode::relaxedEquals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const DelegateNode&>(other);
    // Cubium DensityFunction 无运行时类型标识虚函数，用 typeid 比较动态类型。
    if (!m_densityFunction || !o.m_densityFunction) {
        return false;
    }
    // 绑定到具名 const 引用再取 typeid，规避 -Wpotentially-evaluated-expression
    // （typeid 对多态类型表达式不求值副作用，仅取动态类型；但 clang 对裸解引用告警）。
    const DensityFunction& a = *m_densityFunction;
    const DensityFunction& b = *o.m_densityFunction;
    return typeid(a) == typeid(b);
}

size_t DelegateNode::relaxedHashCode() const
{
    size_t h = 0;
    hashCombine(h, static_cast<u8>(kind()));
    if (m_densityFunction) {
        const DensityFunction& df = *m_densityFunction;
        hashCombine(h, typeid(df).hash_code());
    }
    return h;
}

bool DelegateNode::equals(const AstNode& other) const
{
    if (kind() != other.kind()) {
        return false;
    }
    const auto& o = static_cast<const DelegateNode&>(other);
    return m_densityFunction == o.m_densityFunction;
}

} // namespace mc::world::gen::density::ast
