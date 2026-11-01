# 差分能力：`scheduled_tick`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 111 个能力，29 个排序/去重用例全部一致。方块 tick 调度器本身此前未被覆盖
（`furnace_tick`、`brewing_tick` 等只覆盖了具体方块的行为）。

## 三个比较键

```java
public record ScheduledTick<T>(T type, BlockPos pos, long triggerTick,
                               TickPriority priority, long subTickOrder) {

    public static final Comparator<ScheduledTick<?>> DRAIN_ORDER = (a, b) -> {
        int i = Long.compare(a.triggerTick, b.triggerTick);
        if (i != 0) { return i; }
        i = a.priority.compareTo(b.priority);
        return i != 0 ? i : Long.compare(a.subTickOrder, b.subTickOrder);
    };

    public static final Comparator<ScheduledTick<?>> INTRA_TICK_DRAIN_ORDER = (a, b) -> {
        int i = a.priority.compareTo(b.priority);
        return i != 0 ? i : Long.compare(a.subTickOrder, b.subTickOrder);
    };
}
```

| 比较器 | 主键顺序 |
|---|---|
| `DRAIN_ORDER` | `triggerTick` → `priority` → `subTickOrder` |
| `INTRA_TICK_DRAIN_ORDER` | `priority` → `subTickOrder`（**完全忽略 `triggerTick`**） |

`INTRA_TICK_DRAIN_ORDER` 少一次比较是因为它只用于「同一 tick 内」的容器排序，此时
`triggerTick` 必然相同。

## 四个关键点

**1. `TickPriority` 的枚举声明顺序就是排序顺序。**

```java
EXTREMELY_HIGH(-3), VERY_HIGH(-2), HIGH(-1), NORMAL(0), LOW(1), VERY_LOW(2), EXTREMELY_LOW(3);
```

声明值恰好从 `-3` 递增到 `3`，与「数值越小优先级越高」一致。实测五组两两比较全部正确：

| 单元 | 结果 |
|---|---|
| `drain(VERY_HIGH, HIGH)` | -1 |
| `drain(HIGH, NORMAL)` | -1 |
| `drain(NORMAL, LOW)` | -1 |
| `drain(LOW, VERY_LOW)` | -1 |
| `drain(VERY_LOW, EXTREMELY_LOW)` | -1 |

若按「名字里 HIGH 应该排在后面」的直觉实现会全盘反转。

**2. 去重只认 `type` 与 `pos`，完全忽略时间与优先级。**

```java
public boolean equals(ScheduledTick<?> a, ScheduledTick<?> b) {
    return a.type() == b.type() && a.pos().equals(b.pos());
}
```

| 单元 | type | pos | 结果 |
|---|---|---|---|
| `unique\|fnv\|1\|fnv\|1` | 同 | 同 | **same** |
| `unique\|fnv\|1\|fnv\|2` | 同 | 异 | **diff** |
| `unique\|fnv\|1\|fluid\|1` | **异** | **同** | **diff** |
| `unique\|block\|9\|block\|9` | 同 | 同 | same |

**`unique|fnv|1|fluid|1` 是最关键的一条**：位置相同但类型不同，**不会被去重**。这保证了
「同一方块上的方块 tick 与流体 tick 可以同时存在」。

同时意味着：**同一位置同一类型的两次调度永远视为重复，即使触发时间不同**。这是「同一方块
同一 tick 类型只会被执行一次」的保证。

**3. `type` 用 `==` 比较是引用比较，不是内容比较。**

`a.type() == b.type()` 对对象是引用比较。所以「两个内容相等但不同实例的 type」**不被视为
重复**。实际使用中 `type` 是注册表里的单例对象，所以引用比较等价于身份比较。

**这一点在 oracle 侧踩过坑**：最初用 `"type" + parts[2]` 拼接字符串，两次拼接得到不同
String 实例，`==` 为 false，导致所有 `unique` 用例都返回 `diff`。正解是用 `intern()`
驻留，让相同内容共享引用。

**4. `subTickOrder` 是最后一级 tiebreaker。**

`drain(10/NORMAL/3, 10/NORMAL/5)` 返回 -1，`drain(10/NORMAL/5, 10/NORMAL/3)` 返回 1。
它保证同一 tick、同一优先级下的调度项有确定的顺序，避免依赖插入顺序。

## 完整排序实测

```
sort|10/NORMAL/0, 5/HIGH/2, 5/EXTREMELY_HIGH/5, 5/NORMAL/1
  → 5/EXTREMELY_HIGH/5, 5/HIGH/2, 5/NORMAL/1, 10/NORMAL/0
```

同一 `triggerTick = 5` 的三项按优先级 `EXTREMELY_HIGH < HIGH < NORMAL` 排列；`triggerTick`
更大的 `10/NORMAL/0` 排最后。

## 本轮踩坑记录：两次破坏性操作

1. **把 `unique` 用例的参数语义设计错了**——原设计 `unique|t1|pos1|sy1|t2|pos2|sy2` 中
   type 由 pos 推导，两者永远同步，无法构造「type 相同、pos 不同」的情况。改为
   `unique|type1|pos1|type2|pos2` 让两者独立。

2. **行索引数组手术把两个文件改坏了**——`ScheduledTickOracle.java` 被截断到 78 行，
   `scheduled_tick.cpp` 的 `unique` 分支被整体删除。这是本轮第二次因为复杂行索引操作
   破坏文件结构（见 [`_patterns.md`](./_patterns.md) 的教训）。

   **修正做法**：确认破坏程度后**整个文件重写**，而不是继续在残缺结构上打补丁。

3. **`sort` 单元只有 2 段，但入口检查要求至少 4 段**——调整检查顺序到 2 段。

## 未覆盖范围

- `LevelTicks` 的容量上限与 `LevelChunkTicks` 的容器结构
- `SavedTick` 的相对时间换算（`triggerTick - now`）
- `LevelChunkTicks.schedule` 的实际写入与替换语义
- `subTickOrder` 的生成（`LevelTicks` 内部的递增计数器）