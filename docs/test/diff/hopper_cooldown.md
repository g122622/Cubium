# 差分能力：`hopper_cooldown`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 89 个能力，10 种冷却状态组合全部一致。漏斗的传输冷却时序此前没有被比对过。

#### 原版代码

```java
public static void pushItemsTick(...) {
    hopper.cooldownTime--;                        // 无条件递减
    hopper.tickedGameTime = level.getGameTime();
    if (!hopper.isOnCooldown()) {
        hopper.setCooldown(0);
        tryMoveItems(level, pos, state, hopper, () -> suckInItems(level, hopper));
    }
}

if (!hopper.isOnCooldown() && state.getValue(HopperBlock.ENABLED)) {
    if (!hopper.isEmpty())  { flag = ejectItems(...); }
    if (!hopper.inventoryFull()) { flag |= supplier.getAsBoolean(); }
    if (flag) {
        hopper.setCooldown(8);                    // MOVE_ITEM_SPEED
    }
}

private boolean isOnCooldown() { return this.cooldownTime > 0; }
```

#### 三个关键点

**1. 递减发生在「是否冷却」判定之前。** `cooldownTime == 1` 的那个 tick：先减到 0，
然后 `isOnCooldown()` 为假，**本 tick 就能传输**。实测：

| 单元 | `after` | `attempted` | `moved` |
|---|---|---|---|
| `1\|true\|true` | 8 | **true** | **true** |
| `2\|true\|true` | 1 | **false** | false |

也就是说「设置冷却 8」之后，第 2..8 个 tick 递减、第 9 个 tick 再次传输——间隔恰好 8 个 tick。

**2. 传输成功才设置冷却。** `1|true|false`（无物品可动）得到 `after=0, cooldownSet=false`
——没有传输就不设冷却，下一个 tick 会立刻再试。

**3. 递减是无条件的**，`cooldownTime` 会一路减到 `-1` 并停在那里（`-1` 是构造器的初始值）。

#### 一处「实现不同但行为等价」的差异

Cubium 的 `HopperEntity::tick` 写的是：

```cpp
if (m_transferCooldown > 0) { --m_transferCooldown; }
```

原版是无条件的 `cooldownTime--`。差别在于冷却为 0 时：原版减到 -1，Cubium 停在 0。

**行为上看不出区别**——因为 `isOnCooldown()` 只在 `> 0` 时为真，两者都不冷却。但 NBT 里
存下的 `TransferCooldown` 值会不同（原版 -1、Cubium 0）。本能力比较的是内部状态值，因此
这个差异在校验范围内被记录下来；它不影响玩法，但会影响存档数据的一致性。

#### 验证方法

延续前两轮的做法，跑完逐条打印实际输出核对分支，10 条全部命中（见上表前两行即为边界用例）。

#### 未覆盖范围

- `ejectItems` / `suckInItems` 各自的槽位遍历顺序与方向选取（需要世界与容器）
