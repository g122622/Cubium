# 差分能力：`legacy_random`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 190 个能力。12 个单元，**全部一致**。

这是本会话第一个**纯序列比对**能力：不比对单个函数返回值，而是比对一整串随机数，
任何一处实现偏差都会让后续序列完全分叉。

## 测的是什么

原版 `LegacyRandomSource` 就是 Java 的 48 位 LCG：

```java
@Override
public int next(int bits) {
    long i = this.seed.get();
    long j = i * 25214903917L + 11L & 281474976710655L;
    this.seed.set(j);
    return (int)(j >> 48 - bits);
}

@Override
public void setSeed(long seed) {
    this.seed.set((seed ^ 25214903917L) & 281474976710655L);
}
```

Cubium 的 `mc::math::JavaLegacyRandom` 与之常量完全相同（`MULTIPLIER = 25214903917ULL`、
`INCREMENT = 11ULL`、`MASK = (1ULL << 48) - 1`）。

## 三个关键点

### 一、`setSeed` 先异或再掩码

```java
(seed ^ 25214903917L) & 281474976710655L
```

**少了这步异或，序列会整体错位**——但后续每一拍仍然"看起来是随机的"，很难用肉眼发现。

### 二、`nextInt(bound)` 是拒绝采样，不是取模

```java
do {
    bits = next(31);
    val = bits % bound;
} while (bits - val + (bound - 1) < 0);
```

判定式 `bits - val + (bound - 1) < 0` **依赖 i32 回绕**：当 `bits - val` 接近 `Integer.MAX_VALUE`
时加上 `bound - 1` 会溢出成负数，于是重抽。

简单写 `nextInt() % bound` 会引入模偏置，且**推进的步数不同**，序列完全分叉。

Cubium 对此有专门的注释：

```cpp
// Java 的 `bits - val + (bound - 1) < 0` 依赖 i32 回绕判定溢出，
// 而 C++ 的有符号溢出是 UB，故用 u32 显式回绕后按有符号解释，逐位复刻 Java 语义。
rejected = static_cast<u32>(bits) - static_cast<u32>(val) + static_cast<u32>(bound - 1);
```

### 三、2 的幂走快速路径，只抽一次

```java
if ((bound & (bound - 1)) == 0) {
    return (int)((bound * (long) next(31)) >> 31);
}
```

因此**同一 seed 下，取不同 bound 会推进不同的步数**：

| 序列 | bound | 抽取次数 |
|---|---|---|
| `i:8;i:8;i:8` | 8（2³）| 每次 1 次 |
| `i:3;i:5;i:7` | 非 2 的幂 | 可能 2 次以上 |

本能力的 `42\|i:8;i:8;i:8;i:16;i:32;i:64` 与 `42\|i:3;i:5;i:7;i:9;i:11` 两个单元
专门覆盖这两条路径的差异。

## 单元设计

| 单元 | 覆盖点 |
|---|---|
| `0\|i:10;…` / `1\|…` | 最小种子的基础序列 |
| `12345\|i:100;…` | 常规种子 |
| `42\|i:8;i:8;i:8;i:16;i:32;i:64` | **2 的幂快速路径** |
| `42\|i:3;i:5;i:7;i:9;i:11` | **非 2 的幂拒绝采样** |
| `7\|i:1;i:1;i:1` | `bound = 1` 退化情形 |
| `99\|i:1000000;i:2147483647;i:1000000007` | 大 bound（接近 i32 上限）|
| `2024\|I;I;l;l;b;b;f;f;d;d` | 全部输出类型混合 |
| `555\|c:10;i:100` vs `555\|c:1;i:100` | `consumeCount` 的步进量 |
| `-12345\|i:100;…` | **负种子** |
| `9223372036854775807\|i:1000;…` | **i64 最大值种子** |

同一 seed `42` 用两组不同 bound 的单元是刻意设计：它们在第 4 位之后必然不同，
若实现正确则各自都应与原版一致；若 `nextInt` 的实现有偏差，两组会以不同方式分叉。

## 踩坑：`%.17g` 的末位展开分歧

首轮只剩 1 处差异：

```
原版   0.4940935761119589
Cubium 0.49409357611195892
```

用 `Double.toString` 验证后确认**是同一个 double**：

```
A 的 R: 0.4940935761119589
B 的 R: 0.4940935761119589
相等: True
```

差异来自 `%.17g`：C++ 展开到 17 位，Java 到 16 位后补零。

改用 **16 位有效数字**后全部一致。这是本会话第三次踩到同一类坑
（前两次见 [`easing_type`](./easing_type.md) 与 [`world_border`](./world_border.md)），
已在 [`_patterns.md`](./_patterns.md) 中记录为通用规律。

## 未覆盖范围

- **`nextGaussian()`**：走 `GaussianSource` 的缓存机制（Box-Muller + 惰性缓存），
  属于有状态序列，需要单独设计单元。
- **`forkPositional()` / `LegacyPositionalRandomFactory`**：位置哈希派生，
  已由 [`mth_seed`](./mth_seed.md) 覆盖种子部分，未覆盖派生后的序列。
- **`Xoroshiro128ppRandom` / `Xoshiro256ppRandom`**：Cubium 有实现，未在本能力覆盖。
  `TODO: 新增 xoroshiro_random 能力覆盖这两套现代随机源。`
- **`WorldgenRandom`**：覆写了 `nextLong()`（两次 `next(32)` 拼接），语义与基类不同。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_seed`](./mth_seed.md) | `Mth.getSeed` 位置哈希 |
| [`random_sequence`](./random_sequence.md) | `RandomSequence` 命名随机源 |
| [`random_tick_pos`](./random_tick_pos.md) | 随机刻位置选择 |