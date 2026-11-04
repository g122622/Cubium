# 差分能力：`block_shape`

比较全部 1166 个方块默认状态在原点的外形、碰撞形状和支撑形状。原版参照来自 Minecraft 1.21.11 merged jar。

2026-11-04 合并后验证：强制重跑 oracle 后默认状态 1166/1166、全部状态 29671/29671 一致，未登记任何已知差异；193 个相关单元测试通过。扩展筛选额外命中的两个花坛测试失败，详见收敛记录。

## 文件与运行

| 文件 | 用途 |
|---|---|
| `tests/diff/probes/block_shape.cpp` | 枚举方块并查询 C++ 的三种形状 |
| `tests/diff/java/dev/horizonlab/diff/BlockShapeOracle.java` | 查询 Java 的三种形状 |
| `tests/diff/harness/harnesses/block_shape.ts` | 构造 `{unit}` 输入 |
| `tests/diff/harness/runner/session.ts` | 编译 oracle、执行、缓存及比较 |
| `build/diff/block_shape/run/report.json` | 本机差分结果 |

```powershell
# 项目构建必须使用规定入口。
bash -c './scripts/configure.sh build'
node tests/diff/harness/runner.ts block_shape --no-build --fresh --samples 0
```

`--fresh` 强制重跑 oracle。报告中的 `oracleReused` 可确认是否使用参照结果缓存。

`block_shape_all` 是同一查询维度的全部方块状态能力，来自后续主分支提交。合并涉及形状实现时，应与默认状态能力一并运行：`node tests/diff/harness/runner.ts block_shape block_shape_all --no-build --fresh --samples 0`。两项能力的输出编码分别由各自探针与 oracle 定义。

## 输入输出

输入为 `{ "unit": "minecraft:lectern" }`，状态取该方块默认状态，位置固定为 `(0,0,0)`。每项输出为 `shape=...|collision=...|support=...`。

每种形状在单位格内取 16³ 个体素中心，按 `(y * 16 + z) * 16 + x` 展开。当前两侧均用严格内部判断 `min < p < max`，命中任意盒子即占用。空形状记作 `EMPTY`；非空形状按连续的 0/1 游程编码，每段为“字符+次数”，段间用逗号分隔。

例如 `051,16` 是 51 个零、6 个一，不能读作某个轴上的坐标范围。

覆盖限制：这不是连续几何的完整比较，不覆盖非默认状态、其他坐标、格外几何或采样点之间的细节。两侧严格内部判断还可能受恰好经过采样点的盒子分界影响。讲台高出一格的台面、四朝向及滴水石锥全部状态另有单元回归。

## 查询入口与偏移

三种查询必须使用带位置入口：

| Java | C++ |
|---|---|
| `getShape(level,pos)` | `getShapeAt(pos)` |
| `getCollisionShape(level,pos,context)` | `getCollisionShapeAt(pos)` |
| `getBlockSupportShape(level,pos)` | `getBlockSupportShapeAt(pos)` |

`offsetType` 仅定义偏移函数；是否应用偏移由具体形状实现决定。花、竹子、竹笋和滴水石锥会平移外形；草、树苗等有偏移属性也不能据此统一平移查询结果。

碰撞与外形独立。公共碰撞位置入口必须保留 `getCollisionShape` 的分派，不能全局退回外形。竹子自行使用更细的碰撞柱并平移；滴水石锥的碰撞则复用带偏移的外形。支撑形状同样需要保留独立覆写。

oracle 使用最小只读 `BlockGetter`，实体查询返回 null，方块为空气，流体为空；不能直接给全部方块传 null 世界。

## 已确认的关键陷阱

- 逐盒坐标比较会把不同的并集拆分误报成形状差异；体素采样用于比较覆盖结果。
- `column(width,yMin,yMax)` 的 width 是居中截面的完整宽度，不能当作半径。
- 五参数 `boxZ(width,yMin,yMax,zMin,zMax)` 不能交换 Y/Z 区间。讲台北向斜台为 y=10..14、12..16、14..18 像素，最高达到 18 像素；碰撞与支撑只有底座和立柱。
- 北向转东向的盒体映射为 `(x,z) → (1-z,x)`。不要根据单个方向向量的旋转推断整个形状的变换方向，应直接查询四朝向几何。
- `Direction` 的水平方向枚举值不是紧凑的 0..3，形状表必须显式映射槽位。
- 滴水石锥的最大水平偏移为 0.125；默认尖端在原点的外形、碰撞与支撑均为 `[0.1875,0,0.1875]-[0.5625,0.6875,0.5625]`。
- 比较缓存与新输出前，应核实 `oracleReused`、输入文件、实际可执行文件和 RLE 解码。2026-11-04 的复查中 20 次独立探针输出一致，旧交接中的环境抖动推断未复现。

## 关联资料

- [本轮收敛记录](../../iterations/block_shape差分-交接.md)。
- [face_sturdy](face_sturdy.md)：支撑形状的面判定。
- [shape_face](shape_face.md)：形状的面切片。
- [entity_shape](entity_shape.md)：实体侧的形状能力。
