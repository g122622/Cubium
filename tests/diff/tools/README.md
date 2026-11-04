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
