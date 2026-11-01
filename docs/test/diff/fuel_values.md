# 差分能力：`fuel_values`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、验证结论与恢复经过。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 62 个能力（原编号），1505 个物品的燃料时长全部一致。

## 本轮的特殊之处：恢复了两个「已实现但未注册」的能力

本能力与 [`recipe_book_category`](./recipe_book_category.md) 在仓库里**早已实现且被 git 跟踪**
（探针、oracle、harness 三者齐全，`index.ts` 里也有注册条目），但**三张 CMake 列表里都没有
它们**。

后果：

- 探针源码从未被编译
- 能力从不参与任何测试运行
- 状态表里也没有它们的行

这属于**「实现完整但接线缺失」**的半成品状态——比完全没实现更危险，因为从代码看像是「已完成」。

本轮把两者补进三张 CMake 列表并验证通过：

| 能力 | 结果 | 耗时 |
|---|---|---|
| `fuel_values` | **1505/1505 一致** | 77.9 秒 |
| `recipe_book_category` | **1470/1470 一致** | 18.2 秒 |

## 能力说明

单元是物品 ID，两侧都只输出 `burnTime`（燃烧时长，单位 tick）。

**比的是「玩家实际感受到的那个数」**：原版用标签聚合表达燃料表（`ItemTags.LOGS` 一条覆盖全部
原木，再减去不可燃木材），Cubium 侧无论是标签还是逐条判断，只要最终时长对上就算一致。

实测样例：

```text
minecraft:acacia_boat       -> burnTime=1200
minecraft:acacia_button     -> burnTime=100
minecraft:acacia_chest_boat -> burnTime=1200
```

`burnTime = 0` 表示「不是燃料」——这类物品占绝大多数，**两侧都必须显式给出 0 而不是省略字段**。

## `normalise` 的作用

```typescript
export function normaliseFuelValues(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") { return actual; }
    const record = actual as Record<string, unknown>;
    return { ok: record["ok"] ?? null, burnTime: record["burnTime"] ?? null };
}
```

失败侧（物品未注册）也要补齐 `burnTime`，否则「原版是燃料、本侧根本没注册这个物品」会因为
**缺键被静默通过**——这是一个容易漏掉的假绿。

## `exclusiveGroup: "loot-world"`

```typescript
exclusiveGroup: "loot-world",
// oracle 要在 java.io.tmpdir 里建原版服务端世界并持有区域锁：同组必须串行。
```

本能力的 oracle 需要**构造一个完整的原版服务端世界**（为了拿到配方/燃料注册表），并在
`java.io.tmpdir` 下持有区域文件锁。因此：

- 同一组（`loot-world`）的能力必须**串行**执行
- 与本仓库之外的其他进程同时运行会冲突，报
  `IOException: The process cannot access the file because another process has locked a portion of the file`

**本轮第一次运行就是这样失败的**——合作开发者在 `E:\dev\Cubium\branch-1` 运行同类能力时持有
了同名锁。等待 20 秒重试即通过。

这也解释了 77.9 秒的耗时：建世界本身就很重。

## 恢复过程中遇到的问题：三张列表的顺序

补注册时我把 `fuel_values` 与 `recipe_book_category` 在源列表里的顺序与名称列表写反了，
**配置阶段的断言立刻报错**：

```text
tests/diff: 能力清单第 119 项顺序错位——后缀 'fuel_values'
```

这正是 `_patterns.md` 第十条所要求的保护。修正后配置通过。

## 未覆盖范围

- 燃料的**标签聚合**结构本身（只比对最终时长）
- 熔炉、烟熏炉、高炉各自的燃烧倍率（那是方块实体逻辑，见 `furnace_tick`）
- `burnTime` 在实际熔炼中的消耗与产出