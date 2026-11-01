# 差分能力：`spawner_delay`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 90 个能力，15 种延迟状态组合全部一致。刷怪笼的生成延迟此前没有被比对过。

#### 原版代码

```java
private void delay(Level level, BlockPos pos) {
    if (this.maxSpawnDelay <= this.minSpawnDelay) {
        this.spawnDelay = this.minSpawnDelay;                       // 区间退化
    } else {
        this.spawnDelay = this.minSpawnDelay
            + level.random.nextInt(this.maxSpawnDelay - this.minSpawnDelay);
    }
}

// serverTick
if (isNearPlayer(...) && level.isSpawnerBlockEnabled()) {
    if (this.spawnDelay == -1) { this.delay(level, pos); }
    if (this.spawnDelay > 0) { this.spawnDelay--; }
    else { ... 尝试生成 ... }
}
```

#### 三个关键点

**1. `-1` 是「需要重置」的哨兵值**，不是「还剩 -1 tick」。`spawnDelay == -1` 时先调
`delay()` 取值，**然后继续走正常流程**——同一个 tick 内「先重置、再减 1」。实测
`tick|-1|true|true|200|800|0` 得到 `reset=true` 且 `delay=199`（= 200 - 1）。

**2. 区间退化时直接取 `minSpawnDelay`**，条件是 `maxSpawnDelay <= minSpawnDelay`
（**包含相等**）。实测 `200|200` 得 200、`300|200` 也得 300，都不调用随机数。
若写成 `<`，两者相等时会去调 `nextInt(0)`。

**3. `nextInt` 的上界是 `max - min`**，即区间左闭右开 `[min, max)`。默认 200/800 时
`roll=599` 得到 **799**——**永远取不到 800**。

#### 前置条件是「整 tick 都不动」

`isNearPlayer(...) && isSpawnerBlockEnabled()` 不满足时，**既不重置也不递减**：

| 单元 | `delay` | `reset` | `decremented` |
|---|---|---|---|
| `tick\|-1\|false\|...` | **-1** | false | false |
| `tick\|20\|true\|false\|...` | **20** | false | false |

容易写成「玩家走远了仍然倒计时」或「−1 仍然会被重置」——两者都与原版不符。

#### 一个 tick 内的动作次序

| 单元 | `delay` | `decremented` | `shouldSpawn` |
|---|---|---|---|
| `tick\|1\|true\|...` | 0 | **true** | **false** |
| `tick\|0\|true\|...` | 0 | false | **true** |

减到 0 的那个 tick **不生成**，要等到下一个 tick 才生成。这与 `hopper_cooldown` 那轮的
边界一致：递减在前、判定在后。

#### 验证方法

延续前三轮的做法，跑完逐条打印实际输出核对分支，15 条全部命中。

#### 未覆盖范围

实际生成（`spawnCount` 次尝试、`spawnRange` 内的位置随机、附近实体数量上限），需要世界
与实体注册表。
