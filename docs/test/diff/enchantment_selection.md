# 差分能力：`enchantment_selection`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 79 个能力，14 物品 × 12 等级 = 168 个单元。105 个一致，63 个记为已知差异。

附魔领域此前已有 7 个能力全部覆盖**定义层**（代价曲线、适用性、标签、效果、宝藏枚举）。
本能力补上**选择逻辑**：给定物品与等级，最终挑出哪些附魔、各为几级。

#### 原版逻辑

```java
stream.filter(e -> e.value().isPrimaryItem(stack) || flag)   // flag = 是不是书
      .forEach(e -> {
          for (int i = e.value().getMaxLevel(); i >= e.value().getMinLevel(); i--) {
              if (level >= e.value().getMinCost(i) && level <= e.value().getMaxCost(i)) {
                  list.add(new EnchantmentInstance(e, i));
                  break;      // 每个附魔最多一项
              }
          }
      });
```

#### 三类差异，同一根因

| 差异 | 表现 | 根因 |
|---|---|---|
| 缺宝藏附魔 | `mending` / `vanishing_curse` / `binding_curse` / `frost_walker` / `soul_speed` 从不出现 | `canApplyAtEnchantingTable` 里硬编码了 `if (isTreasure()) return false;` |
| 多 `thorns` | 盔甲类多处多出荆棘 | 荆棘的主物品判定被放宽 |
| 多 `sharpness` 等 | `diamond_axe` 能出锋利 / 亡灵杀手 / 节肢杀手 | 斧被归入 `EnchantmentType::Weapon` 粗分类 |

共同根因：**Cubium 用粗粒度类型枚举代替了原版的物品级主物品表**，并把原版由**调用方**
负责的宝藏过滤内移到了被调方。

#### 「闸门前移」是第三次出现

原版 `getAvailableEnchantmentResults` 不判断宝藏——是否包含宝藏附魔由**传进来的
`Stream<Holder<Enchantment>>`** 决定。Cubium 把这道闸门写进了
`canApplyAtEnchantingTable`，于是任何调用方都拿不到宝藏附魔。

这与前几轮的 `explosion_damage`（把调用方的 `inRange` 闸门前移进伤害公式）、
`food_stats`（`add` 缺下界）属于同一类问题：**把「调用方的职责」内移到「被调方的
公式/判定」里**。这类错误的共同特征是：

- 单独看被调方，逻辑「看起来更自洽」；
- 但对**所有**调用方都生效，而原版只对**部分**路径生效；
- 差分测试是唯一能发现它的手段——因为没有哪个单点测试会同时覆盖两条调用路径。

#### 顺序差异的处理

原版返回顺序由附魔注册表决定，Cubium 的遍历顺序不同。顺序本身不是本能力要验证的口径
（它属于注册表加载顺序，已有能力覆盖），因此探针与 oracle **两侧都按附魔 id 排序**后输出，
让差异集中在「哪些附魔、各几级」上。
