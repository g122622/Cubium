# 矿井构件与随机序列导出工具

两个工具，都只用于差分排查，不参与构建。

| 文件 | 用途 |
|---|---|
| `MsDrive.java` | 直接驱动原版 `MineshaftPieces`，导出构件清单（含父子关系）与每次抽取的归属 |
| `MsTrace.java` | 从 `setLargeFeatureSeed` 起手工复算随机序列（有已知局限，见该文件说明） |

## 编译与运行

```bash
javac -proc:none -cp "<minecraft-merged.jar>;<libraries/*.jar>" -d build/tools tests/diff/tools/MsDrive.java
java -cp "build/tools;<minecraft-merged.jar>;<libraries/*.jar>" MsDrive <worldSeed> <chunkX> <chunkZ>
```

## 关键经验

- **`nextBoolean()` 必须通过覆写 `next(int)` 捕获**。它来自 `BitRandomSource` 的默认方法
  `this.next(1) != 0`，覆写 `nextBoolean()` 本身不会生效。
- 覆写里给每次 `nextInt` 打标时，「所属构件」只能用 `addPiece` 时更新的游标近似——
  一个构件的主口创建出子构件后，父构件剩余的抽取会被记到子构件名下。读日志时要按
  `PIECE` 行与深度关系重新归属。
## 已归档的对照数据

`mineshaft_seq_overworld_0_-13_-60.txt` 是用 `MsDrive` 对 `overworld/0/-13/-60` 导出的
完整序列（159 个构件 + 每次抽取的归属）。差分排查时直接拿它当基准，不必重新跑。

该单元里 `[28]`（双层交叉点）的子构件构成：

| 子构件 | 相对父构件 min | 来源 |
|---|---|---|
| `[29]` | `(1, -5, -9)` | 主口北，楼梯（`minY-5`） |
| `[32]` | `(5, 0, 1)` | 主口东 `maxX+1` |
| `[37]` | `(-15, 4, 1)` | 双层西 `minX-1, minY+4` |
| `[41]` | `(1, 4, 5)` | 双层北 `minX+1, minY+4` |

四个子构件里**没有主口西**，说明西侧主口在原版里失败了（碰撞）。
双层分支命中了两个方向，而 Cubium 在同一位置只命中一个（西向）。
