# 差分能力：`trial_spawner_config`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 70 个能力，28 项 × 9 种玩家数 = 252 个单元，全部一致。它一上来就抓到了两处问题。

#### 差异一：四个数值字段被声明成整数

Cubium 的 `TrialSpawnerBlockEntity::Config`：

```cpp
i32 totalMobs = 6;
i32 totalMobsAddedPerPlayer = 2;
i32 baseSimultaneousMobs = 3;
i32 simultaneousMobsAddedPerPlayer = 1;   // <- 数据包里全是 0.5
```

而原版是 `f32`。数据包 28 项里 `simultaneous_mobs_added_per_player` **无一例外都是
`0.5`**，整数字段根本装不下。

这个偏差会直接改变刷怪数量，因为原版的派生方法是：

```java
calculateTargetSimultaneousMobs(n) = (int) Math.floor(simultaneousMobs + simultaneousMobsAddedPerPlayer * n)
```

乘数 0.5、玩家数 3 时：`1 + 0.5 * 3 = 2.5` → floor 得 **2**。若按整数加法先算成
`1 + 0 = 1`，就少一只。能力输出的实测值：

```json
{"config":"minecraft:trial_chamber/breeze/normal","playerCount":3,
 "simultaneousMobs":"1","simultaneousMobsAddedPerPlayer":"0.5","targetSimultaneousMobs":2}
```

本能力因此在 9 种玩家数下各算一遍，专门压这条边界。

#### 差异二：缺省字段的默认值不是空

探针第一次跑就报了：

```
oracle: {"lootTableCount":2}
probe : {"lootTableCount":0}
```

原版 `loot_tables_to_eject` 的缺省值来自 `DEFAULT.lootTablesToEject`，而那是个**含 2 项的
WeightedList**（`spawner_trial_chamber_consumables` 与 `spawner_trial_chamber_key`），
不是空列表。14 个配置没写这个字段，全都要回退到这两项。

这条比「字段缺失就按空处理」更容易错——空列表看起来是最自然的中性默认值，但原版不是。

#### 一条被验证的假设

`spawn_range` 在全部 28 项里都没有出现，必须回退到 `Builder` 的默认值 4。这条假设成立，
252 行里 `spawnRange` 全部为 4。

TODO 与 `banner_pattern` / `enchantment_provider` 一致：待 Cubium 改为数据包驱动后，
探针应改为读注册表。
