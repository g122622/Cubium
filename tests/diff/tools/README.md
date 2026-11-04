# 矿井随机序列复算工具

`MsTrace.java` 按原版的调用顺序复算某个区块的矿井随机序列，用来和探针的打点逐条比对。

## 编译与运行

```bash
javac -proc:none -cp "<minecraft-merged.jar>;<libraries/*.jar>" -d build/tools tests/diff/tools/MsTrace.java
java -cp "build/tools;<minecraft-merged.jar>;<libraries/*.jar>" MsTrace <worldSeed> <chunkX> <chunkZ>
```

## 已知局限

对 `overworld/0/-13/-60`，本工具算出 `corridorCtor nextInt(3) = 1`（rails=false），
而 oracle 实测第 2 个构件 `hasRails=true`（即 `nextInt(3)==0`）。
两者矛盾，说明本工具复现的调用序列还缺一环，尚未查明。
在查明之前**不要**用它下结论。
