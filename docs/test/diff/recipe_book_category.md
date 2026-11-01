# 差分能力：`recipe_book_category`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、验证结论与恢复经过。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 63 个能力（原编号），1470 个配方的配方书分类全部一致。

## 本轮的特殊之处

本能力与 [`fuel_values`](./fuel_values.md) 一样，属于**「实现完整但接线缺失」**——探针、oracle、
harness 三者齐全，`index.ts` 也有注册条目，但**三张 CMake 列表里没有它**，因此从未被编译、
从未参与测试。本轮补注册后验证通过：**1470/1470 一致**，耗时 18.2 秒。

## 能力说明

本能力验证配方在**配方书界面中的分类页**归属。

单元是配方 ID，输出三元组：

```json
{"ok":true,"page":"minecraft:crafting_misc","recipe":"minecraft:acacia_boat","type":"minecraft:crafting_shaped"}
{"ok":true,"page":"minecraft:crafting_redstone","recipe":"minecraft:acacia_button","type":"minecraft:crafting_shapeless"}
```

| 字段 | 含义 |
|---|---|
| `recipe` | 配方 ID |
| `type` | 配方类型（`crafting_shaped` / `crafting_shapeless` 等）|
| `page` | 配方书里的分类页（`crafting_misc`、`crafting_redstone` 等）|

**分类页不是从配方内容推导的**，而是由配方自己的 `category` 字段声明。1.21.11 把配方书分类
重构成了独立的 `RecipeBookCategory` 注册表，配方的 `category` 指向其中的条目。

## 为什么这个能力有价值

配方书分类看起来是纯展示逻辑，但它在 Cubium 里涉及**跨模块重构**：

- 配方数据的解析（`category` 字段）
- `RecipeBookCategory` 注册表的建模
- 客户端配方书的分页逻辑

**分类错了玩家就会发现**：附魔书会出现在错误的分页里，红石配方跑到杂项页。这类「不影响游戏
逻辑但明显可见」的偏差，正是差分测试擅长捕获的。

## 与 `recipe` / `single_item_recipe` 的分工

| 能力 | 验证内容 |
|---|---|
| `recipe` | 配方的**结构**（原料、产出、图案）|
| `single_item_recipe` | 单物品配方（熔炼、切石等）|
| **`recipe_book_category`** | 配方在**配方书中的分页** |

三者互补，不重叠。

## 未覆盖范围

- 配方书在客户端的**排序**（同一页内的顺序）
- `RecipeBookCategory` 注册表本身的枚举完整性（只通过配方的引用间接验证）
- 由「配方书」触发的**自动补全**行为（那是客户端合成逻辑）