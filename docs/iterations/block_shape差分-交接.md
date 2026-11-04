# 交接文档：`block_shape` 差分收敛（1166 方块 × 外形/碰撞/支撑）

> 面向接管者：交代任务目标、已确认修好的缺陷、当前阻塞点与精确的复现步骤。
> 撰写时的分支：`main`（工作区有未提交改动，见第七节）。
> 日期：2026-11-04。

---

## 一、任务目标与验收标准

差分框架把同一份输入分别喂给 **Cubium C++ 探针**与 **原版 Java oracle**，要求两侧输出逐字节相同。

相关文件：

| 文件 | 作用 |
|---|---|
| `tests/diff/probes/block_shape.cpp` | C++ 探针：`list` 枚举方块，`run <in> <out>` 逐行输出三种形状 |
| `tests/diff/java/dev/horizonlab/diff/BlockShapeOracle.java` | Java oracle：同一套输入输出约定，调用原版 `getShape/getCollisionShape/getBlockSupportShape` |
| `tests/diff/harness/runner/session.ts` | 编排：编译 oracle、跑探针、比对并写 `report.json` |
| `tests/diff/harness/oracle/environment.ts` | 构造 oracle classpath（`minecraft-merged` jar + bundler 依赖） |

```bash
node tests/diff/harness/runner.ts block_shape            # 含构建
node tests/diff/harness/runner.ts block_shape --no-build # 只用现有二进制
```

- 输入：全部 1166 个方块的**默认状态**（`tests/diff/probes/block_shape.cpp` 的 `list`）。
- 输出：三种形状各自的**16³ 体素网格**，网格索引 `(y*16+z)*16+x`，每个体素用中心点 `(i+0.5)/16` 做
  半开区间测试，展开后按游程压缩成 `字符+次数` 逗号分隔串。
- 报告：`build/diff/block_shape/run/report.json` → `differences[].expected.result` / `actual.result`，
  形如 `shape=...|collision=...|support=...`。

验收标准：`differences` 为空。

**起始状态 4 个差异**：`bamboo`、`grindstone`、`lectern`、`pointed_dripstone`。

---

## 二、已确认修复（这几处是真缺陷，可直接保留）

### 1. `Block::getCollisionShapeAt` 默认实现用错了基准 —— 一次消掉 70 个差异

`src/common/world/block/Block.hpp`。原实现：

```cpp
if (!m_hasCollision) return VoxelShapes::empty();
return getShapeAt(state, pos);     // ← 错
```

原版 `getCollisionShape` 是独立虚函数，很多方块覆写返回**固定常量**（悬挂告示牌
`Shapes.rotateHorizontal(Block.column(8,2,16))`、南瓜头、砂轮、骨架头颅……）。用 `getShapeAt`
会把它们全部退回 `getShape`，于是 70 个方块（全部 `*_wall_hanging_sign`、`potted_*`、
`cactus`、`snow`、`sculk_shrieker`、`chorus_flower` 等）碰撞段全部错。改成：

```cpp
MC_UNUSED(pos);
return getCollisionShape(state);
```

**注意**：同一轮里还改过 `BlockState::getCollisionShapeAt`（`src/common/world/block/BlockState.cpp`），
已还原为直接转调，不要在那层加语义。

### 2. 砂轮旋转矩阵写反 —— `grindstone` 差异清零

`src/common/world/block/blocks/functional/GrindstoneBlock.cpp`。

用 Java 实测 `com.mojang.math.OctahedralGroup` 的六个变换，得到真实作用：

| 常量 | 对 `(x,y,z)` 的作用 |
|---|---|
| `BLOCK_ROT_Y_90` | `(x,y,z) → (z, y, -x)` |
| `BLOCK_ROT_Y_180` | `(x,y,z) → (-x, y, -z)` |
| `BLOCK_ROT_Y_270` | `(x,y,z) → (-z, y, x)` |
| `BLOCK_ROT_X_90` | `(x,y,z) → (x, -z, y)` |
| `BLOCK_ROT_X_270` | `(x,y,z) → (x, z, -y)` |

换算到 16³ 体素下标（`+1` 偏移由取 min/max 抵消）：`BLOCK_ROT_Y_90` 是 `(x, z) → (z, 1 - x)`，
`BLOCK_ROT_X_270` 是 `(y, z) → (z, 1 - y)`，`BLOCK_ROT_X_90` 是 `(y, z) → (1 - z, y)`。

另外 `BLOCK_ROT_Y_180.compose(BLOCK_ROT_X_90)` 是**先应用 X_90、再应用 Y_180**（右结合）。

修复后砂轮用统一的八面体变换表 `_Octahedral {axis[3]; flip[3];}` + `_rotateShape`，三个档位：

```
WALL    = rotateY(base, turns)
FLOOR   = rotateY(rotate(base, BLOCK_ROT_X_270), turns)
CEILING = rotateY(rotate(rotate(base, BLOCK_ROT_X_90), BLOCK_ROT_Y_180), turns)
```

其中 `back = boxZ(8.0, 2.0, 14.0, 0.0, 12.0)` 即 x 4..12、y 6..14、z 0..12 像素；
`arm = box(2,6,7,4,10,16) ∪ box(2,5,3,4,11,9)`，`arm2 = rotate(arm, INVERT_X)`（**只翻 X，不翻 Z**）。

### 3. 讲台 `boxZ` 语义读错（但仍未收敛，见第四节）

`boxZ(width, height, y1, y2)` 的 **`y1..y2` 是竖直区间**，`width`/`height` 分别是 x/z 的居中宽度：

```java
// Block.java:201
public static VoxelShape boxZ(double width, double y1, double y2, double zFrom, double zTo) {
    double d0 = width / 2.0;
    return box(8.0 - d0, y1, zFrom, 8.0 + d0, y2, zTo);
}
```

所以 `boxZ(16.0, 10.0, 1.0, 5.333333)` = x 0..16、**y 1..10**、z 5.333..14。

### 4. 讲台形状表按 `Direction` 枚举值索引越界

`Direction` 枚举：`Down=0, Up=1, North=2, South=3, West=4, East=5`。`LecternBlock` 原来写
`m_shapesByFacing[static_cast<size_t>(facing)]` 配 `std::array<CollisionShape, 6>`，只初始化了 0..3，
North(=2)/South(=3) 能撞上，West(=4)/East(=5) 落到未初始化槽位。已改为 `_facingSlot()` 显式映射 0..3。

---

## 三、已确认的其它事实（写代码时可直接引用）

- **`Block.column(w, y1, y2)`** = `column(w, w, y1, y2)` = `box(8-w/2, y1, 8-w/2, 8+w/2, y2, 8+w/2)`，
  即横截面 `w` 像素见方、居中。实测：`column(6,…)` → `[0.3125, 0.6875]`（x 5..11），
  `column(14,…)` → `[0.0625, 0.9375]`（x 1..15），`column(3,…)` → `[0.40625, 0.59375]`（x 6.5..9.5）。
- **`offsetType(XZ)`** 对**所有状态**生效（挂在方块属性上），不是按状态注册：

  ```java
  long i = Mth.getSeed(pos.getX(), 0, pos.getZ());
  double d0 = Mth.clamp(((i & 15) / 15.0 - 0.5) * 0.5, -f, f);
  double d1 = Mth.clamp(((i >> 8 & 15) / 15.0 - 0.5) * 0.5, -f, f);
  ```

  `f = block.getMaxHorizontalOffset()`，默认 `0.25`；`PointedDripstoneBlock` 覆写为
  `SHAPE_BASE.min(X) = 2/16 = 0.125`。**注意 C++ 的 `mthGetSeed` 在实机上与 Java 一致**（竹子实测吻合）。
- `PointedDripstoneBlock` 默认状态是 `thickness=tip, tip_direction=up`，Java 实测于 `BlockPos.ZERO`：
  offset `(-0.125, 0, -0.125)`，`getShape` = `getCollisionShape` = `[0.1875 0 0.1875]-[0.5625 0.6875 0.5625]`
  （x 3..9 像素、y 0..11 像素）。
- `BambooStalkBlock`：`SHAPE_SMALL = column(6,0,16)`、`SHAPE_LARGE = column(10,0,16)`、
  `SHAPE_COLLISION = column(3,0,16)`，三者都 `.move(getOffset(pos))`。
- `LecternBlock`（Java 直查 `BlockPos.ZERO`，`FACING=NORTH`）：
  `bounds = [0,0,0] → [1, 1.125, 1]`，盒列表
  `[0,0,0]-[1,0.125,1]`、`[0.25,0.125,0.25]-[0.75,0.875,0.75]`、
  `[0,0.0625,0.3333]-[1,0.625,0.875]`、`[0,0.3333,0.6042]-[1,0.75,1]`、`[0,0.6042,0.875]-[1,0.875,1.125]`。

---

## 四、当前阻塞点：构建产物 / 运行环境存在**不确定性**

剩余 2 个差异：`minecraft:lectern`（shape 段）、`minecraft:pointed_dripstone`（collision 段）。

它们呈现出**同一个二进制、同一份输入、结果互相矛盾**的特征，在定位根因之前，继续改源码没有意义。

### 4.1 现象 A：探针输出长度抖动（最早发现）

对 `minecraft:bamboo` 的 collision 段，探针在同一输入下曾输出
`05,12,…`（244 字节）与 `0119,12,…`（243 字节）两种结果，相差一个前导 `0`。测过但**未定位**：

- 单条输入 vs 972 条批量输入：两种情况下都得到 244 字节，无差异。
- 构造期与查询期的 `this` 指针一致；构造末尾打印 4 个槽位均为 5 个盒。
- 加过冻结钩子把盒坐标逐字节落盘：恒为 `[0.40625..0.59375]`（即 `column(3)` 的正确几何）。
- 换行符 CRLF 测试：得到第三种结果 `0119,12,014`。
- `/usr/bin/diff` 是 211KB 的 PE32+ 可执行文件（可能是被替换过的 msys 工具，未证实）。

### 4.2 现象 B：lectern 的 oracle 与「Java 直接查询」互相矛盾

- 我写 `/tmp/jv/Q43.java`、`Q49.java` 直接调用 `Blocks.LECTERN.defaultBlockState().getShape(...)`，
  得到 y 上界 1.125（18px）、z 1..14，与源码 `boxZ` 语义一致。
- 但差分 oracle 的 `build/diff/block_shape/java/oracle_outputs.jsonl` 第 509 行显示：
  - 斜台段 **y 从 0 就开始**（无 y=1 的底部留白）；
  - y=15 时 z 5..13 全覆盖（应只到 z 13 的窄条），y=13 时 z 1..9 全覆盖而 x 0..15 不变。

  这种「竖直方向整体下移 + 多层叠加」的形状，不可能由任何沿 z 的水平横条旋转得到。
  换句话说，oracle 输出的形状**无法由我读到的 Java 源码解释**。

### 4.3 现象 C：pointed_dripstone 的两个二进制给出不同答案

- 手工运行 `build/bin/RelWithDebInfo/diff_block_shape.exe run <只有该方块的 jsonl> <out>`：
  `collision=085,16,…`（体素 x 0..8，即盒 `[0, 0.5]`）。
- 差分跑出的 `probe_outputs.jsonl`：`collision=051,16,…`（体素 x 1..5，即盒 `[0.0625, 0.375]`）。
- Java 实测默认状态：盒 `[0.1875, 0.5625]`（x 3..9）。

三者互不相同。注意 C++ 侧 `getOffset` 用 `math::clamp`（`std::clamp`）而 Java 用 `Mth.clamp`，
两者对 `[-0.25, 0.25]` 内取值应当一致，不足以解释这种量级的差异。

### 4.4 已排除的因素

- 输入规模（单条 / 500 条 / 全部 1166 条）不影响同一个方块的输出。
- 构造期与查询期的实例地址一致（`this` 打印过）。
- 形状表槽位在构造末尾正确填充（打印过：`slots 5 5 5 5`）。
- oracle 的 classpath 与手工查询用的是**同一个** `minecraft-merged` jar（`%TEMP%/cp.txt` 与
  `tests/diff/harness/oracle/environment.ts` 都指向 `.gradle/caches/fabric-loom/.../minecraft-merged-...jar`）。
- 两个 Java 进程里 `System.out` 都被 log4j 接管，`forAllBoxes` 的 `Consumer` 直接打印会丢，
  必须用 `toAabbs()` 再打印——这一点容易误判成「形状为空」。

### 4.5 建议的下一步排查顺序

1. **核对 `mc_common` 静态库是否被链接了两份**：`diff_block_shape.exe` 同时链 `mc_common` 与
   `mc_bedrock_addon`/`mc_offset_allocator`（见 `tests/diff/CMakeLists.txt` 的
   `mc_add_diff_probe`）。若 `mc_bedrock_addon` 也把 `mc_common` 的若干 TU 编进去，就会出现
   「同一个符号有多个实现、按链接顺序随机挑一个」——这与观察到的「不同次运行不同结果」高度吻合。
   用 `llvm-nm`/`dumpbin /symbols` 对比 `mc_common.lib` 与最终 exe 的符号表即可确认。
2. **检查 `build/diff/block_shape/java/classes` 是否陈旧**：`compileOracle` 用
   `javac -d <classes>`，javac 只重编「源文件比 .class 新」的类。若曾手工改过包名/内部类，
   旧 `.class` 可能残留并被优先加载。删掉 `build/diff/block_shape/java/classes` 后重跑一次。
3. **检查 oracle 结果缓存**：`session.oracleCache` 以 `sources/toolchainPaths/argv/units/inputs` 的哈希为键
   （`tests/diff/harness/runner/session.ts` 的 `runOracle` 调用处）。若 `sources` 只覆盖了部分文件
   （例如漏了内部类或共享类），改代码后仍会命中旧缓存。
4. **用可复现的最小用例固定现象**：`printf '{"unit":"minecraft:pointed_dripstone"}\n'` 生成的
   单行 jsonl，连续跑 20 次并统计 `collision=` 前 6 个字符的分布。若出现两种以上取值，即证明
   是二进制/环境问题而非源码问题。
5. 确认运行环境：`-march=native -mavx2`（`CMakeFiles/impl-RelWithDebInfo.ninja` 的 `FLAGS`）。
   若构建机与运行机不是同一台（远程共享目录），可能出现指令集不匹配导致的未定义行为。

---

## 五、剩余两个方块的正确实现（待现象消除后落地）

### 5.1 `minecraft:lectern`

`src/common/world/block/blocks/functional/LecternBlock.cpp`，当前已经是：

```cpp
// 斜台（NORTH 基准）：y 区间 1..10 / 5.333..16 / 9.667..18，z 区间见下
box(0, 1.0, 5.333333, 16, 14.0, 10.0)
box(0, 5.333333, 9.666667, 16, 16.0, 12.0)
box(0, 9.666667, 14.0, 16, 18.0, 14.0)
// 碰撞：column(16,0,2) ∪ column(8,2,14) = [0,0,0]-[1,2,1] ∪ [4,2,4]-[12,14,12] px
```

朝向映射（`_facingSlot` + `rotateY`）目前是 EAST/E/S/W = 1/2/3 次 `(x,z) → (z, 1-x)`。
**这一步尚未验证是否与 oracle 一致**——因为 oracle 本身的可信度存疑（第四节 4.2）。

### 5.2 `minecraft:pointed_dripstone`

`src/common/world/block/blocks/cave/PointedDripstoneBlock.cpp`：

- 六种厚度的外形表已经正确（`column(6/8/10/12)` 换算后的像素盒）。
- `getOffsetAt` 已改为**所有状态**都返回 `getOffset(pos)`（原版 `offsetType` 挂在方块属性上）。
- `getCollisionShape` 已改回 `return getShape(state);`（无位置、无偏移）。
- 待核对：`getMaxHorizontalOffset()` 返回 `0.125f`（正确），以及 `math::clamp` 与 `Mth.clamp` 的边界行为。

---

## 六、验证手段备忘

- 手工跑探针：`build/bin/RelWithDebInfo/diff_block_shape.exe run <in.jsonl> <out.jsonl>`，
  `argc==4 && argv[1]=="run"`，否则打印 usage 退出码 2。
- 原版查询环境：classpath 在 `%TEMP%/cp.txt`，用 `/tmp/fixcp.js` 把反斜杠转正斜杠并给
  `build/...` 补绝对路径；查询程序模板：

  ```java
  SharedConstants.tryDetectVersion();
  Bootstrap.bootStrap();
  var st = Blocks.LECTERN.defaultBlockState();
  System.out.println(st.getShape(null, BlockPos.ZERO, CollisionContext.empty()).bounds());
  for (var b : st.getShape(...).toAabbs()) System.out.println(b);   // 不要用 forAllBoxes 打印
  ```

- 体素图可视化：把 `report.json` 的 `expected.result`/`actual.result` 按 `|` 拆段，再按游程展开成
  16×16×16 网格，逐 y 打印 16 行。

---

## 七、工作区状态

未提交改动（`git status`）：

- 本轮修改的核心文件：`Block.hpp`、`Block.cpp`、`BlockState.cpp`、`CollisionShape.hpp`、
  `GrindstoneBlock.cpp`、`LecternBlock.cpp`、`BambooBlock.cpp/.hpp`、`PointedDripstoneBlock.cpp/.hpp`、
  `MangrovePropaguleBlock.cpp/.hpp`、`CaveBlocks.cpp`，以及 `tests/diff/probes/block_shape.cpp`、
  `tests/diff/java/dev/horizonlab/diff/BlockShapeOracle.java`。
- 另有约 40 个方块文件的改动是**上一轮（更早的会话）**为其它形状差异做的，未经本轮验证，
  提交时请单独 review。
- 仓库根目录有一个未跟踪文件 `$nul`（内容是某次探针输出的一行 JSON），来源是某条命令里
  `> $nul` / `2>nul` 的错误重定向。不是本轮代码引入的，但**建议一并清理，并检查是否还有别处**。
- `logs/*.log.gz` 是运行日志，未跟踪，提交时排除。

## 八、提交前检查清单

1. `node scripts/format/clang_format_all.ts`（需 node24）。
2. `./scripts/configure.sh build`（Windows 唯一构建入口，需 ≥30 分钟）。
3. `node tests/diff/harness/runner.ts block_shape --no-build`，确认 `differences` 为空。
4. 与本次改动相关的单测：`build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*BlockShape*:*Lectern*:*Grindstone*:*Bamboo*:*PointedDripstone*"`。
5. 提交信息：`fix(block): 修正形状与碰撞的若干偏差（砂轮旋转、碰撞入口、讲台/竹子/滴水石锥几何）`。