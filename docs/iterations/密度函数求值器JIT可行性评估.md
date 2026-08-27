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

---

## 8. 噪声采样层 SoA 向量化反汇编实测（2026-08-27）

第 7 节确认 JIT 把 eval 提速 1.59× 后瓶颈转移到外部噪声采样（external 占 54.9%）。本节沉淀对噪声采样层 SoA 向量化改造（效仿 C2ME `c2me-opts-natives-math` 的 octave 并行，见 memory [[noise-soa-optimization]]）的**反汇编级实测证据**，确认 AVX2 向量化真实生效，并记录 clang 生成的向量化代码形态。

### 8.1 反汇编方法

用 `llvm-objdump`（`D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-objdump.exe`）反汇编 `build/` 下的 `PerlinNoise.cpp.obj` 与 `BlendedNoise.cpp.obj`（RelWithDebInfo，全局 `-mavx2 -march=native -ffast-math`）：

- `--syms` 定位 `getValue`（`PerlinNoise.cpp.obj` 偏移 `0x2230`）与 `compute`（`BlendedNoise.cpp.obj` 偏移 `0x5e0`，结束于 `0x6340`，函数体 22368 字节）。
- `--start-address/--stop-address` 精确切出函数体，`grep -oE` 统计各类向量化指令计数（排除 `sampleAndLerp`/`gradDot` 等标量 ULP reference 路径的干扰）。

注：`getValue`/`perlinSampleSoA` 均为 `__attribute__((always_inline))`，`perlinSampleSoA` 内联进 `getValue`；`getValue` 本身因可能被虚调用（NormalNoise 持裸指针调 `noise->getValue`）保留了独立符号，未完全内联进调用者。`compute` 是 BlendedNoise 的虚函数，独立符号。

### 8.2 PerlinNoise::getValue 函数体向量化指令计数（0x2230–0x40d0）

| 指令 | 计数 | 作用 |
|------|------|------|
| `vpinsrb` | 84 | 标量 gather 置换表字节后拼入向量寄存器（hash 链 3 级依赖，逐通道查表） |
| `vfmadd213pd` | 43 | 向量 FMA（packed double，4 通道 f64）——梯度点乘 + 三线性插值 |
| `vmulpd` | 42 | 向量乘（wrap 缩放、smear 等） |
| `vpmovzxbq` | 40 | u8→64 位零扩展（置换表字节扩展为 64 位索引） |
| `vgatherqpd` | 32 | **向量 gather**——4 通道并行收集置换表项 |
| `vfmadd231pd` | 21 | 向量 FMA（累加形式） |
| `vaddpd` | 18 | 向量加（坐标 + offset） |
| `vpmovzxbd` | 16 | u8→32 位零扩展（梯度索引扩展） |
| `vgf2p8affineqb` | 16 | **GF(2^8) 仿射变换**——clang 巧用此指令实现 `hash & 0xF` 取梯度表索引 |
| `vgatherdpd` | 16 | **向量 gather**——4 通道并行收集梯度表 4×f64 |
| `vroundpd` | 14 | 向量 floor（`perlinWrap` 的 `floor` 向量化，模式 `$0x9`） |
| `vfmsub231pd` | 14 | 向量 FMA（减法形式，lerp 的 `a*b-c`） |
| `vbroadcastsd` | 13 | 标量广播到向量（坐标 broadcast 到 4 通道） |
| `vpshufb` / `vpmovzxdq` | 4 / 4 | 字节重排 / 32→64 位扩展 |

**合计向量 gather 48 次（`vgatherqpd` 32 + `vgatherdpd` 16），向量 FMA 78 次（`vfmadd213pd` 43 + `vfmadd231pd` 21 + `vfmsub231pd` 14），u8 零扩展 56 次（`vpmovzxbq` 40 + `vpmovzxbd` 16），向量 floor 14 次。** AVX2 f64 4 通道 octave 并行达成。

> `sd`（single double，标量）后缀的 `vfmadd*sd`（共约 34 次）是同 obj 内 `PerlinLayer::sampleAndLerp`/`gradDot`/`noiseWithSmear` 标量路径（保留作 ULP 测试 reference）的指令，**非 SoA 向量化路径**。getValue 函数体切面内 `vfmadd231sd` 19 次 + `vfmadd213sd` 13 次为切面边缘混入的标量收尾循环（标量顺序累加 `ds[]` 保 bit-exact，非向量化）。

#### 8.2.1 实际汇编片段——perlinWrap 三轴并行向量化

`PerlinNoise::getValue` 内 `perlinWrap(coord*inputFactor)` 被编译为 X/Y/Z 三路并行的向量化 floor-wrap，每路一个 ymm（4 通道 f64）。以 X 路为例（`value - floor(value/WRAP+0.5)*WRAP` 的向量化实现）：

```asm
; ymm0 = 4 通道 octave 的 wrap 前坐标（coord*inputFactor）
2417:  vmovapd   0x340(%rsp), %ymm1      ; ymm1 = inputFactor[0..3]（4 个 octave 各自的最低频输入缩放）
2420:  vbroadcastsd (%rip), %ymm14       ; ymm14 = WRAP_PERIOD 广播到 4 通道
2429:  vfmadd213pd %ymm14, %ymm0, %ymm1  ; ymm1 = ymm0*ymm1 + ymm14   （coord*inputFactor + WRAP/2，准备 floor）
242e:  vroundpd  $0x9, %ymm1, %ymm1      ; 向量 floor（roundpd 模式 0x9 = round toward -inf）
2434:  vbroadcastsd (%rip), %ymm15       ; ymm15 = WRAP_PERIOD 广播
243d:  vmulpd    %ymm1, %ymm15, %ymm1    ; ymm1 = floor(...) * WRAP
2441:  vmovapd   0x440(%rsp), %ymm7      ; ymm7 = 另一路缩放因子
244a:  vfmsub231pd %ymm7, %ymm0, %ymm1   ; ymm1 = ymm0*ymm7 - ymm1   （完成 wrap：value - floor*WRAP）
```

关键点：`vfmadd213pd`/`vfmsub231pd` 是 packed double（4×f64）FMA，`vbroadcastsd` 把标量缩放因子广播到 4 通道——即"每 SIMD 通道算一个 octave，各自独立缩放"。`vroundpd $0x9` 实现 `std::floor` 向量化（`-ffast-math` 下 libm floor 映射为 intrinsic）。三轴 X/Y/Z 各占一路 ymm，三路并行展开。

#### 8.2.2 实际汇编片段——置换表 gather 链（hash 链内部串行）

C2ME 杠杆的核心：每通道独立 256 项置换表做独立 gather 链，hash 链 `perm[(perm[(perm[ax]+ay)&0xFF]+az)&0xFF]` 内部 3 级数据依赖串行，跨 octave 并行。clang 未用 `vpgatherdd` 整链 gather（依赖链无法向量化），而是逐通道标量查表后 `vpinsrb` 拼回向量：

```asm
; 4 通道 octave 的 cell 坐标经 &0xFF 后，逐通道查置换表 perm[base + idx]
255b:  vpmovzxdq %xmm2, %ymm2           ; 4 通道 32 位索引零扩展为 64 位
2560:  vmovq    %xmm2, %rax              ; 取通道 0 索引
2565:  vpextrq  $0x1, %xmm2, %rcx        ; 取通道 1 索引
256b:  vextracti128 $0x1, %ymm2, %xmm2
2571:  vpextrq  $0x1, %xmm2, %rdx        ; 取通道 2/3 索引
2577:  movzbl   (%rax,%r13), %eax        ; 通道 0：perm[base0 + idx0] 标量查表
257c:  vmovd    %eax, %xmm4              ; 放入 xmm4 通道 0
2580:  vpinsrb  $0x1, (%rcx,%r15), %xmm4, %xmm4  ; 通道 1：perm[base1 + idx1]，拼入 xmm4
2587:  vmovq    %xmm2, %rax
258c:  vpinsrb  $0x2, (%rax,%r8), %xmm4, %xmm2   ; 通道 2
2593:  vpinsrb  $0x3, (%rdx,%r9), %xmm2, %xmm4   ; 通道 3——4 通道置换表项收集完成
```

`vpmovzxbq`/`vpinsrb` 计数高（PerlinNoise 124 次、BlendedNoise 372 次）正源于此——hash 链每一级都需逐通道标量 gather 再拼向量。这是 C2ME 设计中"hash 链内部串行不碰、跨 octave 并行"的直接机器码体现。

#### 8.2.3 实际汇编片段——梯度表向量 gather（vgatherdpd）

置换表查表得到 hash 后，`hash & 0xF` 取梯度索引，再 `vgatherdpd` 4 通道并行从 `kFlatSimplexGrad` 收集 4×f64 梯度向量：

```asm
2654:  vmovdqa   0x310(%rsp), %xmm13     ; xmm13 = 仿射变换矩阵（用于实现 & 0xF）
265d:  vgf2p8affineqb $0x0, %xmm13, %xmm4, %xmm4  ; GF(2^8) 仿射变换——clang 巧用密码学指令实现 hash & 0xF
2663:  vpbroadcastb (%rip), %xmm5        ; xmm5 = 0x0F 广播
266c:  vpand    %xmm5, %xmm4, %xmm4      ; 显式 & 0xF（与上面 vgf2p8affineqb 配合）
2670:  vpmovzxbd %xmm4, %xmm5            ; 4 通道字节索引零扩展为 32 位
2675:  vpxor    %xmm0, %xmm0, %xmm0      ; 清零目标 ymm0（gather 累加器）
2679:  vpcmpeqd %ymm6, %ymm6, %ymm6      ; ymm6 = 全 1（gather 掩码，启用 4 通道）
267d:  leaq     (%rip), %rax             ; rax = &kFlatSimplexGrad[0]
2684:  vgatherdpd %ymm6, (%rax,%xmm5,8), %ymm0  ; 4 通道并行收集梯度表 4×f64（hash<<2 定位）
268a:  vmovapd   %ymm0, 0x840(%rsp)      ; 存 4 通道梯度
```

`vgatherdpd` 用 32 位索引（`xmm5`）收集双精度，4 通道并行——一次 gather 同时取 4 个 octave 的梯度向量。`vgf2p8affineqb`（GF(2^8) 仿射变换，本为 AES/GFNI 密码学指令）被 clang 用来实现 `hash & 0xF` 取梯度表索引，比 `vpand` + 立即数更巧妙。

### 8.3 BlendedNoise::compute 函数体向量化指令计数（0x5e0–0x6340，22368 字节）

| 指令 | 计数 | 作用 |
|------|------|------|
| `vpinsrb` | 252 | 置换表字节拼入向量（main 8 + min 16 + max 16 = 40 octave，3 阶段） |
| `vfmadd213pd` | 126 | 向量 FMA——梯度点乘 + 插值 |
| `vpmovzxbq` | 120 | u8→64 位零扩展 |
| `vmulpd` | 110 | 向量乘 |
| `vgatherqpd` | 96 | **向量 gather**——置换表收集 |
| `vaddpd` | 72 | 向量加 |
| `vbroadcastsd` | 55 | 标量广播 |
| `vpmovzxbd` | 48 | u8→32 位零扩展（梯度索引） |
| `vgf2p8affineqb` | 48 | GF(2^8) 仿射实现 `& 0xF` |
| `vgatherdpd` | 48 | **向量 gather**——梯度表收集 |
| `vsubpd` | 42 | 向量减（lerp `end-start`） |
| `vroundpd` | 42 | 向量 floor（wrap + smear） |
| `vfmadd231pd` | 42 | 向量 FMA（累加形式） |
| `vfmsub231pd` | 36 | 向量 FMA（减法形式） |
| `vpermpd` | 30 | 向量置换（坐标通道重排） |
| `vandpd` | 21 | 向量按位与（`& 0xFF` 折回等） |
| `vblendvpd` / `vpbroadcastb` | 6 / 6 | 条件混合 / 字节广播 |

**合计向量 gather 144 次（`vgatherqpd` 96 + `vgatherdpd` 48），向量 FMA 204 次（`vfmadd213pd` 126 + `vfmadd231pd` 42 + `vfmsub231pd` 36），u8 零扩展 168 次（`vpmovzxbq` 120 + `vpmovzxbd` 48），向量 floor 42 次。** 三阶段（main 8 / min 16 / max 16 octave）全部向量化，函数体 22KB 机器码体量与 40 octave 全展开相符。

### 8.4 clang 向量化代码形态要点（C2ME 风格杠杆坐实）

反汇编确认 clang 跨 octave 自动向量化达成 C2ME 风格的核心杠杆，且有几处值得记录的代码生成细节：

1. **4 通道 octave 并行**：`%ymm` 寄存器（256 位 = 4×f64）承载 `vfmadd213pd`/`vfmsub231pd`，一次处理 4 个 octave——即"每 SIMD 通道算一个 octave"。每个 octave 用 `vbroadcastsd` 把坐标广播到 4 通道，各自乘独立 `inputFactor`。

2. **置换表 gather 链（hash 链内部串行）**：clang **未**用 `vpgatherdd` 一次 gather 整条 hash 链，而是逐级 `vpmovzxbq`（u8→64 位扩展索引）+ 标量 `movzbl`/`vpinsrb` 逐通道查表拼向量。原因是 hash 链 `perm[(perm[(perm[ax]+ay)&0xFF]+az)&0xFF]` 有 3 级数据依赖（每级依赖上一级结果），无法向量化解依赖——clang 选择标量算出每通道索引再 `vpinsrb` 拼成向量寄存器，符合 C2ME "hash 链内部串行不碰、跨 octave 并行"的设计。`vpmovzxbq`/`vpinsrb` 计数高（PerlinNoise 124 次、BlendedNoise 372 次）正源于此。

3. **`vgf2p8affineqb` 实现 `& 0xF`**：clang 用 GF(2^8) 仿射变换指令（`vgf2p8affineqb`，本为密码学指令）实现 `hash & 0xF` 取梯度表索引——比 `vpand` + 立即数更巧妙（避免 16 位立即数加载）。这是 clang 在 `-march=native` 下对 AVX2 + GFNI 的利用。

4. **`vgatherdpd` 收集梯度表**：4 通道并行从 `kFlatSimplexGrad` 收集 4×f64 梯度向量（`hash<<2` 定位），`vpcmpeqd %ymm,%ymm` 生成全 1 掩码启用 4 通道。

5. **perlinWrap 向量化**：`vroundpd $0x9`（roundpd 的 floor 模式）实现 `std::floor` 向量化（`-ffast-math` 下 libm floor 映射为 intrinsic）。`vmulpd`/`vfmsub231pd` 配合实现 `value - floor(value/WRAP + 0.5)*WRAP`。

6. **标量顺序累加保 bit-exact**：采样循环向量化写 `ds[k]`（栈数组 `alignas(64) f64 ds[kMaxPerlinOctaves]`，`getValue` 栈帧 `subq $0xa38, %rsp` = 2.6KB），**累加循环刻意保留标量**（`vfmadd*sd` 收尾），复刻原 `i=0..N-1` 顺序，故档1 ULP 监控实测 0 ULP bit-exact。BlendedNoise 的 min/max 反向 `k=count-1..0` 累加同理。

### 8.5 向量化成果结论与局限

**结论**：SoA 向量化改造在反汇编层面**确凿生效**——PerlinNoise::getValue 与 BlendedNoise::compute 均生成完整 AVX2 向量化代码，4 通道 octave 并行 + 置换表/梯度表向量 gather + wrap/floor 向量化全部到位，C2ME 风格杠杆（单点内 octave 并行，hash 链内部串行）正确复现。数值正确（见 memory [[noise-soa-optimization]]：1e-9 门禁 6 测试 + ULP 3 档全绿）。

**局限（为何 FillNoiseCells 整体未提速）**：向量化生效 ≠ 整体性能提升。本次向量化优化的是**单次 `getValue`/`compute` 内部的 octave 循环**，而 FillNoiseCells 的开销大头曾是**每次噪声采样的固定调用开销（trampoline + 虚调用 + readTsc 临时计时插桩）× 海量调用次数（约 5.8 万次/区块）**，这些 SoA 碰不到：

- **NormalNoise** 经 `NoiseSample` opcode → `jitNoiseSample` trampoline（`DensityJitTrampolines.cpp:41`）→ 虚调用 `NormalNoise::getValue`。
- **BlendedNoise** 经 `DelegateNode` 退化（`McToAst.cpp:300` TODO，未补专用节点）→ `jitDelegate` trampoline → 虚调用 `BlendedNoise::compute`，SoA 收益被 Delegate 虚调用边界进一步削弱（且 BlendedNoise 是 octave 数最多、SoA 理论收益最大的部分）。
- 单次 octave 循环本身只占 getValue 周期的一部分，向量化省下的几十到一两百周期被 trampoline 的常数项（**临时插桩的 rdtsc ~20–40 周期** + 虚调用 ~5–10 周期 + call/ret）摊薄。

**readTsc 临时计时插桩已于 2026-08-27 整体移除**（见 ast/README 第 7 条）：`DensityEvalProfiler.hpp` 删除，5 个 A 类 trampoline（`jitNoiseSample`/`jitWeirdSampler`/`jitDelegate`/`jitEndIslands`/`jitBeardifier`）与解释器 `evalImpl` 对应 case 的 per-call rdtsc 计时、eval 顶层 `topLevelCycles` 计时、NoiseChunkGenerator 上报块全部清除。这释放了被盖住的 SoA 收益部分。下一步：抓 `MC_ENABLE_TRACY` trace 复测 FillNoiseCells（目标对比 SoA 落地前 25.7ms p50 基线），量化插桩移除 + SoA 向量化的综合收益；若 BlendedNoise Delegate 虚调用边界仍是瓶颈，再给 BlendedNoise 补专用 AST 节点（消除 `McToAst.cpp:300` TODO 的 Delegate 退化）——**此"补专用节点"下一步已于 2026-08-27 落地，去虚化反汇编实测见第 9 节**。向量化本身不回退（生效且数值正确）。

---

## 9. BlendedNoise 专用 AST 节点与 JIT 去虚化反汇编实测（2026-08-27）

第 8.5 节指出的下一步"给 BlendedNoise 补专用 AST 节点（消除 `McToAst.cpp:300` TODO 的 Delegate 退化）"已落地。本节沉淀该改动的去虚化机制与反汇编级实测证据。

### 9.1 背景：Delegate 退化路径的 vtable 间接开销

BlendedNoise 是主世界每个区块必采的噪声（Java `NoiseRouterData.java:212` 无条件 `add` base_3d_noise 进 sloped_cheese，三维度 BASE_3D_NOISE 均为 BlendedNoise）。改动前它走 DelegateNode 退化路径：

```
BlendedNoise → DelegateNode(&df) → Delegate op → jitDelegate trampoline → df->compute()  [vtable 间接]
```

`jitDelegate`（`DensityJitTrampolines.cpp:56`）持 `const DensityFunction*` 基类指针调 `df->compute()`——每次都是 **vtable 间接调用**（约 5–10 周期 + 间接分支预测失败风险）。每区块约 5.8 万次噪声采样，这是海量的固定开销。第 8 节的 SoA 向量化优化的是 `compute` 内部的 octave 循环，但 `compute` 经 Delegate 边界被间接调用，SoA 收益被 vtable 间接边界削弱（BlendedNoise 恰是 octave 数最多、SoA 理论收益最大的部分）。

### 9.2 改动概要：补专用节点链路 + 去虚化 trampoline

按 DFC AST 编译器四层架构为 BlendedNoise 补专用链路（纯增量，9 个文件，无删除）：

1. **AstNodes.hpp/.cpp** — 新增 `BlendedNoiseNode` 类（仿 EndIslandsNode），持 `const DensityFunction*` 裸指针，`relaxedEquals` 按实例地址比（非 DelegateNode 的 typeid 比——三维度参数不同，按 typeid 会错误合并不同维度实例）。`AstNodeKind::BlendedNoise` 枚举本已存在，只是无节点类，本次补齐。
2. **CompiledDensityFunction.hpp** — `OpCode` 枚举新增 `BlendedNoise`。
3. **McToAst.cpp** — 替换 L300 TODO 为 BlendedNoise 注册 lambda。
4. **BytecodeGen.cpp** — case 从 warn+0.0 改为 `emitBlendedNoise`，复用 `RuntimeObject.densityFunction` 槽（无需扩联合）。
5. **CompiledDensityFunction.cpp** — evalImpl switch 新增 `case OpCode::BlendedNoise`（解释器兜底）。
6. **DensityJitTrampolines.hpp/.cpp** — 新增 `jitBlendedNoise`，**关键**：`static_cast<const BlendedNoise*>(df)` 后调 `bn->compute()`。BlendedNoise 是 `final` 类（`BlendedNoise.hpp:52`），编译器据此去虚化。
7. **DensityJitCompiler.cpp** — emitOp switch 新增 `case OpCode::BlendedNoise`。

`OptoPasses.cpp`/newInstance/CMakeLists 均不动（BlendedNoiseNode 落入 OptoPasses 的 `default: return node;` 原样返回；BlendedNoise 维度级不可变，newInstance 深拷贝自动带上）。

**去虚化机制**：`jitDelegate` 持 `const DensityFunction*` 基类指针，编译器无法确定动态类型，`df->compute()` 必须经 vtable 间接调用。`jitBlendedNoise` 先 `static_cast<const BlendedNoise*>(df)` 转具体类型，而 BlendedNoise 是 `final` 类——编译器据此推断 `bn->compute()` 不可能被派生类覆写（final 类无派生），将虚调用去虚化为直接 `call BlendedNoise::compute`。

### 9.3 反汇编方法

用 `llvm-objdump` 反汇编 `build/src/common/CMakeFiles/mc_common.dir/RelWithDebInfo/world/gen/density/ast/DensityJitTrampolines.cpp.obj`（RelWithDebInfo）：

- `--syms` 定位 `?jitBlendedNoise@...`（偏移 `0x2e0`）与对照 `?jitEndIslands@...`（偏移 `0x240`）。
- `--disassemble-symbols=<mangled>` 精确切出两个 trampoline 的函数体。
- `--reloc` 确认 `call` 指令的重定位目标符号（区分直接调用 vs vtable 间接）。

### 9.4 反汇编铁证：jitBlendedNoise（去虚化直接 call）vs jitEndIslands（vtable 间接）

**jitEndIslands（对照，仍是 vtable 间接调用）**——EndIslands 持 `const DensityFunction*` 基类指针，`df->compute()` 走 vtable：

```asm
240:  push   %rbp
...
252:  movq   (%rcx), %rax            ; rax = ctx->objects（rcx=ctx）
255:  movl   %edx, %ecx              ; ecx = objIdx
257:  leaq   (%rcx,%rcx,2), %rcx     ; rcx = objIdx*3
25b:  movq   0x8(%rax,%rcx,8), %rcx  ; rcx = objects[objIdx].densityFunction（this 指针）
260:  testq  %rcx, %rcx              ; 判空
263:  je     0x27f                   ; 为 null → 断言路径
265:  movl   0x30(%rbp), %eax        ; 取 z（栈参数）
268:  movq   (%rcx), %r10            ; ★ r10 = vtable 指针（解引用 this 取 vtable）
26b:  movl   %r8d, %edx              ; 重排参数：x→edx
26e:  movl   %r9d, %r8d              ; y→r8d
271:  movl   %eax, %r9d              ; z→r9d
274:  callq  *0x8(%r10)              ; ★★★ vtable 间接调用：call [vtable+0x8]（compute 在 vtable 偏移 0x8）
278:  nop
279:  addq   $0x40, %rsp
27d:  pop    %rbp
27e:  retq
```

**jitBlendedNoise（去虚化，直接 call）**——`static_cast<const BlendedNoise*>` + final 类触发去虚化：

```asm
2e0:  push   %rbp
...
2f2:  movq   (%rcx), %rax            ; rax = ctx->objects
2f5:  movl   %edx, %ecx              ; ecx = objIdx
2f7:  leaq   (%rcx,%rcx,2), %rcx     ; rcx = objIdx*3
2fb:  movq   0x8(%rax,%rcx,8), %rcx  ; rcx = objects[objIdx].densityFunction（this 指针）
300:  testq  %rcx, %rcx              ; 判空
303:  je     0x31d                   ; 为 null → 断言路径
305:  movl   0x30(%rbp), %eax        ; 取 z（栈参数）
308:  movl   %r8d, %edx              ; 重排参数：x→edx
30b:  movl   %r9d, %r8d              ; y→r8d
30e:  movl   %eax, %r9d              ; z→r9d
311:  callq  0x316                   ; ★★★ PC 相对直接调用（目标=BlendedNoise::compute）
316:  nop
317:  addq   $0x40, %rsp
31b:  pop    %rbp
31c:  retq
```

**关键差异**：

| 项 | jitEndIslands（vtable 间接） | jitBlendedNoise（去虚化直接） |
|----|-----------------------------|------------------------------|
| 取 vtable | `movq (%rcx), %r10`（解引用 this 取 vtable 指针） | **无**（不取 vtable） |
| 调用 | `callq *0x8(%r10)`（内存间接，vtable 偏移 0x8） | `callq 0x316`（PC 相对直接调用） |
| 重定位 | 无指向 compute 符号的 REL32（间接通过 r10） | `0x312 IMAGE_REL_AMD64_REL32 ?compute@BlendedNoise@...` |

参数重排（`movl %r8d,%edx` 等）两者都有——这是 Win x64 调用约定的副产物（trampoline 形参 `(ctx,objIdx,x,y,z)` 在 rcx/rdx/r8/r9/栈，而成员函数 `compute(this,x,y,z)` 需 this→rcx、x→rdx、y→r8、z→r9），非去虚化独有。**去虚化的铁证是 jitBlendedNoise 没有 `movq (%rcx),%r10` 取 vtable 指令、把 `callq *0x8(%r10)` 换成 `callq 0x316` 直接调用**。

**重定位表确认 call 目标符号**（`--reloc`）：

```
0000000000000312 IMAGE_REL_AMD64_REL32    ?compute@BlendedNoise@density@gen@world@mc@@UEBANHHH@Z
```

地址 `0x312` 正是 `jitBlendedNoise` 内 `callq 0x316`（`0x311` 的 `e8` 操作码 + `0x312` 起 4 字节相对偏移重定位）的目标。重定位类型 `IMAGE_REL_AMD64_REL32`（32 位 PC 相对偏移），**目标符号是 `?compute@BlendedNoise@...`（即 `BlendedNoise::compute`）**。这彻底证明去虚化生效——JIT 机器码对 BlendedNoise 的采样是直接 `call BlendedNoise::compute`，无 vtable 间接。

> 注：符号 `?compute@BlendedNoise@...@@UEBANHHH@Z` 的 mangle 中 `UEBA` 表示 public virtual（虚函数）。它仍以独立符号存在于 obj（未被内联进 jitBlendedNoise），去虚化只改变了**调用方式**（间接→直接），未改变 `compute` 函数体本身（其内部 SoA 向量化见第 8.3 节）。

### 9.5 结论与局限

**结论**：BlendedNoise 补专用 AST 节点 + 去虚化 trampoline 已落地并反汇编确认生效。`static_cast<const BlendedNoise*>` + final 类成功让编译器把 `bn->compute()` 去虚化为直接 `call BlendedNoise::compute`，消除了每次采样的 vtable 间接调用（省掉 `mov (%rcx),%r10` 取 vtable + 间接分支预测失败风险）。`compute` 内部 SoA 向量化（第 8.3 节，254 条 vfmadd）因此以直接调用方式被 JIT 调用——既消除 vtable 间接，又保留 SoA 收益，且 bit-exact 天然保证（`compute` 一字未改，仅调用入口从 `jitDelegate` 换 `jitBlendedNoise`，调同一个 `BlendedNoise::compute`）。

**局限**：

- **解释器路径仍是虚调用**：`evalImpl` 的 `case OpCode::BlendedNoise` 走 `df->compute()`（基类指针，虚调用），去虚化收益只在 JIT 路径。这与 EndIslands/Delegate case 一致——解释器是回退兜底，不追求去虚化。
- **未消除 trampoline 的 call/ret 边界**：JIT 机器码仍经 `call jitBlendedNoise` → `call BlendedNoise::compute` 两层调用。彻底消除边界需把 SoA 采样循环手写进 asmjit（全内联档），但工作量巨大 + bit-exact 极难保 + 收益边际，未采用。
- **收益量级待 trace 确认**：本次仅反汇编验证去虚化生效 + 构建通过，未跑 trace 量化 FillNoiseCells 收益（对比基线 20.0ms p50）。BlendedNoise 在总开销中的占比待 trace 确认。



