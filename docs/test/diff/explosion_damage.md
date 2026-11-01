# 差分能力：`explosion_damage`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 73 个能力，6 半径 × 8 距离 × 5 可见度 × 2 击退抗性 = 480 个单元，全部一致。
它一建立就暴露出 Cubium 与原生版的**两处口径差异**。

#### 差异一：距离比例被钳制

原版 `ExplosionDamageCalculator.getEntityDamageAmount`：

```java
float f = explosion.radius() * 2.0F;
double d0 = Math.sqrt(entity.distanceToSqr(center)) / f;   // 裸比值，不钳制
double d1 = (1.0 - d0) * seenPercent;
return (float) ((d1 * d1 + d1) / 2.0 * 7.0 * f + 1.0);
```

Cubium 写的是 `std::min(1.0f, std::sqrt(distSq) / damageRadius)`。

「跳过超距实体」这件事在**调用方**完成（`ServerExplosion` 里的 `if (!(d0 > 1.0))`），
不在公式里。Cubium 把这道闸门前移进了公式，后果是超距实体不再得到「越过 0 之后
重新上升」的伤害：

| 输入 | 原版 | 若按钳制 |
|---|---|---|
| radius 1、distance 16、seen 1 | `ratio=8`、**damage=295** | `ratio=1` → `d1=0` → **damage=1** |

相差 295 倍。这只是公式层的复刻差异——是否真的打到这个伤害由调用方的 `inRange`
决定，而本能力把 `inRange` 单独输出，正是为了让「公式」与「闸门」的分工可验证。

#### 差异二：返回值被取整

原版是 `(float)` 直接转换，保留小数；Cubium 用了 `std::floor`。实测输出里
`damage="11.0898"`，取整后会是 `11`——几乎所有输入下两者都不同。

#### 单元设计

480 个组合刻意跨越 `distance == radius * 2` 这条边界（`inRange` 的分界），
并包含多个超距值。只有把超距值放进表里，差异一才会显示出来——如果只测射程内的
实体，两处实现是完全一致的。
