# 差分能力：`anvil_durability`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 81 个能力，16 种耐久组合全部一致。与上一轮的 `repair_cost` 相邻，覆盖铁砧合并同物品时
的耐久叠加。

#### 原版代码

```java
int l  = itemstack.getMaxDamage()  - itemstack.getDamageValue();
int i1 = itemstack2.getMaxDamage() - itemstack2.getDamageValue();
int j1 = i1 + itemstack1.getMaxDamage() * 12 / 100;
int k1 = l + j1;
int l1 = itemstack1.getMaxDamage() - k1;
if (l1 < 0) { l1 = 0; }
if (l1 < itemstack1.getDamageValue()) {
    itemstack1.setDamageValue(l1);
    i += 2;
}
```

#### 三个易错点

**1. `* 12 / 100` 必须先乘后除。** 对钻石剑（1561）：正确得 187，先除后乘只得 180。

**2. 「有改善」才计入。** `l1 < itemstack1.getDamageValue()` 为假时，损伤值不变、代价也
不 +2。实测主物品满耐久（`1561|0`）得到 `repaired=false, costAdded=0`。

**3. 新损伤值钳到 0**，不能取负。

#### 本轮最大的教训：用例不够敏感会给出虚假的通过

我最初选的顺序敏感用例是 `99|50|99|50`。它**确实**能区分「先乘后除（11）」与
「先除后乘（0）」——但两侧都半损，总耐久远超上限，`newDamage` 一律被钳到 **0**。
于是「算错」和「算对」得到同样的输出，这条用例**完全没有起到验证作用**。

发现问题后我换成 `1561|1500|1561|1400`（主物品剩 61、副物品剩 161）：

| 算法 | 加成 | `newDamage` |
|---|---|---|
| 先乘后除（正确） | 187 | **1152** |
| 先除后乘（错误） | 180 | 1159 |

两者都不到 0，**差异才显现出来**。实测输出 `1152`，确认了实现用的是正确顺序。

这与之前 `explosion_damage` 那轮的教训是同一条：**测试输入必须避开被钳制/被短路吞掉
差异的区间**，否则能力会给出虚假的通过。区别在于那次是「射程内输入测不出钳制差异」，
这次是「钳到 0 吞掉了顺序差异」。

#### 溢出边界

`maxDamage * 12` 是 `int` 运算，理论上在 `maxDamage > 178956970` 时溢出。用例覆盖了
`178956970`（界内）与 `178956971`（`2147483652`，仍小于 `INT_MAX`）两点，两侧一致。
