# 差分测试：跨能力规律与教训

本文件汇总在新增差分能力过程中**反复出现**的问题模式。它们不属于某一项能力，而是每一次
新增能力时都可能踩到的坑，因此在动手前应当对照检查。

各项能力的具体调查记录见同目录下以能力名命名的文件。

---

## 一、`-ffast-math` 会改写浮点中间值

本仓库全局启用 `-ffast-math -march=native -mavx2`。这会允许编译器把除法改写成倒数乘法、
并跨表达式重排浮点运算，导致「先算一个浮点中间值、再与某个精确值比较」的逻辑在编译期被
改变。Java 侧没有这种优化，于是两侧分叉。

### 已知案例

| 能力 | 现象 | 根因 |
|---|---|---|
| `explosion_seen_grid` | 覆盖率残留 `2.78e-17` 而非精确 `0` | `floor(1/step) * step` 被折叠成 `(1/step) * step` |
| `direction_rotate` | `fromYRot(-45)` 得到 EAST 而非 SOUTH | `-45 / 90.0` 被算成 `-0.50000000000000001908`，加 `0.5` 得 `-1.9e-17`，`Mth.floor` 因此返回 `-1` |

### 规避方式

把中间值强制落成一个独立变量，必要时加 `volatile`，阻止编译器跨过它重排：

```cpp
volatile f64 quotient = deg / 90.0;
const f64 shifted = quotient + 0.5;
const i32 idx = mthFloor(shifted) & 3;
```

另一种方式是在结果层面加显式的接近零判断：

```cpp
const f64 covered = std::min(1.0, std::max(0.0, computed));
return std::abs(covered - 1.0) < 1e-12 ? 0.0 : (1.0 - covered);
```

**凡是新增涉及浮点比较的能力，都要先问：这个中间值会不会被 `-ffast-math` 改写？**

---

## 二、把「门控」搬进被调函数

某一层守卫（范围钳制、类型过滤、可用性判断）在原版里只作用于**某一条调用路径**，而
Cubium 把它下沉到被调函数内部，于是对**所有**调用者生效。

这类错误的特点是：被调函数单独看自洽、逻辑正确，但调用语义变了。

### 已知案例

| 能力 | 现象 |
|---|---|
| `explosion_damage` | `inRange` 钳制被下沉 |
| `food_stats` | 下界钳制缺失，与上界不对称 |
| `enchantment_selection` | 宝藏过滤被下沉到所有路径 |

**差分测试是目前唯一能发现这类问题的手段**——单元测试按调用点写，会跟着错误实现一起写对。

---

## 三、把副作用的时机从一处挪到另一处

副作用（扣燃料、设冷却、记状态）在正常路径上两种实现结果相同，只有在**中断路径**上才有
差异。

### 已知案例

| 能力 | 原版 | Cubium |
|---|---|---|
| `brewing_tick` | 开始酿造时扣燃料 | 完成时才扣（可反复启停白嫖） |

设计单元时必须包含**中断/取消**类用例，否则这种差异不会被触发。

---

## 四、用例本身不够敏感

单元表写完之后测试「通过」了，但目标分支**根本没有被执行**。这类问题比实现错误更难发现，
因为测试信号是绿的。

### 已知案例

| 能力 | 问题 | 修正 |
|---|---|---|
| `anvil_durability` | `99\|50\|99\|50` 两种运算顺序的结果**都**被钳到 0 | 改用 `1561\|1500\|1561\|1400`，区分出 1152 vs 1159 |
| `furnace_tick` | 「点燃」单元的 `hasFuel` 字段位写成了 `false`，点燃分支一次都没跑到 | 修正字段位后得到 `lit=200, cook=1` |

### 规避方式

**跑完之后把每一条单元的实际输出逐条打出来，对照单元表注释核对它走了哪个分支。** 这一步
成本很低（一条命令），但能直接暴露「单元没生效」的问题。`brewing_tick` 之后的所有能力都
固定执行这一步。

---

## 五、只读源码片段导致误判

### 已知案例

| 能力 | 误判 |
|---|---|
| `structure_starts` | 只看了常量定义就认定种子写死，实际另有初始化逻辑 |
| `block_support` | 只看了扫描步长，漏掉了完整循环体 |
| `food_stats` | 以为 `addExhaustion` 的 40 上限是个缺陷，读完整个方法发现 Cubium 是对的 |

**必须读完整的方法体，而不是一屏片段。**

---

## 六、表示差异影响控制流

`nullptr` vs `Items.AIR` 这类表示差异不只是输出值不同，它会让**判空顺序**产生行为差异。

### 已知案例

`item_same_components`：第一版按原版写法写成 `!left.isEmpty() && ...`，对两个空堆在
Cubium 恒为 false、在原版恒为 true。必须**先判「两个都空」**。

---

## 七、语言层面的差异不应改动 Cubium

有些差异来自 Java 与 C++ 的语言语义，而不是实现缺陷。这类差异应登记为
`knownDifferences`，而不是去「修复」 Cubium。

### 已知案例

`mth_rounding`：Java 的 `(int)` 转换对超界值饱和到 `Integer.MAX_VALUE`，而 C++
的 `static_cast<i32>(double)` 对超界值是**未定义行为**。

判断标准：**如果 C++ 标准没有定义该行为，就不是 Cubium 的缺陷。**

---

## 八、构造器可能带归一化

对象的构造函数本身可能改变入参语义，所有经由它返回的值都会被改写。

### 已知案例

`aabb_ops`：原版 `AABB` 构造器执行 `min = Math.min(x1, x2)` / `max = Math.max(x1, x2)`。
因此 `intersect` 在盒子完全分离时产生的「负尺寸」结果会被翻成正尺寸——**「空盒」不可能
表示成 `min > max`**。若按 `min > max` 判空，会与其它 `intersects` 调用者的行为不一致。

正确做法是让所有返回该类型的函数都经过同一个复刻构造器行为的工厂函数。

---

## 九、错误路径也会被框架捕获

探针抛异常、oracle 崩溃、oracle 退出码非零，都会作为「错误」报出，而不是「差异」。这意味
着**两侧同时写错**时（例如单元格式解析都写错），第一轮信号是「oracle 运行失败」而不是
「差异」。

排查顺序应当是：先看是错误还是差异——错误优先查解析与输入构造，差异才查实现逻辑。

---

## 十、三张 CMake 列表必须同长同序

`tests/diff/CMakeLists.txt` 里有三张平行列表：

- `MC_DIFF_PROBE_SUFFIXES`
- `MC_DIFF_PROBE_SOURCES`
- `MC_DIFF_PROBE_DOMAINS`

**长度必须相同、顺序必须一一对应**。曾经发生过一次错位，导致 6 个能力链接到了别人的探针，
表现为「跑通了但结果莫名其妙」。现在配置期有一条长度断言兜底。

---

## 十一、新增能力前的固定检查清单

1. 先看 `tests/diff/README.md` 的状态表，确认该能力尚未存在；
2. 观察协作者工作区（`E:\dev\Cubium\branch-1`）的最新提交，避免撞车；
3. 读完原版**完整**方法体，不要只看片段；
4. 单元表写完后，逐条注明「预期的分支」；
5. 跑完后逐条打印实际输出，核对是否命中预期分支；
6. 检查是否涉及浮点中间值比较 → 若是，先考虑 `-ffast-math` 改写；
7. 检查是否有构造函数带归一化 → 若是，用工厂函数统一复刻；
8. 判断差异是「实现缺陷」还是「语言语义差异」→ 后者登记 `knownDifferences`。
## 十二、回调收到的是可变对象，不能存引用

原版的遍历/访问类 API（`BlockPos.breadthFirstTraversal`、`BlockPos.betweenClosed` 等）大量
复用**可变**的位置对象。若在回调里把接收到的对象直接存进列表：

```java
(pos, consumer) -> { order.add(pos); }   // 错：所有条目指向同一个可变实例
```

最终列表里每个元素的内容都会变成**最后一次写入**的坐标。即使调 `pos.immutable()` 也可能
无效——`pos` 传进回调时其内容已经可能与调用时的逻辑位置不一致。

**正确做法**：在**更早**的回调里取值（例如 `BlockPos.breadthFirstTraversal` 的 `nodeStatus`
在 `consumer` 之前调用），或立刻把内容复制成不可变值（`pos.asLong()`、新建常量对象）。

**这个坑在 `blockpos_bfs` 上实际发生过**：`count` 全部正确、`order` 却只记录到 1~7 个元素，
看起来像「遍历提前结束」，实际是引用复用。

---

---

## 十三、oracle 无法直接调用原版时的处理

极少数情况下，要测的原版逻辑既拿不到被调用的对象，也无法构造：

- 方法是**实例方法**，但类是**抽象类**或构造需要重型参数（`Level` 需要 `LevelData`、
  `DimensionType`、大量钩子）；
- `sun.misc.Unsafe.allocateInstance` 在 JDK 模块系统下被拒绝。

此时只能**逐字符复刻**原版方法体。必须遵守：

1. **读完整方法体**，不是片段。复刻的前提是「没有遗漏」，片段复刻必然出错（见第五条）。
2. **只复刻没有外部依赖的部分**。若方法体调用了其他原版方法，复刻它们的链条会迅速失控——
   这种情况应当**放弃该能力**，或改用集成测试。
3. **在文档里显著标注这一点**，并说明置信度边界：它验证的是「与抄下来的公式一致」，不是
   「与运行中的原版一致」。
4. **尽量借用原版的其他组件**。例如 `random_tick_pos` 复刻了 `getBlockRandomPos` 的方法体，
   但用原版 `BlockPos` 构造结果，至少能捕获坐标加法的错误。

**已用此方式的能力**：`random_tick_pos`（唯一的 4 行、零外部依赖的情况）。

---

## 十四、批量修改索引文件前必须先备份

`tests/diff/CMakeLists.txt` 与 `harnesses/index.ts` 是**所有能力的中枢索引**，一处写坏就会让
全部能力无法构建。

**已发生的事故**：注册 `bounding_box` 时，PowerShell 脚本里把
`[System.Collections.Generic.List[string]]` 写成了 `New-Object System.Collections.Generic.List[string]`，
导致后续 `Set-Content` 对着 `$null` 执行，**467 行的 `CMakeLists.txt` 被写成 0 字节**。

**必须遵守**：

1. 改这两个文件前确认 `git status` 是干净的，或先 `Copy-Item` 备份；
2. PowerShell 里用 `@()` 数组而不是泛型 `List`——`List` 的类型名写错会静默产生 `$null`，
   而 `$null | Set-Content` 会清空文件；
3. 修改后**立即校验**：
   - `CMakeLists.txt`：三张列表长度相同（注意 `SOURCES` 不含共享的 `runner_main.cpp`）
   - `index.ts`：`import` 数量与条目数量匹配
4. 一旦发现被清空，立刻 `git checkout -- <文件>` 恢复，再从干净状态重做，**不要在残缺
   结构上打补丁**。

这条与第十条（三张列表同长同序）配套：第十条讲「改对之后要验证」，本条讲「怎么改才安全」。

---

## 十五、查重前必须让状态表与探针列表对得上

**已发生的问题**：计划新增 `spawner_delay` 时，我先在 `README.md` 的状态表里搜这一行，没搜到，
据此判定「未覆盖」。实际它早在第 90 个能力时就已经实现并有文档，**我差点重复实现一遍**。

排查后发现状态表有 17 个能力缺行（`walk_path_type`、`aabb_ops`、`mth_rounding` 等），
另有过时的 `villager_*` 6 行指向不存在的探针。

**查重的正确方法**（不要只查 README）：

```powershell
# 1. 以 CMakeLists 的探针列表为唯一事实来源
$probes = @(); foreach ($ln in (Get-Content "tests/diff/CMakeLists.txt")) {
    if ($ln -match "^\s+probes/([a-z_0-9]+)\.cpp\s*$" -and $Matches[1] -ne "runner_main") {
        $probes += $Matches[1] } }
$probes.Count

# 2. 再查文件是否已存在（最直接的判据）
Test-Path "tests/diff/probes/<name>.cpp"
```

**新增能力后必须同步三处**，缺一处就会误导下一个人：

1. `probes/<name>.cpp` + `java/.../<Name>Oracle.java` + `harness/harnesses/<name>.ts`
2. 三张 CMake 列表（且同长同序）
3. `README.md` 的**状态表行** + 索引区的**文档链接**

**定期自检**：状态表的能力行数应等于探针数。不相等就说明有遗漏或冗余。

这条与第十条（三张列表同长同序）、第十四条（索引文件先备份）共同构成「新增能力的完整性
契约」。

---

## 十六、C++ 十六进制转义会贪婪吞掉后续字符

**已发生的问题**：`chat_formatting` 的用例要表达「`§` 后紧跟一个字母」，写成：

```cpp
"strip|\xc2\xa7ahello",   // 编译失败：hex escape sequence out of range
```

`\xa7a` 被 C++ 当成**一个**十六进制转义序列——转义会一直读到第一个非十六进制字符为止，
所以 `a` 被吞进去了。`0xa7a` 超出 `char` 范围，编译报错。

**正确写法**：用相邻字符串字面量拼接，把转义与后续字符隔开：

```cpp
"strip|\xc2\xa7" "ahello",
```

**适用范围**：只要十六进制转义后面紧跟 `0-9`、`a-f`、`A-F` 就会出现。UTF-8 编码的 `§`
（`\xc2\xa7`）后接颜色码 `0-9`/`a-f` 正好是最容易踩的组合。

**排查提示**：看到 `hex escape sequence out of range` 就去找十六进制转义后面的十六进制字符。

另注：`\xc2` 与 `\xa7` 之间不需要隔开——`\xc2` 之后的 `\` 不是十六进制字符，自然终止。

---

## 十七、合成测试数据必须自校验「它对应真实对象」

当探针用**构造的抽象属性**描述原版对象（而不是直接引用真实数据）时，oracle 需要把这些属性
映射回真实对象才能调用原版方法。**这段映射必须自校验**。

**已发生的问题**：`heightmap_types` 用 `<air>|<blocksMotion>|<hasFluid>|<isLeaves>` 四元组
描述方块。我假设「树叶不阻挡移动」，写了 `0001`（`motion=0`）。实际 `OAK_LEAVES` 的
`blocksMotion()` 返回 **`true`**。

若没有校验，两侧会**一致地**使用我编造的错误属性，测试「通过」但毫无意义——这是最危险的
假绿。

**做法**：在 oracle 的映射函数里，构造出真实对象后立刻回读它的全部相关属性，与输入四元组
逐项比对，不符就抛异常：

```java
BlockState state = block.defaultBlockState();
boolean actualMotion = state.blocksMotion();
boolean actualFluid  = !state.getFluidState().isEmpty();
boolean actualLeaves = block instanceof LeavesBlock;
if (actualAir != air || actualMotion != blocksMotion || ...) {
    throw new IllegalStateException("方块的属性与四元组不符：" + unit + " -> " + key);
}
```

**收益**：这条校验在第一次运行就报出了我的错误假设，并指明了不匹配的具体方块
（`minecraft:oak_leaves`）。

**适用场景**：任何「探针侧用简化描述、oracle 侧需还原为原版对象」的能力。若无法还原为真实
对象（原版根本没有这种组合），**应当删掉该用例**，而不是登记为「已知缺口」——测一个不存在的
情况没有意义。

这条与第五条（只读源码片段导致误判）、第九条（错误路径也会被框架捕获）互补：那两条讲「实现
错」，本条讲「测试数据本身错」。

---

## 十八、「实现完整但未接线」比完全没做更危险

**已发生的问题**：`fuel_values` 与 `recipe_book_category` 两个能力的探针、oracle、harness
全部齐全且已提交，`index.ts` 里也有 `xxxDeclaration()` 注册条目——但**三张 CMake 列表里都没有
它们**。

后果：

- 探针源码从未被编译过（可能有编译错误而无人知晓）
- 能力从不参与任何测试运行
- 状态表里也没有它们的行

**从代码看像是「已完成」，实际是「从未运行过」。**

**为什么危险**：完全没实现的能力，查重时一眼就能看出缺失；这种半成品反而会让人以为已经覆盖，
不去重复实现，也不会发现它跑不起来。

**检出方法**（应当成为常规自检）：

```powershell
# 找出有源码但不在 CMake 列表里的探针
$probes = @(); foreach ($ln in (Get-Content "tests/diff/CMakeLists.txt")) {
    if ($ln -match "^\s+probes/([a-z_0-9]+)\.cpp\s*$" -and $Matches[1] -ne "runner_main") {
        $probes += $Matches[1] } }
Get-ChildItem "tests/diff/probes" -Filter "*.cpp" |
    Where-Object { $probes -notcontains $_.BaseName -and $_.BaseName -ne "runner_main" } |
    ForEach-Object { "未注册: $($_.Name)" }
```

**结论**：`probes/` 目录下的每个 `.cpp`（除 `runner_main.cpp`）都必须出现在三张列表里。
这条与第十五条（状态表与探针列表对得上）合并成完整的三方一致性检查：

```text
probes/*.cpp  ⊆  CMake 三张列表  ⊆  index.ts 注册  ⊆  README 状态表
```

四个集合必须**完全相等**。