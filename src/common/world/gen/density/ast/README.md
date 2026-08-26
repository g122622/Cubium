# ast/ — 密度函数 AST 编译器子系统

## 概述

效仿 C2ME DFC，把 MC 密度函数树编译为扁平指令序列求值器（`CompiledDensityFunction`），再在 Windows x64 上经 asmjit JIT 成本机机器码，消除 switch 解释器开销。对外入口是 `RandomState`（维度级编译）与 `NoiseChunk`（区块级 `newInstance` 复用 + Adapter 包装）。

## 目录结构

```
ast/
├── AstNode.hpp                        — AST 节点抽象基类（不可变，支持 persistence sharing/子树去重）
├── AstNodes.hpp                       — 全部 AST 节点类型定义（binary/unary/leaf/noise/spline/control/marker/特有）
├── AstNodes.cpp                       — 节点实现
├── McToAst.hpp                        — DensityFunction 树 → AST 转换入口
├── McToAst.cpp                        — 实现
├── OptoPasses.hpp                     — AST 优化遍（常量折叠/死代码消除/子树共享等）
├── OptoPasses.cpp                     — 实现
├── BytecodeGen.hpp                    — AST → 扁平 Op 指令序列 + 编译期寄存器分配
├── BytecodeGen.cpp                    — 实现（末尾调 compileJit 维度级编译一次）
├── CompiledDensityFunction.hpp        — 扁平指令序列求值器（eval JIT/evalImpl 解释器/DensityEvalContext/DensityJitFn）
├── CompiledDensityFunction.cpp        — 实现（eval 入口 JIT 分支 + evalImpl switch + newInstance 继承 JIT + compileJit）
├── CompiledDensityFunctionAdapter.hpp — 把 CompiledDensityFunction 包装为 DensityFunction 子类的桥接 Adapter（供缓存类 filler 持有）
├── DensityEvalHelpers.hpp             — 求值纯函数集（clampedLerp/clampedMap/getRarity/evalSpline/evalFindTopSurface 等，解释器与 JIT trampoline 共享）
├── DensityEvalHelpers.cpp             — 实现
├── DensityEvalProfiler.hpp            — 临时 eval 性能插桩（readTsc + g_densityEvalAcc 累加器 + DepthGuard，量化 JIT 收益用）
├── DensityJitCompiler.hpp             — compileDensityJit 声明（Op 序列 → asmjit 机器码函数指针）
├── DensityJitCompiler.cpp             — JIT 编译器实现（Win x64 门控，逐条 Op 翻译为虚拟寄存器指令）
├── DensityJitTrampolines.hpp          — JIT 外部调用 trampoline 自由函数声明
├── DensityJitTrampolines.cpp          — 9 个 trampoline 实现（噪声采样/Delegate/Spline/FindTopSurface/Marker 判空等）
└── README.md                          — 本文件
```

## 内部模块关系

```
维度级编译链（RandomState 触发，编译一次）：
DensityFunction 树
  └─McToAst::convert──→ AstNode 树
  └─OptoPasses::optimize──→ 优化后 AstNode 树
  └─BytecodeGen::compile──→ CompiledDensityFunction（持 m_ops 指令序列）
                              └─compileJit()──→ compileDensityJit ──→ asmjit 机器码 m_jitFn

区块级复用（NoiseChunk 触发）：
CompiledDensityFunction::newInstance
  └─深拷贝 m_ops（字节相同）──→ 复用维度级 m_jitFn
  └─重建 DensityEvalContext（指向自己的 objects/subEvaluators/splines）

求值路径：
CompiledDensityFunction::eval
  ├─m_jitFn != null ──→ JIT 机器码（内部算术内联，外部调用经 trampoline）
  └─m_jitFn == null ──→ evalImpl switch 解释器（兜底）

JIT ↔ 解释器共享：DensityEvalHelpers 的纯函数被 evalImpl 与 JIT trampoline 同时调用
Adapter 桥接：CompiledDensityFunctionAdapter 把编译产物包装为 DensityFunction，供 NoiseInterpolator/CellCache/CacheOnce/FlatCache/Cache2D 的 filler 持有
```

## 外部依赖关系

### 依赖

- `common/world/gen/density/DensityFunction.hpp` — 密度函数接口（Adapter 继承、BytecodeGen/McToAst 转换源）
- `common/world/gen/density/DensityFunctions.hpp` — MarkerType/SplinePoint/CubicSpline 等类型
- `common/world/gen/density/Beardifier.hpp` — 结构物地形修饰器（trampoline 调用）
- `common/world/gen/density/NoiseChunk.hpp` — 区块噪声采样单元（CompiledDensityFunction 持有 NoiseChunk 几何）
- `common/world/gen/noise/NormalNoise.hpp` — 噪声采样（AstNodes 持 `const NormalNoise*`，trampoline 调 getValue）
- `common/core/Types.hpp`、`common/util/assert/AssertAll.hpp`

### 被依赖

- `common/world/gen/RandomState` — 维度级编译入口（include BytecodeGen/McToAst/OptoPasses/CompiledDensityFunction）
- `common/world/gen/density/NoiseChunk` — 区块级 newInstance + Adapter 包装（include CompiledDensityFunction/CompiledDensityFunctionAdapter）
- 测试：OptoPassesTest、DensityAstCompileTest、DensityJitBaselineTest

对外暴露 7 个头：`CompiledDensityFunction.hpp`、`CompiledDensityFunctionAdapter.hpp`、`BytecodeGen.hpp`、`McToAst.hpp`、`OptoPasses.hpp`、`AstNode.hpp`、`AstNodes.hpp`。`DensityEvalHelpers/DensityEvalProfiler/DensityJitCompiler/DensityJitTrampolines` 为内部实现，不应被 ast/ 外部直接 include。

## 容易踩的坑

1. **asmjit 常量池 int64 仅 8 字节对齐——packed 指令配内存操作数会 #GP**：Abs 清符号位用 `andps dst, [const]` 会崩。ANDPS/ANDPD/XORPS/ORPS/PAND 等 SSE packed 指令的 m128 内存操作数要求 16 字节对齐，asmjit 常量池 int64 条目仅 8 字节对齐触发 #GP（非 #PF，故 VEH 报 fault_addr 为垃圾值 -1，与"读入有效内存"表面矛盾）。修复：掩码先 `movsd`（标量，8 字节对齐足矣）加载到 Xmm，再寄存器-寄存器 `andps dst, mask`。所有 128 位 packed 指令同理。
2. **asmjit vcpkg 是 camelCase 旧 API**：`newXmmSd/newIntPtr/newDoubleConst(ConstPoolScope::kLocal,v)/invoke(&call,imm(fnPtr),sig)/InvokeNode::setArg` 等。`asmjit::Error`（uint32_t typedef）与 `mc::Error`（Result.hpp 类）冲突，**绝不能 `using namespace asmjit`**，全部 `asmjit::` 显式限定。
3. **f64 Imm 参数触发 asmjit error 25 (kErrorInvalidAssignment)**：`moveImmToRegArg` 仅处理整数 TypeIds，f64 常量传给 f64 形参报错 25。修复：f64 常量经 `newDoubleConst`+`movsd` 加载到 Xmm 再传 Xmm（YGradient 的 4 个 f64 常量如此处理）。
4. **`DensityEvalContext` 与 `DensityJitFn` 定义在 CompiledDensityFunction.hpp**（非 DensityJitCompiler.hpp），以打破循环 include。DensityJitCompiler.hpp/DensityJitTrampolines.hpp 仅前向声明 + include CompiledDensityFunction.hpp 可见这两个类型。DensityJitCompiler.hpp 刻意不 include asmjit（封装在 .cpp）。
5. **维度级与区块级共享同一 JIT 代码**：newInstance 深拷贝 m_ops 字节相同故复用 m_jitFn，只重建 DensityEvalContext。MARKER 翻译时生成运行时判空（`cacheObj != null` 走 compute / 否则走 delegate eval），维度级（占位）与区块级（注入缓存）两种路径都覆盖。
6. **浮点累加顺序必须 bit-exact**：逐条 Op 顺序翻译 + asmjit 不重排保证浮点累加顺序不变 → JIT 与解释器 1e-9 一致。两操作数指令须先 `movsd(dst,a)` 再 `*sd(dst,b)`（x64 两操作数破坏 dst）。macOS ARM64 JIT 留 TODO，须注意避免 fmadd 融合破坏 bit-exact。
7. **DensityEvalProfiler.hpp 是临时插桩**：双桶差值法（interpreterCycles=topLevelCycles−externalCycles）量化 JIT 收益用，profiler 完成后本文件及 eval 内插桩代码将整体删除。Marker 是"递归外部调用"（B 类）不可计时，误归 A 类（叶子）计时致双重计数（externalCycles>totalCycles 悖论）。
8. **性能计数器在 JIT 路径下 externalCycles 失真**：trampoline 未插桩，JIT 路径下 externalCycles/externalCalls 不累计，interpreterRatio 失真——以 topLevelCycles 绝对下降为 JIT 收益主指标。
