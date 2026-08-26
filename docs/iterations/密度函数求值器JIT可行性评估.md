# 密度函数求值器 JIT 可行性评估

> 评估"用 asmjit 把密度 AST 求值器 JIT 成机器码"是否值得。
> 评估结论：**JIT 强烈值得**（interpreterRatio = 65.4%，226 区块全部 >40%）。
> 落地实测结论：**JIT 已落地，eval 提速约 1.59×（耗时降约 37%）**，详见第 7 节。
> 本文档沉淀测量设计、关键陷阱、修正前后数据、收益估算、落地实测与后续方向。

---

## 1. 背景

`CompiledDensityFunction::eval`（`src/common/world/gen/density/ast/CompiledDensityFunction.cpp:321`）是区块生成逐方块热路径，每区块调用数万次。当前实现是"扁平指令序列（`std::vector<Op>`）+ switch 解释执行"，约 25 个 `OpCode`，写入 `f64` 寄存器数组，遇 `Return` 返回。

用户想评估用 asmjit 把 eval JIT 成原生机器码是否值得。

### JIT 能优化什么、不能优化什么

- **能优化（解释器自身开销）**：switch 分发、Op 取指、regs 数组间接寻址、常量加载、LoadConst/Coord/Binary/Copy/Unary 等纯算术指令。
- **救不了（外部 C++ 调用）**：`NoiseSample`→`NormalNoise::getValue`、`Delegate`→原版 DF `compute`、Marker 区块级缓存对象 `compute` 等真正的噪声采样/缓存查表。

因此关键问题是：**eval 总时间里，解释器开销 vs 外部调用开销各占多少？**

历史背景：先前 SoA 优化（NormalNoise/BlendedNoise octave 拍平向量化）因数值正确但性能倒退被回退（commit 495832dd9），修复 A（栈寄存器）+ 修复 B（min/max 短路）后 p50≈25.7ms，短路耗尽后瓶颈疑似转移到噪声采样。本次评估旨在用硬数据验证，避免重演盲目优化。

---

## 2. 测量设计：双桶差值法

### 核心公式

```
interpreterCycles = topLevelCycles − externalCycles   （JIT 能优化的上限）
topLevelCycles    = depth==0 eval 入口→Return 总周期（覆盖整棵树含递归层 + fillSlice 角点填充）
externalCycles    = 5 个真叶子外部调用 per-call 计时累加
interpreterRatio  = interpreterCycles / topLevelCycles
```

### 决策标准

| interpreterRatio | 结论 |
|------------------|------|
| ≥0.40 | JIT 强烈值得 |
| 0.30–0.40 | JIT 值得 |
| 0.20–0.30 | 灰色地带，结合绝对耗时权衡 |
| <0.20 | 不值得，转 SIMD 批量化/噪声层 |

### 外部调用分类（防双重计数的关键）

evalImpl 的外部调用指令必须区分两类，否则双重计数：

**A 类——叶子外部调用（直接计时）**：执行流离开 evalImpl 进入外部 C++ 函数，**不递归回 eval**。
- `NoiseSample`（`noise->getValue`）
- `WeirdSampler`（`noise->getValue`，稀有度缩放）
- `Delegate`（`df->compute`，df 是原版 OOP DF 裸指针，不回 eval）
- `EndIslands`（专门的 endIslands 实例 compute）
- `Beardifier`（`beardifier->compute`）

**B 类——递归外部调用（不计时）**：调用 `sub->eval` 或 `cacheObj->compute` 进入子层 evalImpl，子树耗时已含在顶层 `topLevelCycles` 内，子树内叶子由子层计时。父层再计时会双重计数。
- `SharedSubtreeCall`（`sub->eval`）
- `Spline`（`evalSpline` 内含 valueEvaluators 递归 + Hermite）
- `FindTopSurface`（`evalFindTopSurface` 内含 densitySub->eval 循环 + 循环控制）
- **`Marker` 区块级**（`cacheObj->compute`，见下）

### Marker 归类——本次评估的关键陷阱

Marker 区块级走 `cacheObj->compute`，缓存对象（NoiseInterpolator/CellCache/CacheOnce/FlatCache/Cache2D）的 compute **在缓存未命中时回调 `m_filler->compute`**，而 filler 是 `CompiledDensityFunctionAdapter`，其 `compute` → `CompiledDensityFunction::eval` **递归**。

证据链：
- `newInstance`（CompiledDensityFunction.cpp:260）：`filler = make_unique<CompiledDensityFunctionAdapter>(delegate)`
- `NoiseInterpolator::compute`（NoiseChunk.cpp:144）：未命中时 `return m_filler->compute(...)`
- `CellCache::compute`（NoiseChunk.cpp:259/273）：未命中时 `return m_filler->compute(...)`
- `Adapter::compute`（CompiledDensityFunctionAdapter.hpp:57-60）：`return m_compiled->eval(...)`

故 Marker 的 `cacheObj->compute` 是 B 类递归调用，**不可计时**。其缓存查表/插值开销归入 `interpreterCycles`（会高估 JIT 收益，给出乐观上界——若上界仍不值得，JIT 肯定不值得）。

---

## 3. 插桩实现

### 时钟源：rdtsc（不用 steady_clock）

- 用 `__rdtsc()`（Win）`__builtin_readcyclecounter()`（ARM64），~10ns/读。
- **不用 steady_clock**：单次读取 ~100-200ns，而一次 eval 有多个 NoiseSample（每个都计时），per-call 计时会让测量开销达 20-40%，严重失真。
- **不用 rdtscp**：会等前序指令退休，在外部调用前用会把延迟计入 t0 之前的串行化等待。
- 用 `__rdtsc()` + 编译器屏障（`_ReadWriteBarrier()` MSVC / `asm volatile("":::"memory")` GCC/Clang），不加 CPU mfence（会序列化流水线放大开销）。
- 返回原始 cycle，**不换算纳秒**——ratio 是同量纲比值，无需换算。

### thread_local 累加器 + 递归深度守卫

```cpp
struct DensityEvalAccumulator {
    u64 topLevelCycles = 0;   // depth==0 eval 入口→Return 累加
    u64 externalCycles = 0;   // A 类叶子外部调用累加
    u64 topCalls = 0;
    u64 externalCalls = 0;
    u32 depth = 0;            // 当前递归深度（reset 不归零，跨 eval 维持）
    void reset() noexcept;    // depth 不归零
};
inline thread_local DensityEvalAccumulator g_densityEvalAcc;  // 多 worker 并行，单区块串行
```

- 多 worker 线程并行生成不同区块（UniversalWorkerPool），单区块内串行求值，故 thread_local 无锁安全。
- `DepthGuard` RAII（构造 ++/析构 --）保证 depth 在任何返回路径（含异常/early-return/missing-Return assert）前回退。

### eval 顶层计时

```cpp
f64 eval(x,y,z) {
    auto& acc = g_densityEvalAcc;
    const bool isTop = (acc.depth == 0);
    const u64 tEnter = isTop ? readTsc() : 0;   // 仅顶层读入口时间
    DepthGuard depthGuard;                       // ++depth
    f64 result = evalImpl(...);                  // A 类指令累加 externalCycles
    if (isTop) {
        acc.topLevelCycles += (readTsc() - tEnter);
        acc.topCalls += 1;
    }
    return result;
}
```

顶层 topLevelCycles 覆盖整棵树（含递归层）总耗时；子层 eval `isTop=false`，tEnter=0，不重复计入。

### 上报点

`NoiseChunkGenerator.cpp` 的 `_generateNoiseWithDensityFunction` 出口，`noiseChunk.stopInterpolation()` 之后、`chunk.setChunkStatus(ChunkStatuses::NOISE)` 之前。单区块 NOISE 阶段结束，fillSlice 阶段的所有 eval 都在 stopInterpolation 之前发生，已被累加器捕获，reset 在上报后——一个区块的所有 eval 被正确归集到这一个上报点。

上报 6 个 counter（`MC_TRACE_COUNTER`，双轨 Perfetto+Tracy）：`totalCycles`/`externalCycles`/`interpreterCycles`/`topCalls`/`externalCalls`/`interpreterRatio_x1000`。

### 文件清单（临时性插桩，profiler 完成后整体删除）

- 新增：`src/common/world/gen/density/ast/DensityEvalProfiler.hpp`（纯头文件：readTsc + 累加器 + DepthGuard + isLeafExternalCall）
- 修改：`src/common/world/gen/density/ast/CompiledDensityFunction.cpp`（eval depth 守卫 + 顶层计时；evalImpl 5 个 A 类 case per-call 计时；Marker 不计时）
- 修改：`src/common/world/gen/chunk/NoiseChunkGenerator.cpp`（上报点）

**约束**：不修改 `ProfilerConfig.hpp`（会导致大量文件重编译）；不用宏控制（profiler 完毕后代码删除）；插桩不进入浮点运算路径，`DensityAstBaselineTest`（1e-9）不受影响。

---

## 4. 测量数据

### 修正前（Marker 误计时——失真数据）

| 指标 | avg | count |
|------|-----|-------|
| totalCycles | 133,467,159 | 220 |
| topCalls | 158,922 | — |
| externalCycles | **151,057,192** | — |
| externalCalls | 344,860 | — |
| interpreterCycles | 2,463,506（大量为 0） | — |
| interpreterRatio_x1000 | **24.1**（ratio ≈ 2.4%） | 148，max 256 |

**致命悖论**：`externalCycles (151M) > totalCycles (133M)`——数学上不可能（externalCycles 理论上是 totalCycles 子集）。根因：Marker 误归 A 类计时，其 cacheObj->compute 子树被父层 + 子层双重计数，Marker 是主世界路由里数量最多的指令，把 externalCycles 顶到 totalCycles 之上，把 interpreterCycles 压成负→归零→ratio 失真压低到 2.4%。

**此数据无效，不可作为决策依据。**

### 修正后（Marker 不计时——可信数据）

| 指标 | avg | count | min | max |
|------|-----|-------|-----|-----|
| totalCycles | 95,424,121 | 226 | 61,551,271 | 324,697,289 |
| topCalls | 157,751 | — | 126,858 | 404,790 |
| externalCycles | 34,104,785 | — | 20,639,947 | 152,723,661 |
| externalCalls | 53,338 | — | 31,884 | 266,057 |
| interpreterCycles | 61,319,102 | — | 40,593,577 | 176,809,049 |
| **interpreterRatio_x1000** | **653.67**（ratio ≈ **65.4%**） | 226 | 529 | 725 |

**健康检查通过**：`externalCycles (34.1M) ≤ totalCycles (95.4M)`，悖论消失，Marker 双重计数已消除。

**结论**：interpreterRatio = 65.4%，226 个区块全部 >52.9%，极稳定，远超 40% "强烈值得"阈值。

### 逐 eval 拆解（数据自洽性验证）

- `cycles/eval = 95.4M / 157,751 = 605`
- `cycles/外部调用 = 34.1M / 53,338 = 639`（接近 NormalNoise 8-octave Perlin 的 >500 cycles 预期 ✓）
- **`外部调用/eval = 53,338 / 157,751 = 0.34`** ← 关键

每次 eval 仅 0.34 次外部调用，即**三分之二的 eval 根本不采样噪声**，是纯算术运算（LoadConst/Coord/Binary/Copy/Unary/插值）。这些纯算术 eval 的解释器开销（switch 分发 + Op 取指 + regs 间接寻址）累积成了 65.4% 的大头——这正是 JIT 的甜区。

---

## 5. 保留与不确定性

### interpreterCycles 含 Marker 缓存查表（高估 JIT 收益）

Marker 不计时后，其 `cacheObj->compute` 命中缓存走查表的耗时归入了 `interpreterCycles`。缓存查表 JIT 救不了（cacheObj 是多态对象，compute 是虚调用 + 数组查表），所以这部分高估了 JIT 收益。

保守扣除估算：Marker 查表是 O(1) 数组索引，单次极快（~10-20 cycles）；解释器开销 388 cycles/eval（= 605 × 0.64）。假设 Marker 查表占 interpreterCycles 的 30-50%（已很悲观），扣除后真实 JIT 可优化比例仍为 **32-46%**，仍落在"值得/强烈值得"区间。

故结论稳健：即便考虑 Marker 查表高估，JIT 仍值得。

### 收益估算（保守）

JIT 能消除解释器开销的 60-70%（非 100%，因仍有寄存器 spill/调用约定残留）：
- eval 提速 ≈ 65.4% × 65% ≈ 42%
- 若 eval 占 FillNoiseCells 约 80%：FillNoiseCells 提速 ≈ 34%
- 25.7ms → **约 17ms**

接近但未必到 <15ms 目标，需 JIT 原型实测定论。

### Beardifier 计时分支可能是死路径

Beardifier case 的计时在生产可能是死路径（注释说"维度级编译期 Beardifier 未注入，占位返回 0.0"）。若实际 `beardifier==nullptr`，计时记录的是 `0.0` 计算的 ~0 周期——不影响 ratio 正确性，但 `externalCalls` 计数会包含这些空调用。只影响 externalCalls 解读，不影响 ratio（cycles 比值）。

---

## 6. 决策与下一步

**决策**：ratio 65.4% 且 226 区块全稳定 >40%，纸面证据足够强，推进 JIT 原型实测（用 asmjit 把 eval 的扁平指令序列 JIT 成机器码：switch 分发→直接跳转、regs 数组→寄存器分配、常量内联），跑 benchmark 拿实际加速比作为最终判据。

后续实现要点（供 JIT 原型参考）：
- asmjit 把 `std::vector<Op>` 编译为本地函数指针，eval 改为调用该指针。
- A 类外部调用指令保留为 `call`（NoiseSample/Delegate 等仍调原 C++ 函数）。
- 内部类指令（LoadConst/Coord/Binary/Copy/Unary）内联为算术指令，regs 数组尝试寄存器分配。
- B 类递归调用（SharedSubtreeCall/Spline/FindTopSurface/Marker）保留为对子求值器/缓存对象的 `call`。
- 必须保持浮点累加顺序 bit-exact（`DensityAstBaselineTest` 1e-9 基线，见 memory [[dfc-ast-compiler-project]] DFC relaxedEquals 标量修正陷阱）。

---

## 7. JIT 落地实测（2026-08-26，commit 84dc56463 + 686f8473b）

JIT 原型已落地并验证通过。本节沉淀落地后的实测数据与收益量化结论，回填第 6 节"需 JIT 原型实测定论"的判据。

### 7.1 落地架构概要

- **维度级编译一次**：`BytecodeGen::compile` 末尾调 `CompiledDensityFunction::compileJit()` → `compileDensityJit(ops, regCount)`（`DensityJitCompiler.cpp`）用 `asmjit::x86::Compiler` 逐条 Op 翻译为虚拟寄存器指令，`rt.add` 产出函数指针 `m_jitFn`。
- **区块级复用**：`newInstance` 深拷贝 `m_ops`（字节相同）直接复用维度级 `m_jitFn`，只重建自己的 `DensityEvalContext`。维度级与区块级共享同一 JIT 机器码。
- **JIT 函数签名** `f64 jitEval(const DensityEvalContext* ctx, i32 x, i32 y, i32 z)`，ctx 驻留 callee-saved 寄存器。
- **外部调用经 trampoline**：9 个自由函数（`DensityJitTrampolines.cpp`，`jitNoiseSample`/`jitDelegate`/`jitSpline`/`jitFindTopSurface`/`jitMarkerDispatch` 等），JIT 代码不碰 C++ 对象布局。内部算术内联。
- **MARKER 运行时判空**：`cacheObj != null` 走 compute / 否则走 delegate eval，维度级（占位）与区块级（注入缓存）共享同一 JIT 代码。
- **失败回退**：JIT 编译失败 / 非 Win x64 / 空 ops → `m_jitFn=nullptr` → eval 回退 `evalImpl`，记 `spdlog::warn`。macOS ARM64 留 TODO（a64::Compiler，须避免 fmadd 融合保 bit-exact）。
- **性能计数器保留**：eval 顶层仍计 `topLevelCycles`/`topCalls`；解释器 `evalImpl` 5 个 A 类 case 的 per-call 计时保留；JIT trampoline 同步加 `readTsc` 计时（见 7.3）。

### 7.2 核心 bug——ANDPS m128 未对齐触发 #GP（耗时最长的卡点）

Abs 翻译用 `andps dst, [const]` 清符号位，掩码 `0x7FFFFFFFFFFFFFFF` 经 `newInt64Const` 入 asmjit 常量池（8 字节对齐）。但 **ANDPS 是 SSE packed 指令，其 m128 内存操作数要求 16 字节对齐**，8 字节对齐地址触发 **#GP（常规保护异常）而非 #PF**。

关键诊断难点：#GP 不像 #PF 那样向 VEH 报告具体故障地址，故 `ExceptionInformation[1]` 是垃圾值 `0xffffffffffffffff`，与"读入有效常量内存 [0x68]=0x7fff... 却崩"表面矛盾，长期误导排查方向。

修复：掩码先 `movsd`（标量，8 字节对齐足矣）加载到临时 Xmm 寄存器，再用**寄存器-寄存器** `andps dst, mask`（无内存操作数，不触发 #GP）。

教训：**asmjit 常量池 int64 条目仅 8 字节对齐，任何 128 位 packed 指令（andps/andpd/xorps/orps/pand 等）配内存操作数都会 #GP；必须先标量加载到寄存器再寄存器-寄存器操作。**

### 7.3 JIT 路径计数器口径修正（commit 686f8473b）

落地初期 JIT trampoline 未插桩，导致 JIT 路径下 `externalCycles`/`externalCalls` 恒为 0，`interpreterRatio` 失真为 100%（`interpreterCycles = total − 0 = total`），无法拆分"JIT 省的解释器开销"与"噪声采样固有开销"。

修正：在 5 个 A 类叶子 trampoline（`jitNoiseSample`/`jitWeirdSampler`/`jitDelegate`/`jitEndIslands`/`jitBeardifier`）加 `readTsc` per-call 计时，口径与解释器 `evalImpl` **完全一致**（如 `jitWeirdSampler` 只计时 `noise->getValue(...)*r`，`getRarity` 在 `_t0` 外，对齐 evalImpl WeirdSampler case）。

- **A 类（计时）**：5 个真叶子 trampoline —— 现已插桩，JIT 路径 externalCycles 不再为 0。
- **B 类（不计时）**：`jitMarkerDispatch`/`jitCacheCompute`/`jitDelegateSubEval`/`jitSpline`/`jitFindTopSurface` —— 递归回 eval，子树叶子由子层计时，父层不计时（防双重计数，对齐解释器 SharedSubtreeCall 不计时）。
- **不计时**：`jitYGradient`/`jitSin`/`jitCos` —— 纯算术/libm，解释器也不计入 externalCycles。

### 7.4 实测数据（JIT 路径 + trampoline 计时）

201 区块采样：

| 指标 | avg | count | min | max |
|------|-----|-------|-----|-----|
| totalCycles | 68,331,670 | 201 | 36,755,634 | 219,835,301 |
| topCalls | 165,320 | 201 | 126,858 | 397,035 |
| externalCycles | 37,501,889 | 201 | 17,285,567 | 136,009,798 |
| externalCalls | 58,395 | 201 | 31,884 | 267,779 |
| interpreterCycles | 30,830,849 | 201 | 17,962,899 | 83,825,503 |
| **interpreterRatio_x1000** | **462**（≈ **46.2%**） | 201 | 365 | 545 |

健康检查：`externalCycles (37.5M) ≤ totalCycles (68.3M)` ✓；`interpreterCycles = total − external = 30.8M` ✓。

**关键自洽验证**：`externalCalls / topCalls = 58395 / 165320 = 0.35`，与纯解释器基线的 **0.34（53338/157751，见第 4 节）高度吻合**——证明 JIT 没改变调用模式，两套计数器口径一致，跨路径比较可靠。

### 7.5 interpreterRatio 在 JIT 路径下的语义变化

**注意：JIT 路径下已经没有"解释器"了**（JIT 机器码替换了 switch 解释器）。故此处的 46.2% **不是"解释器开销占比"**，而是 **JIT 机器码自身残留耗时 + B 类递归控制** 占 eval 总耗时的比例：

- JIT 内联算术（mulsd/addsd/clamp/lerp/spline Hermite 等）——真实计算，JIT 也消除不了；
- B 类 trampoline（Marker 缓存查表、Spline 二分、FindTopSurface 循环、SharedSubtree 递归）的控制流——JIT 没展开，留作 call。

即 JIT 已消除能消除的（switch 分发、Op 取指、regs 间接寻址），剩余 46.2% 是**不可消除的真实算术 + 递归控制**。

### 7.6 JIT 收益量化（间接估算）

利用"外部噪声采样成本跨路径近似恒定"这一合理假设（同一套 NormalNoise 实现，调用次数比 0.35≈0.34 一致），从两次 external 占比反推：

| 路径 | external 占比 | 非外部占比 | 来源 |
|------|--------------|-----------|------|
| 纯解释器（第 4 节基线） | 34.6%（34.1M/95.4M） | 65.4% | 评估期数据 |
| JIT（7.4 节） | 54.9%（37.5M/68.3M） | 45.1% | 本次实测 |

设外部噪声采样成本 E 跨路径不变：

- 解释器 total = E / 0.346 = 2.89E
- JIT total = E / 0.549 = 1.82E
- **JIT total / 解释器 total = 0.346 / 0.549 = 0.63**

**结论：JIT 路径 eval 耗时降至解释器的约 63%，即 eval 提速约 1.59×（耗时减少约 37%）。**

> 注：7.4 表 ratio 46.2% 与 7.6 用 45.1% 略有出入，因 7.4 是 201 区块 avg 的瞬时比，7.6 是基于 external/total 各自 avg 的比；两者一致到个位百分点，结论稳健。

收益来源拆分（解释器侧开销从 1.89E 降到 0.82E）：
- JIT **消除了约 57%** 的解释器侧开销（1.07E）——switch 分发/Op 取指/regs 间接寻址/常量加载；
- 剩余 43%（0.82E）是 JIT 也去不掉的真实算术 + B 类递归控制。

### 7.7 结论与后续方向

- **JIT 收益实测确认**：eval 提速约 1.59×（耗时降约 37%），略低于第 5 节保守估算的 42%，因残留算术/递归占比略高于预估——但仍是显著收益，且 JIT 已接近该方案理论上限。
- **瓶颈已转移到外部噪声采样**：JIT 路径下 external 占比升至 54.9%（解释器期 34.6%），说明 JIT 把非外部部分压低后，噪声采样成了新的大头。继续提升 eval 性能的边际收益递减——下一步应转向：
  - 减少外部噪声调用次数（缓存下沉、共享拓扑子树复用，见 memory [[density-sharedtopology-optimization]]）；
  - 噪声采样层 SoA/SIMD 批量化（见 memory [[noise-soa-optimization]]）；
  - B 类递归控制内联（Spline Hermite / FindTopSurface 循环展开），但收益有限且破坏 bit-exact 风险高。
- **精确 A/B 待定**：7.6 是间接估算（依赖跨路径恒定假设 + 不同运行）。若需同世界同区块 JIT 开/关精确对比，可加环境变量门控（如 `MC_DENSITY_JIT_DISABLE=1`）强制 eval 走解释器跑两次。当前间接估算的 0.35≈0.34 自洽验证已相当有力，是否需精确 A/B 视后续需求。
- **trampoline 计时插桩为临时性**：profiler 量化完成后，`DensityEvalProfiler.hpp` 及 eval/trampoline 内计时代码将整体删除（见第 3 节约束）。

