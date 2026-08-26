# 密度函数求值器 JIT 可行性评估

> 评估"用 asmjit 把密度 AST 求值器 JIT 成机器码"是否值得。
> 结论：**JIT 强烈值得**（interpreterRatio = 65.4%，226 区块全部 >40%）。
> 本文档沉淀测量设计、关键陷阱、修正前后数据、收益估算与保留。

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
