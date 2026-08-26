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

#include "common/world/gen/density/ast/McToAst.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"

#include <spdlog/spdlog.h>

#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace mc::world::gen::density::ast {

using DF = ::mc::world::gen::density::DensityFunction;
using namespace ::mc::world::gen::density; // TwoArgument/Mapped/Marker/... 在此命名空间
using Ptr = AstNode::Ptr;                  // 命名空间层别名，供自由函数与成员函数返回类型使用

namespace {

/// 辅助：构造坐标轴节点。
Ptr coord(Axis axis)
{
    return std::make_shared<CoordinateNode>(axis);
}

/// 辅助：构造 f64 常量节点。
Ptr constant(f64 v)
{
    return std::make_shared<ConstantNode>(v);
}

/// 辅助：blockAxis * scale（坐标缩放）。
Ptr scaledAxis(Axis axis, f64 scale)
{
    return std::make_shared<MulNode>(coord(axis), constant(scale));
}

/// 辅助：blockAxis * scale + shift（移位噪声坐标）。
Ptr shiftedAxis(Axis axis, f64 scale, const DF& shift)
{
    return std::make_shared<AddNode>(scaledAxis(axis, scale), McToAst::convert(shift));
}

} // namespace

std::unordered_map<std::type_index, McToAst::Emitter>& McToAst::registry()
{
    // Meyers 单例：静态局部 map 首次访问时填充。C++11 起静态局部变量初始化线程安全。
    static std::unordered_map<std::type_index, Emitter> reg = [] {
        std::unordered_map<std::type_index, Emitter> r;
        initializeBuiltin(r);
        return r;
    }();
    return reg;
}

void McToAst::initializeBuiltin(std::unordered_map<std::type_index, Emitter>& reg)
{

    // ---- Constant ----
    reg[std::type_index(typeid(Constant))] = [](const DF& df) -> Ptr {
        return constant(static_cast<const Constant&>(df).value());
    };

    // ---- YClampedGradient ----
    reg[std::type_index(typeid(YClampedGradient))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const YClampedGradient&>(df);
        return std::make_shared<YClampedGradientNode>(f.fromY(), f.toY(), f.fromValue(), f.toValue());
    };

    // ---- Clamp ----
    // Cubium Clamp 的 minValue()/maxValue() 虚函数即标量边界（无单独 getter）。
    reg[std::type_index(typeid(Clamp))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const Clamp&>(df);
        return std::make_shared<ClampNode>(convert(f.input()), f.minValue(), f.maxValue());
    };

    // ---- Mapped（单参数变换统一类）----
    reg[std::type_index(typeid(Mapped))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const Mapped&>(df);
        Ptr input = convert(f.input());
        switch (f.type()) {
            case MappedType::Abs:
                return std::make_shared<AbsNode>(input);
            case MappedType::Square:
                return std::make_shared<SquareNode>(input);
            case MappedType::Cube:
                return std::make_shared<CubeNode>(input);
            case MappedType::Squeeze:
                return std::make_shared<SqueezeNode>(input);
            case MappedType::HalfNegative:
                // value>0 ? value : value*0.5  ≡  value<=0 ? value*0.5 : value  ≡ NegMul(0.5)
                return std::make_shared<NegMulNode>(input, 0.5);
            case MappedType::QuarterNegative:
                return std::make_shared<NegMulNode>(input, 0.25);
            case MappedType::Invert:
                return std::make_shared<DivNode>(constant(1.0), input);
        }
        MC_ASSERT_MSG(false, "unhandled MappedType");
        return nullptr;
    };

    // ---- TwoArgument（双参数运算统一类）----
    // Min/Max 的短路：仅当 arg1 范围能从 arg2 范围获益时才用 Short 变体（对齐 DFC McToAst:88-103）。
    reg[std::type_index(typeid(TwoArgument))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const TwoArgument&>(df);
        Ptr a = convert(f.arg1());
        Ptr b = convert(f.arg2());
        switch (f.type()) {
            case TwoArgumentType::Add:
                return std::make_shared<AddNode>(a, b);
            case TwoArgumentType::Mul:
                return std::make_shared<MulNode>(a, b);
            case TwoArgumentType::Min: {
                const f64 rightMin = f.arg2().minValue();
                if (f.arg1().minValue() < rightMin) {
                    return std::make_shared<MinShortNode>(a, b, rightMin);
                }
                return std::make_shared<MinNode>(a, b);
            }
            case TwoArgumentType::Max: {
                const f64 rightMax = f.arg2().maxValue();
                if (f.arg1().maxValue() > rightMax) {
                    return std::make_shared<MaxShortNode>(a, b, rightMax);
                }
                return std::make_shared<MaxNode>(a, b);
            }
        }
        MC_ASSERT_MSG(false, "unhandled TwoArgumentType");
        return nullptr;
    };

    // ---- Lerp ----
    reg[std::type_index(typeid(Lerp))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const Lerp&>(df);
        return std::make_shared<LerpNode>(convert(f.delta()), convert(f.start()), convert(f.end()));
    };

    // ---- RangeChoice ----
    reg[std::type_index(typeid(RangeChoice))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const RangeChoice&>(df);
        return std::make_shared<RangeChoiceNode>(convert(f.input()),
            f.minInclusive(),
            f.maxExclusive(),
            convert(f.whenInRange()),
            convert(f.whenOutOfRange()));
    };

    // ---- NoiseDensity（noise）----
    // blockX*xzScale, blockY*yScale, blockZ*xzScale → 坐标展开。
    reg[std::type_index(typeid(NoiseDensity))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const NoiseDensity&>(df);
        return std::make_shared<GenericShiftedNoiseNode>(scaledAxis(Axis::X, f.xzScale()),
            scaledAxis(Axis::Y, f.yScale()),
            scaledAxis(Axis::Z, f.xzScale()),
            &f.noise());
    };

    // ---- ShiftedNoise ----
    // blockX*xzScale + shiftX 等 → Add(Mul, shift) 坐标展开。
    reg[std::type_index(typeid(ShiftedNoise))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const ShiftedNoise&>(df);
        return std::make_shared<GenericShiftedNoiseNode>(shiftedAxis(Axis::X, f.xzScale(), f.shiftX()),
            shiftedAxis(Axis::Y, f.yScale(), f.shiftY()),
            shiftedAxis(Axis::Z, f.xzScale(), f.shiftZ()),
            &f.noise());
    };

    // ---- ShiftNoise（ShiftA/ShiftB/Shift 统一类）----
    // 坐标先 *0.25，结果 *4.0；ShiftA 的 Y 轴为常量 0；ShiftB 的 X↔Z 交换。
    reg[std::type_index(typeid(ShiftNoise))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const ShiftNoise&>(df);
        Ptr noiseNode;
        switch (f.type()) {
            case ShiftType::Shift:
                noiseNode = std::make_shared<GenericShiftedNoiseNode>(
                    scaledAxis(Axis::X, 0.25), scaledAxis(Axis::Y, 0.25), scaledAxis(Axis::Z, 0.25), &f.noise());
                break;
            case ShiftType::ShiftA:
                // Y 轴恒 0：用 Constant(0) 而非 Mul(Y,0.25)，避免无意义采样。
                noiseNode = std::make_shared<GenericShiftedNoiseNode>(
                    scaledAxis(Axis::X, 0.25), constant(0.0), scaledAxis(Axis::Z, 0.25), &f.noise());
                break;
            case ShiftType::ShiftB:
                // 坐标轴交换：X 用 Z*0.25，Y 用 X*0.25，Z 恒 0。
                noiseNode = std::make_shared<GenericShiftedNoiseNode>(
                    scaledAxis(Axis::Z, 0.25), scaledAxis(Axis::X, 0.25), constant(0.0), &f.noise());
                break;
        }
        return std::make_shared<MulNode>(noiseNode, constant(4.0));
    };

    // ---- WeirdScaledSampler ----
    reg[std::type_index(typeid(WeirdScaledSampler))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const WeirdScaledSampler&>(df);
        const WeirdType type = (f.type() == WeirdScaledSamplerType::Type1) ? WeirdType::Type1 : WeirdType::Type2;
        return std::make_shared<WeirdScaledSamplerNode>(convert(f.input()), &f.noise(), type);
    };

    // ---- CubicSpline → SplineNode（递归嵌套子样条）----
    reg[std::type_index(typeid(CubicSpline))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const CubicSpline&>(df);
        Ptr location = convert(f.input());
        std::vector<SplineNode::Point> points;
        points.reserve(f.points().size());
        for (const auto& sp : f.points()) {
            SplineNode::Point p;
            p.location = sp.location;
            p.derivative = sp.derivative;
            if (std::holds_alternative<f64>(sp.value)) {
                p.value = std::get<f64>(sp.value);
            } else {
                // 嵌套子样条：递归转 AST。
                const auto& child = std::get<std::shared_ptr<CubicSpline>>(sp.value);
                p.value = convert(*child);
            }
            points.push_back(std::move(p));
        }
        return std::make_shared<SplineNode>(std::move(location), std::move(points));
    };

    // ---- Marker → MarkerNode（保留 MarkerType + delegate + delegate 的 min/max 元数据）----
    reg[std::type_index(typeid(Marker))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const Marker&>(df);
        const auto& wrapped = f.wrapped();
        // Marker 透传 delegate，故 Marker 的 min/max == delegate 的 min/max。
        // 记录到 MarkerNode 供 BytecodeGen 把 delegate 子树独立编译为子求值器时
        // 记录到 CompiledDensityFunction（经 Adapter→缓存类 minValue/maxValue 消费）。
        return std::make_shared<MarkerNode>(f.markerType(), convert(wrapped), wrapped.minValue(), wrapped.maxValue());
    };

    // ---- BeardifierMarker / Beardifier → BeardifierNode ----
    // BeardifierMarker 是占位（compute 返回 0），Beardifier 是实际结构贡献；二者在 AST 层统一为
    // BeardifierNode（无字段占位），区块期注入实际 Beardifier 实例。
    reg[std::type_index(typeid(BeardifierMarker))] = [](const DF& /*df*/) -> Ptr {
        return std::make_shared<BeardifierNode>();
    };
    reg[std::type_index(typeid(Beardifier))] = [](const DF& /*df*/) -> Ptr {
        return std::make_shared<BeardifierNode>();
    };

    // ---- SharedTopology → SharedSubtreeRefNode ----
    // 内部子树跨区块共享（纯拓扑），McToAst 在此边界发引用节点 + 把 inner() 递归转成 AST 存入。
    // BytecodeGen 把 inner AST 独立编译为子求值器，父节点通过 SharedSubtreeCall 指令索引调用。
    // subTreeId 用内部子树根指针地址（稳定且唯一标识一棵共享子树）。
    reg[std::type_index(typeid(SharedTopology))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const SharedTopology&>(df);
        return std::make_shared<SharedSubtreeRefNode>(reinterpret_cast<u64>(&f.inner()), convert(f.inner()));
    };

    // ---- SharedHolder → 递归转 inner（深拷贝语义，与 SharedTopology 不同）----
    reg[std::type_index(typeid(SharedHolder))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const SharedHolder&>(df);
        return convert(f.inner());
    };

    // ---- FindTopSurface ----
    reg[std::type_index(typeid(FindTopSurface))] = [](const DF& df) -> Ptr {
        const auto& f = static_cast<const FindTopSurface&>(df);
        return std::make_shared<FindTopSurfaceNode>(
            convert(f.density()), convert(f.upperBound()), f.lowerBound(), f.cellHeight());
    };

    // ---- EndIslands ----
    // 正常路径 EndIslands 被 SharedTopology 包装，McToAst 在边界发 SharedSubtreeRefNode，
    // 不会到达此处。此规则仅覆盖未共享的退化路径——持裸指针。
    reg[std::type_index(typeid(EndIslands))] = [](const DF& df) -> Ptr {
        return std::make_shared<EndIslandsNode>(&df);
    };

    // TODO: BlendedNoise / MappedNoise 暂走 DelegateNode 退化（出现频率低，专用节点待主世界/
    // 下界/末地噪声树覆盖确认后再补）。MappedNoise 无公共字段访问器，BlendedNoise 可后续补专用节点。
}

Ptr McToAst::convert(const DF& df)
{
    return toAst(df);
}

Ptr McToAst::toAst(const DF& df)
{
    auto& reg = registry();
    const auto it = reg.find(std::type_index(typeid(df)));
    if (it != reg.end()) {
        return it->second(df);
    }
    // 未识别类型：退化为 DelegateNode，首次告警以便观测覆盖率。
    static std::unordered_set<std::type_index> warned;
    static std::mutex warnMutex;
    const auto key = std::type_index(typeid(df));
    {
        std::lock_guard<std::mutex> lock(warnMutex);
        if (warned.insert(key).second) {
            spdlog::warn("McToAst: 未识别的 DensityFunction 类型，退化为 DelegateNode: {}", key.name());
        }
    }
    return std::make_shared<DelegateNode>(&df);
}

} // namespace mc::world::gen::density::ast
