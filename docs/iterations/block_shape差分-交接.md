# `block_shape` 差分交接与收敛记录

日期：2026-11-04。接手时提交为 `893fe23fb`，此前形状修复已提交于 `a1b74b1c2`。

状态：已收敛并完成合并验证。强制重跑 oracle 后默认状态 1166/1166、全部状态 29671/29671 一致，差异与已知缺口均为 0；193 个相关单元测试通过。扩展筛选额外命中的花坛测试有 2 个失败，未改动或调查这些无关用例。

## 目标与范围

全部 1166 个方块的默认状态，在原点查询外形、碰撞形状和支撑形状，两侧输出逐字节一致，`differences` 为空。

- 探针：`tests/diff/probes/block_shape.cpp`。
- 原版：`tests/diff/java/dev/horizonlab/diff/BlockShapeOracle.java`。
- 编排：`tests/diff/harness/runner/session.ts`。
- 报告：`build/diff/block_shape/run/report.json`。
- 表示：每格 16³ 个中心点，按 `(y * 16 + z) * 16 + x` 展开，游程编码为 `字符+次数`，逗号分隔。当前包含判断为严格内部 `min < p < max`。

这项差分只覆盖默认状态、原点及单位方块内部的采样点，不代表全部状态或连续几何已经穷尽验证。单元测试另覆盖本轮涉及的非默认状态和讲台高于一格的台面。

## 本轮重新确认的证据

1. `node tests/diff/harness/runner.ts block_shape --no-build --fresh --samples 0` 强制重新运行 oracle，报告 `oracleReused=false`，仍是讲台外形与滴水石锥碰撞两个差异。
2. 同一个探针、同一份竹子/讲台/滴水石锥输入，独立运行 20 次，输出 SHA-256 全部相同：`0dab77d054c4fa350e45edfb81e1f749ab3f10e09046921e6c562c48ac484c1d`。
3. 用编排器的 `loadOracleEnvironment(projectLayout())` 获取相同 classpath，单独运行 Java 查询并写文件，得到与 oracle 一致的讲台几何及滴水石锥偏移。
4. 将 Java 源码中的正确讲台盒体体素化，与报告中的 expected 完全相同（单位格内 1872 个占用体素）。

因此，之前记录的“同一二进制输出抖动”“oracle 无法由源码解释”在本轮没有复现，不能作为已确认的环境缺陷。两处差异均有直接的源码原因，无需修改链接或缓存框架。

## 讲台：修正参数顺序与东西朝向

Java 实际调用的是五参数版本：`boxZ(width, yMin, yMax, zMin, zMax)`。此前交接遗漏了调用中的一个参数，并据此交换了 Y/Z 区间。

北向斜台的正确像素盒为：

| X | Y | Z |
|---|---|---|
| 0..16 | 10..14 | 1..5.333333 |
| 0..16 | 12..16 | 5.333333..9.666667 |
| 0..16 | 14..18 | 9.666667..14 |

底座为 `[0,0,0]-[16,2,16]`，立柱为 `[4,2,4]-[12,14,12]`。外形包含斜台，碰撞与支撑只包括底座和立柱。

绕 Y 轴顺时针的实际盒体映射为 `(x,z) → (1-z,x)`。北→东→南→西依次旋转 0/1/2/3 次；此前讲台用了相反方向。以直接查询 Java 的四朝向形状为依据修正，并将缓存缩至四个水平槽位。

## 滴水石锥：补齐碰撞位置入口

`Block::getCollisionShapeAt` 默认转调独立的 `getCollisionShape`，这是此前修复的必要行为：不能让所有方块的碰撞退回外形，否则会破坏碰撞与外形不同的方块。

但 `PointedDripstoneBlock` 必须自行覆写位置入口，返回 `getShapeAt(state,pos)`。此前只有外形和支撑带偏移，碰撞仍取未偏移的柱体。

直接查询 Java，在原点的结果为：

- 默认状态：`thickness=tip, tip_direction=up`。
- 偏移：`(-0.125,0,-0.125)`。
- 外形、碰撞与支撑：`[0.1875,0,0.1875]-[0.5625,0.6875,0.5625]`。

注意 RLE 的 `051,16` 表示先有 **51 个零，再有 6 个一**；`085,16` 表示先有 **85 个零，再有 6 个一**。按 16³ 网格解码后分别从 `(x,z)=(3,3)` 和 `(5,5)` 开始，恰好相差两像素偏移，不能把字符串中的数字直接当作轴坐标范围。

## 合并前回归与验证

新增回归：

- `LecternBlockTest.Shapes_AllFacingsKeepSlopingTopSeparateFromCollision`：四朝向 × POWERED/HAS_BOOK 的全部组合；覆盖三级斜台、上下空隙和 y=17.5 像素处外形。
- `PointedDripstoneBlockTest.PositionedShapes_AllStatesUseClampedHorizontalOffset`：五厚度 × 上下方向 × 含水状态 × 五坐标，分别检查外形、碰撞与支撑的六个边界。

验证命令：

```powershell
node scripts/format/clang_format_all.ts <本轮修改的 C++ 文件>
bash -c './scripts/configure.sh build'
node tests/diff/harness/runner.ts block_shape --no-build --fresh --samples 0
ctest --test-dir build --build-config RelWithDebInfo --output-on-failure -j8 -R 'LecternBlockTest|PointedDripstone|Grindstone|BambooBlock|BlockShape'
```

- 格式化：6 个修改的 C++ 文件处理成功。
- 完整构建：退出码 0，耗时 26 分 14 秒。
- 差分：`oracleReused=false`、`matched=1166`、`differenceCount=0`、`knownDifferenceCount=0`。
- CTest：142/142 通过，耗时 153.92 秒（含用例发现与准备）。

## 工作区说明

接手时跟踪文件均已提交；未跟踪项是 `$nul` 与若干 `logs/*.log.gz`。已确认 `$nul` 内容为旧探针的一行输出并清理。本轮只提交明确修改的代码、测试与文档，运行日志不纳入提交。

## 合并主分支

本轮修复提交为 `2e86d1844`。随后按用户要求合并 `origin/main`（起点 `421db965c`，含 96 个后续提交），逐项解决形状实现与序列化的重叠改动。保留讲台书籍变化事件、世界摧毁接口等新增行为，以及本轮的精确讲台几何、位置偏移和回归测试。默认状态能力保留逗号游程编码，两侧同步；新增全状态能力保留其自身编码。

上文构建与 142 个单测结果属于合并前验证；合并后另行重新构建，并运行默认状态、全状态形状差分及相关单测。

首次合并后构建成功（38 分 17 秒），默认状态仍为 1166/1166 一致。全状态验证暴露 144 处差异：16 色床各 8 个非默认状态，以及脚手架 16 个 BOTTOM 状态。进一步修正床头/床脚的连接方向、东西向腿位置，补齐脚手架底板，并将无实体碰撞固定为稳定形状。活板门打开仍有碰撞，相关旧断言已更正；床连接方向与脚手架的旧断言同步更新。

最终验证：

- 完整入口增量构建成功，退出码 0，耗时 1 分 22 秒。
- `node tests/diff/harness/runner.ts block_shape block_shape_all --no-build --fresh --samples 0`：1166/1166 与 29671/29671 一致，两项均为 `oracleReused=false`，差异和已知缺口均为 0。
- CTest 筛选 `LecternBlockTest|PointedDripstone|Grindstone|BambooBlock|BlockShape|BedBlock|ScaffoldingBlock`：193 个相关用例全通过；因 `BedBlock` 子串额外选中 37 个 `FlowerBedBlockTest`，其中 35 个通过，2 个失败。总计 228/230，耗时 180.89 秒。
- 两个无关失败为 `FlowerBedBlockTest.Shape_AmountFour_CoversFullBlock`、`FlowerBedBlockTest.Shape_BoxCount_IncreasesWithAmount`。按项目规范未调查、未隔离重跑，也未修改花坛实现或断言。
