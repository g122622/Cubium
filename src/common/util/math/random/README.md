# 随机数

项目统一的随机源抽象与各算法实现。**全项目禁止直接使用 `std::mt19937` 等标准库随机引擎**，
一律经由本目录的接口。

## 目录结构树

```text
random/
├── IRandom.hpp/cpp                 # 随机源抽象接口（nextInt/nextFloat/nextLong 等）
├── Random.hpp                      # 对外别名 `Random`，按平台选择 Xoroshiro128++ 或 Xoshiro256++
├── JavaLegacyRandom.hpp/cpp        # Java 版 LinearCongruentialGenerator 语义（原版刷怪/结构用）
├── LcgRandom.hpp/cpp               # 简化 LCG
├── LinearCongruentialGenerator.hpp # LCG 核心常数与状态推进
├── Mt19937Random.hpp/cpp           # MT19937 适配（仅用于需要 MT 语义的历史路径）
├── Xoroshiro128ppRandom.hpp/cpp    # Xoroshiro128++
├── Xoshiro256ppRandom.hpp/cpp      # Xoshiro256++
├── WorldgenRandom.hpp/cpp          # 世界生成专用：把所有位宽抽取折算成反复取内层 nextLong
├── PositionalRandomFactory.hpp/cpp # 由 (seed, 坐标) 派生确定性子随机源
├── BlockPositionRandom.hpp         # 以方块坐标派生的随机源
├── RandomRanges.hpp/cpp            # IRandomRange / RandomValueRange 等区间取值
└── UniformIntDistribution.hpp/cpp  # 整数均匀分布
    UniformRealDistribution.hpp/cpp # 实数均匀分布
```

## 内部模块关系

`IRandom` 是唯一的对外接口，全部引擎实现它。`Random.hpp` 只做一行别名，
让调用方写 `math::Random` 而不必关心具体算法。`WorldgenRandom` 是**装饰器**：
它持有一个内层 `IRandom`，把 `nextInt` / `nextFloat` / `nextDouble` 等高位宽请求
拆成对 `nextLong()` 的重复调用来消耗随机流，以复现原版的取值顺序。
`PositionalRandomFactory` 与 `BlockPositionRandom` 由种子 + 坐标派生，用于
"同一位置每次都得到同一结果"的场景。两个 `*Distribution` 是纯工具，不持有随机源。

## 上下游外部依赖关系

上游只依赖 `common/core/Types.hpp` 的定宽整数与 `MathUtils` 的取整函数。
下游覆盖世界生成（密度函数、特征放置、装饰种子）、实体 AI、战利品表、
刷怪、附魔等几乎所有需要随机性的子系统。

## 容易踩的坑

- **世界生成的装饰种子必须走 `WorldgenRandom`，不能用裸 `Xoroshiro128ppRandom`。**
  原版把位宽抽取折算成"反复调用内层 `nextLong()`"，直接换引擎会改变随机流位置，
  导致生成结果与原版不一致。
- **`nextInt(bound)` 的上界是排他的**（`[0, bound)`），需要闭区间时用
  `nextIntBetweenInclusive(origin, bound)`。后者内部是
  `nextInt(bound - origin + 1) + origin`，**那个 `+1` 不可省**——
  漏掉会让上界永远取不到，且不会报错。
- `Random.hpp` 用 `MC_RANDOM_XOROSHIRO128PP` / `MC_RANDOM_XOSHIRO256PP` /
  `MC_RANDOM_LCG` 宏分支选择 `Random` 别名，默认走 Xoroshiro128++。
  读别名定义时不要只看第一个分支。
- **`UniformIntDistribution` 的构造函数带有默认参数**
  （`min = 0, max = limits<i32>::max()`），违反项目"函数参数禁止默认值"的规范。
  调用时请显式传参；若后续清理该默认值，请按"让编译器当解析器"的流程处理
  （去掉默认值 → 编译 → 从 `requested here` 取精确行号 → 按行号补参）。
- 需要"可复现的种子流"时用 `PositionalRandomFactory`，不要自己拼接坐标与种子——
  拼接方式一旦与原版不同，同一世界的生成结果就对不上。