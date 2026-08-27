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

#include "common/world/gen/density/ast/CompiledDensityFunction.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctions.hpp" // MarkerType
#include "common/world/gen/density/NoiseChunk.hpp"       // NoiseChunk + 缓存类（newInstance 用）
#include "common/world/gen/density/ast/AstNodes.hpp"     // WeirdType
#include "common/world/gen/density/ast/CompiledDensityFunctionAdapter.hpp"
#include "common/world/gen/density/ast/DensityEvalHelpers.hpp"
#include "common/world/gen/density/ast/DensityJitCompiler.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace mc::world::gen::density::ast {

namespace {

using eval_helpers::clampedLerp;
using eval_helpers::clampedMap;
using eval_helpers::evalFindTopSurface;
using eval_helpers::evalSpline;
using eval_helpers::getRarity;

/// 从 opFlags 低 4 位取 UnaryOp。
[[nodiscard]] UnaryOp unpackUnary(u8 flags) noexcept
{
    return static_cast<UnaryOp>(flags & 0x0F);
}

/// 从 opFlags 低 4 位取 BinaryOp。
[[nodiscard]] BinaryOp unpackBinary(u8 flags) noexcept
{
    return static_cast<BinaryOp>(flags & 0x0F);
}

/// 从 opFlags 高 4 位取 RegAxis。
[[nodiscard]] RegAxis unpackAxis(u8 flags) noexcept
{
    return static_cast<RegAxis>((flags >> 4) & 0x0F);
}

} // namespace

CompiledDensityFunction::CompiledDensityFunction(std::vector<Op> ops,
    u32 regCount,
    std::vector<RuntimeObject> objects,
    std::vector<std::shared_ptr<CompiledDensityFunction>> subEvaluators,
    std::vector<std::shared_ptr<CompiledSpline>> splines,
    f64 minValue,
    f64 maxValue,
    bool hasMarkerOrBeardifier,
    std::vector<std::unique_ptr<::mc::world::gen::density::DensityFunction>> ownedCaches)
    : m_ops(std::move(ops))
    , m_regCount(regCount)
    , m_objects(std::move(objects))
    , m_subEvaluators(std::move(subEvaluators))
    , m_splines(std::move(splines))
    , m_minValue(minValue)
    , m_maxValue(maxValue)
    , m_hasMarkerOrBeardifier(hasMarkerOrBeardifier)
    , m_ownedCaches(std::move(ownedCaches))
{
    // m_evalCtx 指向本实例成员表（JIT 机器码经首参指针访问）。JIT 编译（compileJit）不在构造
    // 函数触发——维度级由 BytecodeGen::compile 末尾显式调用（编译一次），区块级 newInstance
    // 复用维度级 m_jitFn（ops 字节相同）。故构造时 m_jitFn 保持 nullptr，m_evalCtx 就绪待 JIT 接入。
    m_evalCtx = DensityEvalContext{m_objects.data(), m_subEvaluators.data(), m_splines.data()};
}

// 区块级 newInstance：把 MARKER 占位替换为 per-chunk 缓存对象。
// 仅当 hasMarkerOrBeardifier() 为 true 时调用（否则 NoiseChunk 直接 Adapter 包装维度级实例）。
// delegate 子求值器递归 newInstance（含嵌套 Marker 时）或共享维度级（不含时），
// 包 Adapter 作缓存对象 filler。缓存对象所有权：NoiseInterpolator/CellCache 转入 NoiseChunk
// 容器，CacheOnce/FlatCache/Cache2D 由本区块级求值器 m_ownedCaches 持有。
std::shared_ptr<CompiledDensityFunction> CompiledDensityFunction::newInstance(
    ::mc::world::gen::density::NoiseChunk& chunk) const
{
    // 1. 深拷贝 Op 序列与运行时对象表（Op 是 POD，RuntimeObject 是可拷贝 POD-like）。
    std::vector<Op> newOps = m_ops;
    std::vector<RuntimeObject> newObjects = m_objects;

    // 2. 递归子求值器：含 MARKER/BEARDIFIER 的递归 newInstance 得区块级，不含的共享维度级（零深拷贝）。
    std::vector<std::shared_ptr<CompiledDensityFunction>> newSubEvaluators;
    newSubEvaluators.reserve(m_subEvaluators.size());
    for (const auto& sub : m_subEvaluators) {
        if (sub != nullptr && sub->hasMarkerOrBeardifier()) {
            newSubEvaluators.push_back(sub->newInstance(chunk));
        } else {
            newSubEvaluators.push_back(sub); // 共享维度级（不可变）
        }
    }

    // 区块级求值器拥有的缓存对象（CacheOnce/FlatCache/Cache2D 拥有型）。
    std::vector<std::unique_ptr<::mc::world::gen::density::DensityFunction>> ownedCaches;

    // 3. 遍历 newOps，把 MARKER 占位替换为缓存对象。
    const auto& cellCfg = chunk.cellConfig();
    for (auto& op : newOps) {
        if (op.code != OpCode::Marker) {
            continue;
        }
        const MarkerType markerType = static_cast<MarkerType>((op.opFlags >> 4) & 0x0F);

        // delegate 区块级子求值器（subIdx 索引 newSubEvaluators），包 Adapter 作 filler。
        auto filler = std::make_unique<CompiledDensityFunctionAdapter>(newSubEvaluators[op.subIdx]);

        switch (markerType) {
            case MarkerType::Interpolated: {
                auto interpolator = std::make_unique<::mc::world::gen::density::NoiseInterpolator>(
                    std::move(filler), cellCfg.cellCountXZ, cellCfg.cellCountY);
                interpolator->bindNoiseChunk(&chunk, cellCfg.cellWidth, cellCfg.cellHeight);
                newObjects[op.objIdx].densityFunction = chunk.registerInterpolator(std::move(interpolator));
                break;
            }
            case MarkerType::CacheAllInCell: {
                auto cellCache = std::make_unique<::mc::world::gen::density::CellCache>(
                    std::move(filler), cellCfg.cellWidth, cellCfg.cellHeight);
                cellCache->bindNoiseChunk(&chunk);
                newObjects[op.objIdx].densityFunction = chunk.registerCellCache(std::move(cellCache));
                break;
            }
            case MarkerType::CacheOnce: {
                auto cacheOnce = std::make_unique<::mc::world::gen::density::CacheOnce>(std::move(filler));
                chunk.bindCacheOnceCounters(*cacheOnce);
                newObjects[op.objIdx].densityFunction = cacheOnce.get();
                ownedCaches.push_back(std::move(cacheOnce));
                break;
            }
            case MarkerType::FlatCache: {
                auto flatCache = std::make_unique<::mc::world::gen::density::FlatCache>(
                    std::move(filler), chunk.firstNoiseX(), chunk.firstNoiseZ(), chunk.noiseSizeXZ(), true);
                newObjects[op.objIdx].densityFunction = flatCache.get();
                ownedCaches.push_back(std::move(flatCache));
                break;
            }
            case MarkerType::Cache2D: {
                auto cache2D = std::make_unique<::mc::world::gen::density::Cache2D>(std::move(filler));
                newObjects[op.objIdx].densityFunction = cache2D.get();
                ownedCaches.push_back(std::move(cache2D));
                break;
            }
            case MarkerType::BeardifierMarker:
                // 方案X：Beardifier 不进编译产物（留 NoiseChunk OOP 层手工组装 CellCache(Add(...))）。
                // 维度级树不含 BeardifierMarker，newInstance 不应遇到此型。
                MC_ASSERT_RELEASE_MSG(false, "newInstance: BeardifierMarker marker should not appear in compiled tree");
                break;
        }
    }

    // 4. 组装区块级求值器：共享 m_splines（样条不可变），min/max/hasMarkerOrBeardifier 沿用维度级。
    //    注意：区块级 hasMarkerOrBeardifier 仍为 true（MARKER 指令仍在序列中，只是缓存对象已注入），
    //    这不影响正确性——newInstance 不会被对区块级实例再次调用（NoiseChunk 构造只 newInstance 一次）。
    //    ownedCaches 持有 CacheOnce/FlatCache/Cache2D 的所有权，保证 newObjects 中的裸指针生命周期
    //    与区块级求值器一致（否则返回后悬垂）。
    auto inst = std::make_shared<CompiledDensityFunction>(std::move(newOps),
        m_regCount,
        std::move(newObjects),
        std::move(newSubEvaluators),
        m_splines, // 共享（const shared_ptr 拷贝）
        m_minValue,
        m_maxValue,
        m_hasMarkerOrBeardifier,
        std::move(ownedCaches));

    // 5. JIT 复用：区块级 ops 与维度级字节相同（newInstance 只改 newObjects 缓存对象，不改 Op），
    //    故直接复用维度级 m_jitFn（构造函数不触发 JIT 编译，m_jitFn 初始为 nullptr）。m_evalCtx
    //    重建指向区块级自身的对象/子求值器/样条表（构造函数已建过，但 make_shared 后成员地址
    //    确定，此处显式重建确保指针指向区块级实例自身而非临时）。
    inst->m_jitFn = m_jitFn;
    inst->m_evalCtx = DensityEvalContext{inst->m_objects.data(), inst->m_subEvaluators.data(), inst->m_splines.data()};
    return inst;
}

f64 CompiledDensityFunction::eval(i32 x, i32 y, i32 z) const
{
    // 寄存器缓冲：编译期已知 regCount 固定，绝大多数密度函数树规模很小
    // （实测三维度 15 root 最大 regCount=17，子求值器更小）。优先用栈上定长数组
    // 避免每次求值的堆分配——eval 在 FillNoiseCells 逐方块主循环被高频调用
    // （每区块数万次），堆分配是已观测的性能瓶颈。
    //
    // 栈数组上限 kInlineRegCount 取保守值 128（实测最大 17 的 ~7.5 倍余量，覆盖未来
    // 数据包更大的密度函数树）。超出时回退堆 vector 安全兜底（罕见路径，保证不越界）。
    //
    // 递归安全：eval 经 SharedSubtreeCall/Marker/Spline/FindTopSurface 递归调用自身，
    // 每层 eval 独立栈缓冲，互不覆盖（故不能用 thread_local 单 buffer）。

    // JIT 路径：编译成功（m_jitFn != nullptr）时直接调机器码，消除 switch 分发 / Op 取指 /
    // regs 间接寻址开销。
    if (m_jitFn != nullptr) [[likely]] {
        return m_jitFn(&m_evalCtx, x, y, z);
    }

    // 回退路径：原 switch 解释器 evalImpl。
    if (m_regCount <= kInlineRegCount) [[likely]] {
        std::array<f64, kInlineRegCount> regs{};
        return evalImpl(x, y, z, regs.data());
    }
    std::vector<f64> regs(m_regCount);
    return evalImpl(x, y, z, regs.data());
}

f64 CompiledDensityFunction::evalInterpreter(i32 x, i32 y, i32 z) const
{
    // 测试/调试专用：绕过 JIT 强制走 switch 解释器 evalImpl（纯解释器求值供
    // DensityJitBaselineTest 与 JIT 机器码求值逐点对比）。缓冲策略与 eval 一致。
    if (m_regCount <= kInlineRegCount) [[likely]] {
        std::array<f64, kInlineRegCount> regs{};
        return evalImpl(x, y, z, regs.data());
    }
    std::vector<f64> regs(m_regCount);
    return evalImpl(x, y, z, regs.data());
}

f64 CompiledDensityFunction::evalImpl(i32 x, i32 y, i32 z, f64* regs) const
{
    const size_t n = m_ops.size();
    for (size_t pc = 0; pc < n; ++pc) {
        const Op& op = m_ops[pc];
        switch (op.code) {
            case OpCode::Return:
                return regs[op.dst];
            case OpCode::LoadConst:
                regs[op.dst] = op.imm;
                break;
            case OpCode::Coord: {
                const RegAxis axis = unpackAxis(op.opFlags);
                regs[op.dst] = (axis == RegAxis::X) ? static_cast<f64>(x)
                    : (axis == RegAxis::Y)          ? static_cast<f64>(y)
                                                    : static_cast<f64>(z);
                break;
            }
            case OpCode::YGradient:
                // imm3=fromY, imm4=toY, imm=fromValue, imm2=toValue
                regs[op.dst] = clampedMap(static_cast<f64>(y), op.imm3, op.imm4, op.imm, op.imm2);
                break;
            case OpCode::NoiseSample: {
                const auto* noise = m_objects[op.objIdx].noise;
                MC_ASSERT_RELEASE_MSG(noise != nullptr, "NoiseSample: noise object is null");
                regs[op.dst] = noise->getValue(regs[op.srcA], regs[op.srcB], regs[op.srcC]);
                break;
            }
            case OpCode::WeirdSampler: {
                const auto* noise = m_objects[op.objIdx].noise;
                MC_ASSERT_RELEASE_MSG(noise != nullptr, "WeirdSampler: noise object is null");
                const WeirdType type = (op.opFlags & 0x0F) == 0 ? WeirdType::Type1 : WeirdType::Type2;
                const f64 inputValue = regs[op.srcA];
                const f64 r = getRarity(type, inputValue);
                regs[op.dst] = std::abs(noise->getValue(
                                   static_cast<f64>(x) / r, static_cast<f64>(y) / r, static_cast<f64>(z) / r)) *
                    r;
                break;
            }
            case OpCode::Delegate: {
                const auto* df = m_objects[op.objIdx].densityFunction;
                MC_ASSERT_RELEASE_MSG(df != nullptr, "Delegate: density function is null");
                regs[op.dst] = df->compute(x, y, z);
                break;
            }
            case OpCode::EndIslands: {
                const auto* df = m_objects[op.objIdx].densityFunction;
                MC_ASSERT_RELEASE_MSG(df != nullptr, "EndIslands: density function is null");
                regs[op.dst] = df->compute(x, y, z);
                break;
            }
            case OpCode::Beardifier: {
                // 维度级编译期 Beardifier 未注入（区块特定），占位返回 0.0。
                // 阶段5 newInstance 注入真实 Beardifier 后此指令段被替换。
                const auto* beardifier = m_objects[op.objIdx].beardifier;
                regs[op.dst] = (beardifier != nullptr) ? beardifier->compute(x, y, z) : 0.0;
                break;
            }
            case OpCode::SharedSubtreeCall: {
                const auto& sub = m_subEvaluators[op.subIdx];
                MC_ASSERT_RELEASE_MSG(sub != nullptr, "SharedSubtreeCall: sub-evaluator is null");
                regs[op.dst] = sub->eval(x, y, z);
                break;
            }
            case OpCode::Unary: {
                const UnaryOp uop = unpackUnary(op.opFlags);
                const f64 v = regs[op.srcA];
                switch (uop) {
                    case UnaryOp::Abs:
                        regs[op.dst] = std::abs(v);
                        break;
                    case UnaryOp::Square:
                        regs[op.dst] = v * v;
                        break;
                    case UnaryOp::Cube:
                        regs[op.dst] = v * v * v;
                        break;
                    case UnaryOp::Squeeze: {
                        // squeeze(v) = clamp(v,-1,1)/2 - clamp(v,-1,1)^3/24
                        const f64 c = std::clamp(v, -1.0, 1.0);
                        regs[op.dst] = c / 2.0 - c * c * c / 24.0;
                        break;
                    }
                    case UnaryOp::Sqrt:
                        regs[op.dst] = std::sqrt(v);
                        break;
                    case UnaryOp::Sin:
                        regs[op.dst] = std::sin(v);
                        break;
                    case UnaryOp::Cos:
                        regs[op.dst] = std::cos(v);
                        break;
                    case UnaryOp::Floor:
                        regs[op.dst] = std::floor(v);
                        break;
                    case UnaryOp::Ceil:
                        regs[op.dst] = std::ceil(v);
                        break;
                }
                break;
            }
            case OpCode::NegMul: {
                // input <= 0 ? input * negMul : input（NegMulNode / HalfNegative/QuarterNegative）
                const f64 v = regs[op.srcA];
                regs[op.dst] = (v <= 0.0) ? v * op.imm : v;
                break;
            }
            case OpCode::Clamp:
                regs[op.dst] = std::clamp(regs[op.srcA], op.imm, op.imm2);
                break;
            case OpCode::Binary: {
                const BinaryOp bop = unpackBinary(op.opFlags);
                const f64 a = regs[op.srcA];
                const f64 b = regs[op.srcB];
                switch (bop) {
                    case BinaryOp::Add:
                        regs[op.dst] = a + b;
                        break;
                    case BinaryOp::Mul:
                        // vanilla TwoArgument.Mul 当 v1==0.0 短路返回 0.0；此处不短路，
                        // a==0 时 a*b=0.0 数值一致（b 仍被求值，纯性能差异，见 hpp TODO）。
                        regs[op.dst] = a * b;
                        break;
                    case BinaryOp::Div:
                        regs[op.dst] = a / b;
                        break;
                    case BinaryOp::Max:
                        regs[op.dst] = std::max(a, b);
                        break;
                    case BinaryOp::Min:
                        regs[op.dst] = std::min(a, b);
                        break;
                }
                break;
            }
            case OpCode::Lerp: {
                // clampedLerp(delta, start, end)：delta<=0→start, delta>=1→end, else start+delta*(end-start)
                regs[op.dst] = clampedLerp(regs[op.srcA], regs[op.srcB], regs[op.srcC]);
                break;
            }
            case OpCode::RangeChoice: {
                const f64 input = regs[op.srcA];
                // jumpTarget/jumpTarget2 是目标 Op 下标；--pc 抵消 for 的 ++pc，使下次从目标下标执行。
                if (input >= op.imm && input < op.imm2) {
                    pc = op.jumpTarget; // 跳到 whenInRange 段
                } else {
                    pc = op.jumpTarget2; // 跳到 whenOutOfRange 段
                }
                --pc; // 抵消 for 循环的 ++pc
                break;
            }
            case OpCode::Jump:
                // 无条件跳转。--pc 抵消 ++pc。
                pc = op.jumpTarget;
                --pc;
                break;
            case OpCode::JumpIfCmp: {
                // 条件跳转（MaxShort/MinShort 短路）：cmpOp 低 4 位，Gt=0/Lt=1。
                // 满足条件则跳到 jumpTarget（跳过 right 子段），否则顺序执行 right 段。
                const f64 v = regs[op.srcA];
                const bool take = (op.opFlags & 0x0F) == static_cast<u8>(CmpOp::Gt) ? (v > op.imm) : (v < op.imm);
                if (take) {
                    pc = op.jumpTarget;
                    --pc; // 抵消 for 的 ++pc
                }
                break;
            }
            case OpCode::Copy:
                regs[op.dst] = regs[op.srcA];
                break;
            case OpCode::Marker: {
                // Marker 分层：objIdx 索引 m_objects.densityFunction。
                // 维度级（densityFunction==nullptr）：透传 delegate 子求值器 eval（数值等价阶段4 内联透传）。
                // 区块级（densityFunction!=nullptr，newInstance 注入缓存对象）：走缓存对象 compute。
                const auto* cacheObj = m_objects[op.objIdx].densityFunction;
                if (cacheObj != nullptr) {
                    // 区块级走缓存对象 compute。缓存对象（NoiseInterpolator/CellCache/CacheOnce/
                    // FlatCache/Cache2D）的 compute 在缓存未命中时回调 m_filler->compute，而 filler
                    // 是 Adapter，其 compute → CompiledDensityFunction::eval 递归。
                    regs[op.dst] = cacheObj->compute(x, y, z);
                } else {
                    // 维度级透传 delegate 子求值器 eval（递归）。
                    const auto& delegate = m_subEvaluators[op.subIdx];
                    MC_ASSERT_RELEASE_MSG(delegate != nullptr, "Marker: delegate sub-evaluator is null");
                    regs[op.dst] = delegate->eval(x, y, z);
                }
                break;
            }
            case OpCode::Spline: {
                const auto& spline = m_splines[op.objIdx];
                MC_ASSERT_RELEASE_MSG(spline != nullptr, "Spline: spline data is null");
                regs[op.dst] = evalSpline(*spline, regs[op.srcA], x, y, z);
                break;
            }
            case OpCode::FindTopSurface: {
                const auto& densitySub = m_subEvaluators[op.subIdx];
                MC_ASSERT_RELEASE_MSG(densitySub != nullptr, "FindTopSurface: density sub-evaluator is null");
                const f64 upperVal = regs[op.srcA];
                const i32 lowerBound = static_cast<i32>(op.imm);
                const i32 cellHeight = static_cast<i32>(op.imm2);
                regs[op.dst] = evalFindTopSurface(*densitySub, upperVal, lowerBound, cellHeight, x, z);
                break;
            }
        }
    }
    // 序列无显式 Return（不应发生，编译器保证末尾有 Return）。
    MC_ASSERT_RELEASE_MSG(false, "CompiledDensityFunction::eval: missing Return instruction");
    return 0.0;
}

void CompiledDensityFunction::compileJit() noexcept
{
    // 维度级编译一次：把 m_ops 翻译为 asmjit 机器码。失败（asmjit Error / 非 Win x64 平台 /
    // 空 Op 序列）m_jitFn 留 nullptr，eval 自动回退 evalImpl（功能不受影响）。JIT 机器码内存由
    // 进程级 JitRuntime 单例持有至进程结束（求值器不可变，无需 release）。区块级 newInstance
    // 不调本函数——直接复用维度级 m_jitFn（ops 字节相同）。
    m_jitFn = compileDensityJit(m_ops, m_regCount);
}

} // namespace mc::world::gen::density::ast
