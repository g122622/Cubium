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

#include <memory>
#include <vector>

namespace mc::world::gen::density {
class DensityFunction;
class Beardifier;
class NoiseChunk;
} // namespace mc::world::gen::density

namespace mc::world::gen::noise {
class NormalNoise;
} // namespace mc::world::gen::noise

namespace mc::world::gen::density::ast {

class CompiledDensityFunction;

// ============================================================================
// 扁平指令序列求值器（阶段4 产物，阶段5 扩展 newInstance）
//
// 效仿 C2ME DFC 的字节码求值器，但 C++ 无 JIT 红利，改用扁平指令序列
// （std::vector<Op> + switch 解释执行）消除虚分发、提升 cache 局部性。
//
// 求值契约：单点求值 eval(x, y, z) -> f64。Cubium 的 DensityFunction 唯一求值入口是
// compute(i32, i32, i32) const，无 fillArray/EachApplier/NoisePos 批量接口（与 MC Java
// 不同），故求值器只做单点，无 evalMulti / DfcObjectCache（后者是 Java fillArray 的
// 对应物，Cubium 无此机制）。
//
// 寄存器模型：编译期把每个 AST 节点的求值结果分配到一个 f64 寄存器 slot（m_regs 数组
// 下标）。Op 通过 dst/srcA/srcB 索引读写寄存器。常量不占运行时指令——直接内联到消费 Op
// 的 imm 字段。
//
// 子树引用：样条嵌套子样条、SharedSubtreeRef、FindTopSurface.density、Marker.delegate
// 独立编译为子 CompiledDensityFunction，父序列用 SHARED_SUBTREE_CALL/SPLINE/
// FIND_TOP_SURFACE/MARKER 指令调度。子求值器用 shared_ptr 持有——区块级 newInstance
// 时，不含 MARKER/BEARDIFIER 的子求值器直接共享维度级实例（零深拷贝）。
//
// Marker 分层：维度级编译产物含 MARKER 占位指令（delegate 子求值器透传）。
// 区块级 newInstance 把 MARKER 指令的缓存对象注入 m_objects[objIdx].densityFunction，
// eval 检测到非空则走缓存对象 compute（NoiseInterpolator/CellCache/CacheOnce/FlatCache/
// Cache2D），为空则走 delegate 子求值器 eval（维度级透传）。Beardifier 不进编译产物
// （Beardifier 含区块特定结构数据，方案X 留 NoiseChunk OOP 层手工组装 CellCache(Add(...))）。
// ============================================================================

/// 坐标轴（COORD 指令操作数）。
enum class RegAxis : u8 { X, Y, Z };

/// 一元运算类型（UNARY 指令操作数）。覆盖 Abs/Square/Cube/Squeeze/Sqrt/Sin/Cos/Floor/Ceil。
enum class UnaryOp : u8 {
    Abs,
    Square,
    Cube,
    Squeeze,
    Sqrt,
    Sin,
    Cos,
    Floor,
    Ceil,
};

/// 二元运算类型（BINARY 指令操作数）。覆盖 Add/Mul/Div/Max/Min。
/// MaxShort/MinShort 在求值上等价 Max/Min（rightMax/rightMin 仅用于编译期 relaxedEquals 区分
/// 与未来短路优化，不参与求值），BytecodeGen 统一编译为 Max/Min。
enum class BinaryOp : u8 {
    Add,
    Mul,
    Div,
    Max,
    Min,
};

/// 指令操作码。顺序按 AST 节点分类组织，switch 解释执行按此分发。
enum class OpCode : u8 {
    // ---- 终止 ----
    /// RETURN dst：把寄存器 dst 的值作为求值结果返回。每个求值器序列末尾必有且仅有一条。
    Return,

    // ---- 叶子（无 src，写 dst）----
    /// LOAD_CONST dst = imm：常量载入寄存器（ConstantNode）。imm 内联常量值。
    LoadConst,
    /// COORD dst, axis：取整型坐标轴转 f64（CoordinateNode）。axis 取 RegAxis。
    Coord,
    /// Y_GRADIENT dst, fromY, toY, fromValue, toValue：YClampedGradient 的 clampedMap(y,...)。
    /// 四个标量内联到 imm 字段组。
    YGradient,
    /// NOISE_SAMPLE dst, objIdx, srcX, srcY, srcZ：噪声采样（GenericShiftedNoiseNode）。
    /// m_objects[objIdx] 是 const NormalNoise*，坐标由三个子寄存器提供。
    NoiseSample,
    /// WEIRD_SAMPLER dst, objIdx, srcInput, weirdType：WeirdScaledSampler。
    /// abs(noise(x/r,y/r,z/r))*r，r=getRarity(input)。坐标用当前 (x,y,z)（采样前除以 r）。
    WeirdSampler,
    /// DELEGATE dst, objIdx：回退原版 DensityFunction.compute(x,y,z)（DelegateNode）。
    /// m_objects[objIdx] 是 const DensityFunction*。
    Delegate,
    /// END_ISLANDS dst, objIdx：末地岛屿（EndIslandsNode）。m_objects[objIdx] 是 const DensityFunction*。
    EndIslands,
    /// BEARDIFIER dst, objIdx：Beardifier 贡献（BeardifierNode）。m_objects[objIdx] 是 const Beardifier*。
    /// 方案X：Beardifier 不进编译产物（留 OOP 层），维度级 finalDensity 树不含 BeardifierMarker，
    /// 故生产路径 BEARDIFIER 指令不触发。保留供 McToAst 映射 BeardifierNode 的完整性。
    Beardifier,
    /// SHARED_SUBTREE_CALL dst, subIdx：调用独立编译的共享子树求值器（SharedSubtreeRefNode）。
    /// m_subEvaluators[subIdx]->eval(x,y,z)。SharedTopology 纯拓扑子树跨区块复用同一求值器。
    SharedSubtreeCall,

    // ---- 一元 ----
    /// UNARY dst, op, src：一元运算（Abs/Square/Cube/Squeeze/Sqrt/Sin/Cos/Floor/Ceil）。
    Unary,
    /// NEG_MUL dst, src, imm(negMul)：input <= 0 ? input*negMul : input（NegMulNode）。
    NegMul,
    /// CLAMP dst, src, imm(min), imm2(max)：std::clamp(src, min, max)（ClampNode）。
    Clamp,

    // ---- 二元 ----
    /// BINARY dst, op, srcA, srcB：二元运算（Add/Mul/Div/Max/Min）。
    /// 数值上两子都求（密度函数无副作用）。Mul 的 v1==0.0 短路、MaxShort/MinShort 的
    /// rightMax/rightMin 短路均为纯性能优化（短路时结果与不短路数值一致），阶段4 先不实现跳转短路，
    /// 统一走 BINARY 求两子再运算，保证数值严格对齐基线。
    // TODO: 阶段4.5 性能优化——为 Mul(left==0)/MaxShort/MinShort 增加跳转短路指令，
    // 跳过 right 子段求值。
    Binary,
    /// LERP dst, srcDelta, srcStart, srcEnd：clampedLerp 语义（LerpNode）。
    /// delta<=0→start, delta>=1→end, else start+delta*(end-start)。
    Lerp,

    // ---- 控制 ----
    /// RANGE_CHOICE dst, srcInput, imm(minInclusive), imm2(maxExclusive),
    /// jumpTarget(inRange段起点), jumpTarget2(outOfRange段起点)：区间分支（RangeChoiceNode）。
    /// input∈[minInclusive,maxExclusive) 时跳到 whenInRange 段，否则跳到 whenOutOfRange 段。
    /// 两段各自把结果 COPY 到 dst 后用 JUMP 跳到序列末尾。
    /// 跳转语义：jumpTarget/jumpTarget2 = 目标 Op 下标；eval 执行 pc=jumpTarget 后 --pc 抵消 for 的 ++pc。
    RangeChoice,
    /// JUMP jumpTarget：无条件跳转（RangeChoice 的 inRange 段尾跳过 outOfRange 段用）。
    /// jumpTarget = 目标 Op 下标，eval 同 RANGE_CHOICE 的 --pc 语义。
    Jump,
    /// COPY dst, srcA：寄存器复制（regs[dst] = regs[srcA]）。RangeChoice 两段结果归一到 dst 用。
    Copy,
    /// MARKER dst, objIdx, subIdx, markerType：Marker 占位（MarkerNode）。
    /// objIdx 索引 m_objects：维度级 densityFunction==nullptr（占位），区块级 newInstance 注入
    /// 缓存对象（NoiseInterpolator/CellCache/CacheOnce/FlatCache/Cache2D）。
    /// subIdx 索引 m_subEvaluators：delegate 子求值器（维度级透传用）。
    /// markerType 编码在 opFlags 高 4 位（newInstance 据此创建对应缓存对象）。
    /// eval：densityFunction 非空走缓存对象 compute，为空走 delegate 子求值器 eval。
    Marker,

    // ---- 样条 ----
    /// SPLINE dst, objIdx, srcLocation：样条求值（SplineNode）。
    /// m_splines[objIdx] 持预编译样条数据（locations/derivatives/values 子求值器），
    /// 二分查找 + Hermite 三次插值。location 由 srcLocation 寄存器提供。
    Spline,

    // ---- FindTopSurface（Cubium 特有，循环搜索）----
    /// FIND_TOP_SURFACE dst, subIdx(density sub-evaluator), srcUpperBound,
    /// imm(lowerBound), imm2(cellHeight)：从 floor(upper/cellH)*cellH 向下逐 cellH
    /// 找首个 density>0 的 Y。density 子树独立编译为子求值器（m_subEvaluators[subIdx]）。
    FindTopSurface,
};

/// 样条预编译数据（SPLINE 指令引用）。locations 升序，values 为各控制点的子求值器
/// （常量点编译为 LOAD_CONST 单指令求值器，嵌套子样条编译为递归 SPLINE 求值器）。
/// valueEvaluators 用 shared_ptr：区块级 newInstance 共享维度级样条（样条不可变）。
struct CompiledSpline {
    std::vector<f64> locations;
    std::vector<f64> derivatives;
    /// 每个控制点的值求值器（length == locations.length）。常量点也封装为求值器统一调度。
    std::vector<std::shared_ptr<CompiledDensityFunction>> valueEvaluators;
};

/// 单条指令（POD）。不同 OpCode 用不同字段；未用字段留默认值。
struct Op {
    OpCode code = OpCode::Return;
    u32 dst = 0;         // 目标寄存器 slot
    u32 srcA = 0;        // 源寄存器 A
    u32 srcB = 0;        // 源寄存器 B
    u32 srcC = 0;        // 源寄存器 C（Lerp 的 end / RangeChoice 的 input 等）
    u32 objIdx = 0;      // 运行时对象索引（m_objects：噪声/Delegate/Beardifier/Marker缓存对象）
    u32 subIdx = 0;      // 子求值器索引（m_subEvaluators：样条/SharedSubtree/FindTopSurface density/Marker delegate）
    u32 jumpTarget = 0;  // 短路/分支跳转目标（Op 序列下标）
    u32 jumpTarget2 = 0; // 第二跳转目标（RangeChoice 的 outOfRange 段）
    f64 imm = 0.0;       // 内联标量（常量值/negMul/rightMax/rightMin/minInclusive/Clamp min/lowerBound）
    f64 imm2 = 0.0;      // 第二内联标量（maxExclusive/Clamp max/cellHeight）
    f64 imm3 = 0.0;      // 第三内联标量（YGradient fromY / Lerp 等）
    f64 imm4 = 0.0;      // 第四内联标量（YGradient toY 等）
    u8 opFlags = 0;      // 复合操作数位域：低 4 位 UnaryOp/BinaryOp，次 4 位 RegAxis/MarkerType/WeirdType
};

/// 运行时对象联合（m_objects 数组元素）。按 objIdx 取，OpCode 决定用哪个成员。
/// 用联合避免多态开销；编译期已知每个 objIdx 的实际类型（BytecodeGen 登记时确定）。
/// Marker 缓存对象注入也用 densityFunction 槽（缓存对象是 DensityFunction 子类）。
struct RuntimeObject {
    const ::mc::world::gen::noise::NormalNoise* noise = nullptr;
    const ::mc::world::gen::density::DensityFunction* densityFunction = nullptr;
    const ::mc::world::gen::density::Beardifier* beardifier = nullptr;
};

/**
 * @brief 扁平指令序列求值器
 *
 * 维度级编译产物（不可变）。持 Op 序列 + 寄存器大小 + 运行时对象表 + 子求值器表 +
 * 样条表 + 编译期记录的 minValue/maxValue + 是否含 Marker/Beardifier 标记。
 * 单点求值 eval(x,y,z) 遍历 Op 序列写寄存器，遇到 Return 返回。
 *
 * 区块级 newInstance（阶段5）：对含 MARKER 的序列做原地替换（缓存对象注入 m_objects），
 * 产出区块级求值器。不含 MARKER/BEARDIFIER 的子求值器直接共享维度级实例（零深拷贝）。
 */
class CompiledDensityFunction {
public:
    /// 维度级编译构造。minValue/maxValue 从原 DensityFunction 取，hasMarkerOrBeardifier
    /// 由 BytecodeGen 编译期标记（emitMarker/emitBeardifier 时置 true，递归子求值器继承）。
    /// ownedCaches 仅区块级 newInstance 填充（CacheOnce/FlatCache/Cache2D 拥有型缓存对象所有权），
    /// 维度级编译传空 vector。
    CompiledDensityFunction(std::vector<Op> ops,
        u32 regCount,
        std::vector<RuntimeObject> objects,
        std::vector<std::shared_ptr<CompiledDensityFunction>> subEvaluators,
        std::vector<std::shared_ptr<CompiledSpline>> splines,
        f64 minValue,
        f64 maxValue,
        bool hasMarkerOrBeardifier,
        std::vector<std::unique_ptr<::mc::world::gen::density::DensityFunction>> ownedCaches);

    /// 单点求值。遍历 Op 序列，switch 分发，写寄存器，Return 时返回 dst 寄存器值。
    /// 每次求值用栈上局部寄存器数组（线程安全）。
    [[nodiscard]] f64 eval(i32 x, i32 y, i32 z) const;

    /// 编译期记录的最小值（从原 DensityFunction 取，供 CompiledDensityFunctionAdapter::minValue 用）。
    [[nodiscard]] f64 minValue() const { return m_minValue; }
    /// 编译期记录的最大值。
    [[nodiscard]] f64 maxValue() const { return m_maxValue; }

    /// 是否含 MARKER/BEARDIFIER 指令（含则区块级 newInstance 必须递归替换，不含则可共享维度级）。
    [[nodiscard]] bool hasMarkerOrBeardifier() const { return m_hasMarkerOrBeardifier; }

    /// 区块级实例化（阶段5）：把 MARKER 占位替换为 per-chunk 缓存对象。
    /// 仅当 hasMarkerOrBeardifier() 为 true 时需要调用（否则直接 shared_from 持有维度级实例）。
    /// 实现见 CompiledDensityFunction.cpp（跨层依赖 NoiseChunk 缓存类 + Adapter）。
    /// chunk 提供 cell 几何 + m_interpolators/m_cellCaches 注册容器 + interpolationCounter 指针。
    /// 返回区块级求值器（shared_ptr，供 Adapter 包装）。
    [[nodiscard]] std::shared_ptr<CompiledDensityFunction> newInstance(
        ::mc::world::gen::density::NoiseChunk& chunk) const;

    // === 调试/阶段5 访问器 ===
    [[nodiscard]] const std::vector<Op>& ops() const { return m_ops; }
    [[nodiscard]] u32 regCount() const { return m_regCount; }
    [[nodiscard]] const std::vector<RuntimeObject>& objects() const { return m_objects; }
    [[nodiscard]] const std::vector<std::shared_ptr<CompiledDensityFunction>>& subEvaluators() const
    {
        return m_subEvaluators;
    }
    [[nodiscard]] const std::vector<std::shared_ptr<CompiledSpline>>& splines() const { return m_splines; }

private:
    std::vector<Op> m_ops;
    u32 m_regCount;
    std::vector<RuntimeObject> m_objects;
    std::vector<std::shared_ptr<CompiledDensityFunction>> m_subEvaluators;
    std::vector<std::shared_ptr<CompiledSpline>> m_splines;
    f64 m_minValue;
    f64 m_maxValue;
    bool m_hasMarkerOrBeardifier;

    /// 区块级 newInstance 拥有的缓存对象（CacheOnce/FlatCache/Cache2D 拥有型；
    /// NoiseInterpolator/CellCache 所有权在 NoiseChunk 容器，不在此）。
    /// 持 unique_ptr 保证缓存对象生命周期与区块级求值器一致。
    std::vector<std::unique_ptr<::mc::world::gen::density::DensityFunction>> m_ownedCaches;
};

} // namespace mc::world::gen::density::ast
