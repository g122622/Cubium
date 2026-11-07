# 差分测试：历史调查记录

> 本文件由 `tests/diff/README.md` 拆分而来，按时间顺序记录各轮排查过程与结论。
> 单能力的实现要点见同目录下以能力名命名的文件；跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

### 随机数：已对齐到哪一层

`random_sequence` 直接比对两个**内层**生成器（`Xoroshiro128ppRandom`、
`JavaLegacyRandom`）的接口序列，全部逐位一致。上层的 `WorldgenRandom`（原版把它包在
生成器外面、用自己的 `next(bits)` 重写所有抽取）现在全部路径都与原版逐位一致：

| 路径 | 状态 |
|---|---|
| `nextInt(bound)` / `nextFloat` / `nextDouble` | 与原版逐位一致（golden 测试通过） |
| 裸 `nextLong()` / `nextInt()` | 与原版逐位一致（`WorldgenRandomVanillaGoldenTest` 全绿） |

此前 `nextLong()` 与装饰种子存在逐位差异，根因有两处，均已修复：

1. `nextLong()` 把两个半部按 `u64` 拼接。原版是
   `((long) next(32) << 32) + next(32)`，而 `next(32)` 返回带符号 `int`，两半都要**符号扩展**。
2. `IRandom::nextBits` 对 `bits >= 32` 特判成截断取低位，与
   `BitRandomSource.next(bits) = (i32)(nextLong() >>> (64 - bits))` 的取高位语义相反。

`BlendedNoiseVanillaGoldenTest` / `DensityAstBaselineTest` 仍有约 1e-7 量级残余差异，
已与随机序列无关（修复前误差在 1e-6 量级且量级随种子放大），属噪声求值的浮点精度缺口，
待单独排查。


### 活塞推动结构：一次推动的纯计算

`piston_movement` 覆盖的是**一次推动的结构求解**：给定一小块方块台、活塞位置、朝向与是否伸出，
比较「能否推动」「移动链」「破坏链」。它调用 Cubium 的 `PistonStructureHelper`（只读世界）与原版
`PistonStructureResolver`，两侧读同一份方块描述字符串。

逐刻动画（`MovingPistonBlock` 推进、方块实体 tick、收回阶段）必须真实驱动世界，属于 world 型
能力，当前不可行，仍由 `block_behavior` 的 GameTest 覆盖。判据见 skill `mc-add-diff-test` 的
「world 型能力当前不可行」一节。

首轮 523 个场景中有 61 处差异，全部指向同一类问题——**方块的「推动反应」（pushReaction）没被读取**：

- **`Block::getPushReaction` 恒返回 `Normal`**：Cubium 把 pushReaction 登记在材质上
  （`Material::m_pushReaction`），但基类实现直接 `return Normal`，从不读材质。于是火把、树苗、
  铁轨等原版登记为 `DESTROY` 的方块，被活塞推着走而不是被破坏；
- **`Material::REPLACEABLE_PLANT` 漏登记 `Destroy`**：修好基类读取后，植物类材质本身的值仍是
  默认 `Normal`，树苗继续被推动；
- **水/岩浆登记成 `Block`**：原版流体是 `DESTROY`，活塞推到水应当把它破坏掉；登记成 `Block`
  会让活塞被水完全挡住。

三处修好后 523/523 全部一致。这个例子说明：**手写的材质表里漏一个字段，不会有任何编译期提示，
只会让游戏安静地按错误规则运行**。


### 玩家经验值：公式与状态机的两处错位

`experience` 覆盖三层语义：等级 ↔ 累计经验的换算公式、加点/扣点的状态机（原版
`Player#giveExperiencePoints` 的两段 while）、以及进度条这个 f32 字段的逐位比较。

首轮 455 个单元里有 333 处差异，收敛后全部一致。查出两个缺陷，都不是「公式抄错」，
而是**状态维护路径**出了问题：

- **`_validateState()` 越权重算等级**：它在末尾有 `while (progress >= 1) { progress -= 1; level++; }`，
  把进度当成「整级数量」扣除，而不是按容量折算。这个函数本应只做越界夹取，却作用在
  `addExperience` 刚算好的、已经收敛的状态上，把合法进度强行截断。后果很重：**加 100 点
  会得到 94 级，而原版是 7 级**；
- **`addLevels` 无条件重算累计经验**：方法的注释写着「正等级时不重新计算
  `totalExperience`」，下一段却无条件用 `getExperienceForLevel(level) + progress × capacity`
  重算。原版 `giveExperienceLevels` **从不**改写 `totalExperience`（它只由加点与归零分支
  改写），于是「只加减等级」也会把累计经验改成与玩家实际获得经验不符的值。

另外，`addExperience` 原本给负数数量单独写了一套「用等级起点 + 进度重算 `total`」的路径，
与正数分支的累加路径并存且互斥。现在两者合并为原版的单一路径：
`clamp(total + amount, 0, INT_MAX)`。

修复后 440/440 一致；`ExperienceManagerTest`、`VillagerTradeXpTest` 等 184 个单测全绿。


### 一个注意事项：单次加点量要落在合理范围

加点会**逐级**推进（每次 1 级），单次给出 `2147483647` 点会让两侧都陷入数千万次循环——
那是输入不合理，不是行为差异。场景里把单次加点上限设为 10000（覆盖击杀经验的量级）。


### 药水与酿造：名前缀缺失 + 初始化顺序

`potion` 比较 46 种药水的基础名、效果列表（两侧都先按效果 ID 排序——原版存的是 `List`，
顺序没有玩法语义）、是否含瞬时效果，以及三种前缀下的翻译键。首轮 18 处差异，全部是
**`baseName` 为空**：`Potions.cpp` 里短变体药水一律写成 `Potion("", {...})`，翻译键退化成
`".effect.minecraft."`。14 处显式空串 + `empty/water/mundane/thick/awkward` 五个无参构造，
补上自身 ID 后 46/46 一致。

`potion_brewing` 比较「药水 × 试剂」的笛卡尔积（874 组），每个单元回答三件事：试剂是不是
酿造材料、能否酿造、酿造出哪种药水。首轮 93 处差异，查出两个缺陷：

- **多出一条原版没有的规则**：`尴尬药水 + 发酵蛛眼 → 虚弱`。原版只有「水瓶 + 发酵蛛眼」，
  多出的这条让玩家能凭空调出虚弱药水；
- **整组起始酿造缺失**：原版的 `PotionBrewing.Builder#addStartMix` 会为每种材料同时注册
  `水 + 材料 → 平凡` 与 `尴尬 + 材料 → 目标`，共 5 组（微风棒 / 黏液块 / 石头 / 蛛网 /
  岩浆膏）。Cubium 只登记了岩浆膏那一组，蓄风、黏稠、盘丝、渗浆四种药水**完全酿不出来**。

补齐后仍剩 46 处（全部是 `breeze_rod`），根因埋在**初始化顺序**里：

> `Items::initialize()` 里 `PotionBrewing::initialize()` 排在 `_registerPotions()` 之后、
> `_registerTrialChamberItems()` 之前。而「微风棒」正是试炼密室物品，注册得比酿造配方晚
> 24 步——配方注册时 `Items::BREEZE_ROD` 还是 `nullptr`，`_addMix` 直接静默丢弃了这条规则。

把 `PotionBrewing::initialize()` 移到 `_registerTrialChamberItems()` 之后，874/874 一致。
这类缺陷没有任何编译期或运行期提示：只有配方表少了一条，微风棒静静地「不是酿造材料」。


### 方块支撑判定：一类「根本没写」的功能缺失

`block_support` 对 1166 个方块、每个配 23 种底座，比较「能否留在这个位置」
（原版 `BlockState#canSurvive` ↔ Cubium `Block::isValidPosition`）。

场景摆放有一个容易摆错的地方：附着方式有**两类**，底座必须同时放在正下方与四个水平
侧面。只放下方的话，墙上悬挂告示牌、墙面头颅、按钮、火把、灯笼、珊瑚这些侧挂方块
会整体被判成「不支持」——那是探针没摆对场景，不是实现差异。两侧必须用完全相同的摆法。

首轮 1166 个单元里 211 处差异。修掉三处后（按钮、压力板、苔藓材质）差异降到 218 处，
但统计口径给出了本轮最有用的结论：

- **在 12 种完整方块底座上，没有任何一个方块存在差异**——正常情况全部正确；
- 218 处差异**全部**落在台阶、栅栏、漏斗、箱子、地毯、耕地、水、岩浆、空气这些
  「不能支撑」的底座上。

也就是说，问题不是「判定算错」，而是**大量方块根本没有实现支撑判定**，继承了
`Block::isValidPosition` 的默认实现（永远返回 true）。这一点用单个方块的用例很难发现。

已修复的三处（都是「缺失」而非「算错」）：

- **按钮**：`AbstractButtonBlock` 有 `getAttachDirection` 等辅助方法，但从未覆写
  `isValidPosition`，按钮能悬浮在空气中。按原版
  `FaceAttachedHorizontalDirectionalBlock#canSurvive` 补上（`isFaceSturdy(FULL)`）；
- **压力板**：同样缺失。按原版 `BasePressurePlateBlock#canSurvive` 补上
  （`canSupportRigidBlock || canSupportCenter(UP)`）；
- **苔藓块材质**：`makeMossMaterial()` 写的是 `solid(false)`，而苔藓块与苍白苔藓块都是
  完整固体方块。这让 `BlockState::isSolid()` 为 false，所有用 `isSolid()` 判支撑的方块
  （站立告示牌等）都认为苔藓块撑不住——单这一项就贡献了 179 处差异；
- **站立告示牌**：判定写成 `isSolidSide(world, below, UP)`，比原版
  `getBlockState(below).isSolid()` 严格一档，玻璃、箱子这类「有碰撞但上表面不完整」的
  方块撑不住告示牌。

已确认但**未修**的全局议题：原版 `isSolid()` 由 `calculateSolid()` 从碰撞形状推导
（碰撞盒体积 ≥ 0.729 或 Y ≥ 1.0），Cubium 靠注册时手写 `notSolid()`（全项目 234 处）。
玻璃、树叶、冰这类「有整格碰撞却被标成非固体」的方块因此与原版不一致。修它要动全部
234 个调用点，属于独立议题。TODO: 对齐 `calculateSolid` 后把玻璃/树叶/冰加回底座清单。


### 第二轮：把 218 处差异压到 85

第一轮修完之后，差异的分布指向一个更具体的病灶——**`isSolidSide` 与 `isFaceSturdy`
被混用**。前者只判「是固体且有碰撞」，后者判「这一面在形状上铺满整格」。原版里需要
「面坚固」的地方几乎都用后者，Cubium 却常常用前者，于是任何侧面不满的方块
（箱子、漏斗）在所有方向上都被当成坚固。

这一轮修掉的六处：

- **箱子**：注册时带 `notSolid()`，但原版箱子的 `legacySolid` 为 true（forceSolidOn），
  它能撑住站立告示牌。去掉多余的 `notSolid()`；
- **红石火把 / 墙上红石火把 / 拉杆 / 钟**：都没有 `isValidPosition`，能悬浮在空中。
  按原版 `BaseTorchBlock`、`WallTorchBlock`、`FaceAttachedHorizontalDirectionalBlock`、
  `BellBlock` 的对应实现补上；
- **门**：下半部分支撑判定用 `isSolidSide(UP)`，原版是 `isFaceSturdy(UP)`；
- **珊瑚 / 珊瑚扇 / 墙上海扇**：三处 `canAttachTo` 都用了 `isSolidSide`，原版是
  `isFaceSturdy(FULL)`；
- **漏斗形状**：这是本轮最隐蔽的一处。原版顶部是
  `Shapes.join(column(16,10,16) ∪ column(8,4,10), column(12,11,16), ONLY_FIRST)`，
  `ONLY_FIRST` 即「a 减 b」，所以顶部不是实心 16×16，而是**外框 16×16、中间挖空
  12×12 的环形**。Cubium 写成了实心全宽，于是上表面被当成完整面——凡是「下方需坚固」
  的方块（门、蜡烛、花、海龟蛋）都能悬在漏斗上。单这一项贡献了 108 处误判。

顺带发现 `CollisionShape::combine` 只实现了 `OR`，`AND` / `NOT` 会**静默返回空形状**，
既不报错也无提示。已在源头补上 TODO；漏斗的环形改用四条边显式拼出，不依赖差集。


### 要塞：合并随机源后仍未命中，转向新增能力

上一轮记录的怀疑点是「`generateRingPositions` 内部的 rng 与外层 fork 用的 rng 不是同一个」。
原版确实在同一条流里边算起点边 fork：

```java
RandomSource randomsource = RandomSource.create();
randomsource.setSeed(concentricRingsSeed);
double d0 = randomsource.nextDouble() * PI * 2.0;
for (...) {
    double d1 = ... randomsource.nextDouble() ...;    // 半径扰动
    RandomSource randomsource1 = randomsource.fork(); // <- 同一个流上 fork
    d0 += (PI * 2) / k;
    if (换环) d0 += randomsource.nextDouble() * PI * 2.0;
}
```

已按此改造：`generateRingLayout` 在同一条随机流上取 fork 种子并随起点一起带出，
`StructurePlacementState` 直接用它构造搜索随机源。

**结果：仍是 47 处差异，要塞依然一个都不生成。**

到此为止，要塞问题已经排除了五个方向：

| 方向 | 结论 |
|---|---|
| 环容量递推 / 半径公式 | 正确（序列 3,6,10,15,21 正确，半径 107~1507 与原版量级一致） |
| `preferredBiomes` | 已修复（37 个，ID 空间一致且能命中） |
| 群系采样 `getNoiseBiome` | 已验证返回正确群系 |
| 扫描参数（方向 / 步长 / 边缘） | 已按 8 参数重载修正 |
| fork 随机源是否同一流 | 已合并，无变化 |

`structure_starts` 的要塞部分到此**暂停深挖**，改为记录状态、转向新增能力。理由：连续三轮
都在这一个能力上深挖，每轮都修掉真实缺陷，但差异数没有动过。持续新增能力才是这个任务的
本意，继续单点深挖的投入产出比已经不划算。

要塞仍然是一个明确的缺口，留给后续单独处理。

### 要塞搜索：三个参数猜错，逐一被源码否证

上一轮记下的 TODO 是「`k1` 的递增步长应为 `spread` 而非 1」。本轮逐参数对照原版
`BiomeSource.findBiomeHorizontal`，三处猜测逐一验证：

| 猜测 | 实际 | 结论 |
|---|---|---|
| 步长应为 `spread`（3） | 结构生成走**8 参数重载**，内部固定 `p_220565_ = 1` | **猜错**，改回 1 |
| 应从最大半径向内递减 | `j1 = findClosest ? 0 : k`，`findClosest = false` 时确实从 `k` 开始 | 已按此修正 |
| 只扫环边缘即可 | `if (findClosest) { ...只扫边缘... }`，`false` 时**扫满整个方形** | 已按此修正 |

三处修完后半径分布正确（107 ~ 1507 区块，与原版量级一致），坐标也从「几乎等于搜索
起点」变成有明显偏移。但 `(-524,-678)` 这个具体位置仍未命中——最近的候选是
`(-530,-410)` 与 `(-360,-655)`，同一象限、距离数百区块。

已确认不是以下原因：

- 环容量递推（按 `k += 2*k/(i1+1)` 复刻，序列 3,6,10,15,21,... 正确）
- 半径公式（`4*distance + distance*ringIndex*6 + ...`，最大半径 1507 与 128 个要塞匹配）
- `preferredBiomes`（37 个，已验证群系 ID 空间一致且能命中）
- 群系采样（`getNoiseBiome` 返回正确的群系）

剩下最可能的是**蓄水池抽样的随机数消耗时机**：原版对每个要塞 `fork()` 一次，
而 fork 的种子来自「此前所有起点已经消耗过随机数」的那个 `randomsource`。
我的复刻里 `generateRingPositions` 内部自己持有一个 rng（用于半径与角度扰动），
外面又新建了一个用于 fork，**两者不是同一个随机源**——这是一个明确的、可验证的怀疑点。

TODO: 把半径/角度扰动的 rng 与 fork 用的 rng 合并成同一个实例，让 fork 序列反映
起点生成过程中已经发生的消耗。

**教训（本轮第二次）**：上一轮我依据「原版有两个 `nextDouble` 调用」推断步长应为
spread，并写进了 TODO。实际那个参数由重载固定为 1，`spread` 在结构生成路径上根本不
参与。**读源码时推断出的结论，必须回到具体调用点确认参数值**，不能只看到形参名就下判断。

### 要塞：生物群系标签没加载 + 搜索太慢，两处基础设施缺陷

接着上一轮的要塞问题往下查，又发现两处**与算法无关**的基础设施缺陷。

#### 一、探针从未加载生物群系标签

`preferred_biomes` 引用 `#minecraft:stronghold_biased_to`（37 个群系），由
`StructureSetLoader` 去 `BiomeTags` 里查。探针的初始化序列里**没有**
`BiomeTagLoader::loadFromDataPackRepository`，于是：

```
[ss] strongholds preferredBiomes = 0    // 应为 37
```

`preferredBiomes` 恒为空 → `findBiomeHorizontal` 直接返回空 → 要塞永远退回原始环坐标。
这与上一轮修的环算法无关，是另一条独立的失效路径。

补上标签加载后 `preferredBiomes = 37`，并验证了后续链路可用：

```
[ss] 采样 quart(-33,0,-20) 群系 id = 4
[ss] 该群系是否在 preferred 中: 1        // 命中
[ss] preferred 前 5 个 id: 1 129 12 140 4
```

群系 ID 空间在两侧一致，`getNoiseBiome` 采样正常。

#### 二、128 次螺旋扫描被每个区块重跑一次

补上搜索后整个能力**跑不完**（1800 秒超时）。实测单个单元 2.3 秒——每个单元都要
跑 128 个要塞 × 29×29 个 quart 的螺旋扫描。

根因是缓存位置：原版把 `List<ChunkPos>` 缓存在 `ChunkGeneratorStructureState` 里，
**与放置规则同生命周期**；而 `StructurePlacementState` 是每个区块（探针里是每个单元）
新建的，只靠它的实例缓存等于没缓存。

改法：把改写后的最终坐标表回填到 `ConcentricRingsStructurePlacement` 上（按世界种子
缓存）。3 个单元从 6.9 秒降到 2.58 秒，542 个单元回到 170 秒量级。

#### 仍然存在的差距

要塞依然一个都不生成。已排除的原因：

- 环算法（上一轮已按原版修正三处）
- `preferredBiomes` 为空（本轮已修，37 个）
- 群系采样失效（已验证返回正确群系且命中偏好集）
- 搜索性能（已用缓存解决）

剩下的可能是**搜索结果的坐标与原版差一个数量级**：542 个候选单元来自原版 oracle
（原版能找到要塞的区块），如果我们的 `findBiomeHorizontal` 复刻有细节偏差（例如扫描
顺序、蓄水池抽样的调用时机），算出的坐标就会系统性偏离，从而全部错过这批候选。

TODO: 逐参数对照原版 `findBiomeHorizontal` 的扫描顺序——特别是 `k1` 的递增步长
（原版是 `k1 += spread`，而非 1）与内层 `l1`/`i2` 的起始值。

### 要塞环算法：三处偏差已修，生物群系搜索仍缺失

`structure_starts` 恢复运行后，47 处差异里最大的一块是**要塞**：9 个单元在原版有、
Cubium 完全没有。追下去发现 `ConcentricRingsStructurePlacement` 的环位置生成与原版
（`ChunkGeneratorStructureState.generateRingPositions`）有三处实质偏差：

| 项目 | 原版 | 修复前 | 状态 |
|---|---|---|---|
| 半径公式 | `4*distance + distance*ringIndex*6 + ...` | `distance + distance*ringIndex*6 + ...`（少了开头的 4 倍） | 已修 |
| 环容量递推 | `k += 2*k/(ringIndex+1)`，动态扩张 | 硬编码表 `3,6,10,15,21,28,36,9` | 已修 |
| 环间角度扰动 | 每换环再 `angle += random*2π` | 无 | 已修 |
| 生物群系搜索 | `findBiomeHorizontal(..., 112, ...)` 把要塞挪到 `#stronghold_biased_to` | 无 | **未修** |

三处都修完之后要塞仍然一个都不生成，说明**最后一项才是决定性的**：原版算出的环坐标
只是搜索起点，真正的要塞位置由半径 112 区块的生物群系搜索决定。

#### 一次被数据否定的猜想

读原版源码时发现 `createForNormal` 传入的 `concentricRingsSeed` 是**常量 0L** 而不是
世界种子，我据此判断「要塞分布与种子无关」，改成了常量并重新测——差异数纹丝不动。

然后我用 oracle 的输出证伪了这个猜想：

```
种子 0              : (-524,-678) (-573,1114) (1113,567) (612,910)
种子 9248738493917  : (-295,91) (-705,79) (1005,747) (174,-1053) (763,-1043) ...
```

两个种子的要塞位置**完全不重合**。如果种子恒为 0 且没有生物群系搜索，两者必须落在
同一批坐标上。所以常量 0 只用于生成「搜索起点」，最终坐标由生物群系搜索（其 fork
随机源来自世界种子）改写。改动已回退。

**教训**：只读源码得出的结论必须用数据验证。这次如果我停在「已按源码改成常量」，
就会写下一段看起来有依据、实际上使行为变差的改动。

TODO: 补齐生物群系搜索需要把 `IBiomeSource` 传进 `StructurePlacementState::ringPositions`，
算出每个环坐标后复刻 `findBiomeHorizontal(半径 112, preferredBiomes, fork 随机源)` 并改写
坐标。这是要塞对齐的最后一块拼图。

#### 其余差异

47 处差异的构成：要塞 9 处（完全缺失）、矿井 13 处（片段数普遍只有原版的 1/3~1/10）、
试炼密室 5 处、前哨站 / 海底废墟等各若干处。矿井片段数偏少是下一个独立问题。

### 一次静默失效：CMake 三张列表错位让 6 个能力链接到了别的探针

`structure_starts` 长期报 `key 'origin' not found`，看起来像是探针自身的缺陷。实际根因
在 `tests/diff/CMakeLists.txt`：

三张列表按**相同索引**配对，构建时用 `list(GET ...)` 取同一位置的 suffix 与源文件：

```cmake
list(GET MC_DIFF_PROBE_SUFFIXES ${index} targetSuffix)
```

历史上有一次提交在追加新能力时把 `structure_density` / `structure_starts` /
`villager_trades` 三项插在了 `walk_path_type` **前面**（SUFFIXES 里在**后面**），
于是从第 37 项起两者错开 3 位：

| 索引 | SUFFIXES（命名） | SOURCES（实际编译） |
|---|---|---|
| 37 | `walk_path_type` | `structure_density` |
| 38 | `path_finder` | `structure_starts` |
| 39 | `fly_path_finder` | `villager_trades` |
| 40 | `structure_density` | `walk_path_type` |
| 41 | `structure_starts` | `path_finder` |
| 42 | `villager_trades` | `fly_path_finder` |

后果是这 6 个可执行文件**各自链接了别人的探针源文件**。错位不会产生任何编译错误或
警告，只表现为「某个能力报了一个与它无关的错误」。`structure_starts` 之所以报
`origin`，是因为它当时跑的是别的探针的代码。

对齐后 6 项全部恢复正常：`structure_starts` 从「完全无法运行」变成 542 个单元、47 处差异；
`walk_path_type` / `path_finder` / `fly_path_finder` 各 282/282 全部一致。

**教训**：这组列表的正确性完全依赖三张列表的**顺序**，靠肉眼维护。新增能力时若只往
SUFFIXES 和 DOMAINS 尾部追加、却把源文件插到别处，就会静默错位。
`MC_DIFF_PROBE_COUNT` 只校验了长度相同，没有校验顺序对应关系。
TODO: 在 CMake 里加一条配置期断言，逐索引比对 `MC_DIFF_PROBE_SUFFIXES[i]` 与
`MC_DIFF_PROBE_SOURCES[i]` 的文件名主干是否一致，把这类错位在配置阶段就拦下来。

### 乐器：浮点字段与两个派生值，8/8 全一致

`instrument` 比对 8 种山羊角音色：音效 ID、吹奏时长、可听距离、描述翻译键，外加两个
**派生值**——`useDurationTicks` 与 `volume`。一次通过 **8/8**。

原版 `record Instrument(Holder<SoundEvent> soundEvent, float useDuration, float range,
Component description)` 只有四个字段，派生值不在记录里，但在 `InstrumentItem` 里各占
一行换算：

```java
getUseDuration -> Mth.floor(useDuration * 20.0F)
play           -> range / 16.0F
```

Cubium 侧同样实现了这两个派生（`useDurationTicks()` / `volume()`），把它们也纳入比对，
是因为**换算口径本身会错**：秒与 tick 的换算、范围与音量的比例，这两处一旦写错，
只比原始字段是发现不了的。

#### 浮点输出口径

三个浮点字段（`useDuration` / `range` / `volume`）在两侧都按「6 位有效数字，去掉尾随零」
格式化成**字符串**输出。这是 `food_properties` 与 `jukebox_song` 两个能力踩过同一个坑之后
定下的规矩：C++ 的 `%.6g` 与 Java 的 `%g` 行为不同——`178.0f` 一边给 `178`，另一边给
`178.000`。若直接按数字输出，Gson 与 nlohmann 的打印差异还会再叠一层
（`0.3` vs `0.30000001192092896`）。

#### 一个初始化顺序的坑

探针首轮 8 个单元全部返回 `missing`，注册表为空。原因是服务端 `RegistryBootstrap` 里
这个注册表有三步：

```
clear()                     -> 清掉可能残留的旧条目
initialize()                -> 登记写死的内置条目（8 种山羊角）
loadFromDataPackRepository  -> 用数据包覆盖同名条目
```

探针最初只做了第三步。数据包里的条目其实能注册成功，但 `get()` 走的是另一条路径，
表现为「找不到」。补齐前三步的顺序后立刻 8/8。

这提示了一个更一般的规律：**差分能力复刻的是原版的运行时状态，不是原版的输入数据**。
凡是注册表，都要照抄服务端的初始化顺序，而不是只加载数据包。### 动物变体：六类变体的出生条件，32/32 一次收敛

`animal_variant` 比对狼 / 猫 / 蛙 / 牛 / 鸡 / 猪六类变体，共 32 个单元。首轮 32 处全差，
排查后收敛到 **32/32 全一致**。

六类变体此前在 Cubium 中都没有实现，变体的贴图与出生条件全都无处可查。

#### 数据形状

六类的形状高度一致，因此共用一套加载器（`AnimalVariants`），差异由 `AssetField` 承载：

| 类别 | 贴图字段 | `model` 字段 | 数量 |
|---|---|---|---|
| `wolf_variant` | `assets: { wild, tame, angry }`（三张） | 无 | 9 |
| `cat_variant` / `frog_variant` | `asset_id`（一张） | 无 | 11 / 3 |
| `cow_variant` / `chicken_variant` / `pig_variant` | `asset_id`（一张） | 有，缺省 `normal` | 各 3 |

出生条件是 `spawn_conditions: [{ condition, priority }]`，条件只有三种：

- `biome` → `BiomeCheck(HolderSet<Biome>)`，字段 `biomes`
- `structure` → `StructureCheck(HolderSet<Structure>)`，字段 `structures`
- `moon_brightness` → `MoonBrightnessCheck(MinMaxBounds.Doubles)`，字段 `range: { min, max }`

#### 首轮全差的三个根因

| 现象 | 根因 |
|---|---|
| `entries` 全为空、`rangeMin` 全丢失 | 数据文件里 `type` 写作 `minecraft:structure`，而原版注册名是 `structure`。字符串比较不相等，整个条件解析分支被跳过 |
| 贴图 ID 两侧对不上 | 原版 `ClientAsset.ResourceTexture` 的构造器把 `asset_id` 变换成 `textures/` + path + `.png`。数据文件写 `entity/cat/all_black`，原版暴露 `textures/entity/cat/all_black.png` |
| `model` 为空 vs `normal` | 原版是 `optionalFieldOf("model", ModelType.NORMAL)`，缺省取 `normal`。但只有牛 / 鸡 / 猪三类有 model 概念，狼 / 猫 / 蛙必须保持空串 |

第一条尤其典型：`type` 带不带命名空间前缀是**数据文件写法**与**注册表 key** 的差异，
单看任何一侧都发现不了，只有两侧对着比才会暴露。

#### 两处刻意的口径

- **`entries` 保留 `#` 前缀**：原版 `HolderSet` 的标签引用与内联元素语义不等价
  （标签会随数据包内容变化），比对必须能区分。
- **狼的 `assets` 按 `AssetInfo(wild, tame, angry)` 的字段顺序输出**，而不是按 JSON 对象的
  遍历顺序；`selectors` 也保持数组原顺序——优先级链的书写顺序有语义。

### 盔甲纹饰：两类数据包注册表，一次通过

`armor_trim` 一次性比对 18 个纹饰图案与 11 种纹饰材料，**29/29 全一致**。

这个能力顺带补上了 Cubium 的一处空白：此前**完全没有** `trim_pattern` /
`trim_material` 这两层注册表——`ArmorTrim` 只以物品组件和数据组件类型枚举的形式存在，
图案的贴图 ID、描述、是否贴花，材料的贴图后缀、描述颜色、覆盖装备材质表全都无处可查。
本次新增 `TrimPattern`/`TrimPatterns`、`TrimMaterial`/`TrimMaterials` 两对类，
按 `PaintingVariants` 的既有模式从数据包加载。

比对项与原版结构的对应关系：

| 单元 | 比什么 | 对应原版结构 |
|---|---|---|
| `pattern/<id>` | 贴图 ID / 描述翻译键 / 是否贴花 | `record TrimPattern(Identifier assetId, Component description, boolean decal)` |
| `material/<id>` | 贴图后缀 / 描述翻译键 / 描述颜色 / 覆盖装备材质表 | `record TrimMaterial(MaterialAssetGroup assets, Component description)` |

两处沿用了此前踩过的坑的结论：

- **描述比翻译键而非渲染文本**（`TranslatableContents.getKey()`），这是唱片机歌曲能力里
  踩过 `getString()` 返回已渲染文本的坑之后定下的口径。
- **`overrides` 是无序表**（原版是 `Map<ResourceKey<EquipmentAsset>, AssetInfo>`），
  两侧都按装备材质键排序后输出，并额外用 `unorderedArrays` 声明一次，让比较器不依赖顺序。

`asset_name` + `override_armor_assets` 对应原版 `MaterialAssetGroup` 的
`base`（`AssetInfo`）与 `overrides`；`override_armor_assets` 在数据文件里是可缺省字段，
缺省即空表，与 `optionalFieldOf(..., Map.of())` 一致。

### 附魔适用性：枚举归纳 vs 标签判定（28 → 0）

`enchantment_applicability` 对每个附魔逐个物品问「能不能附上去」，比的是**最终物品集合**。
首次运行暴露出 **28 个附魔与原版不符**，两轮迭代后收敛到 **43/43 全绿**。

根因在表示方式：原版直接读数据包的 `supported_items` 物品标签引用（如
`#minecraft:enchantable/sharp_weapon`）；Cubium 的附魔定义是硬编码的，没有这个字段，
改用 `EnchantmentType` 枚举**归纳**原版标签，再由 `canApplyToItem` 按物品类型判定。
枚举归纳得粗糙，差异就藏在这一层。

#### 最隐蔽的一类错误：继承链假设错了

```cpp
const auto* toolItem = dynamic_cast<const item::tool::ToolItem*>(item);
return toolItem != nullptr && toolItem->getToolType() == item::tool::ToolType::Sword;
```

这段代码**看上去完全正确**，注释也写着「剑」。但：

| 物品类 | 实际继承 | 后果 |
|---|---|---|
| `SwordItem` | `TieredItem`（**不继承 `ToolItem`**） | `getToolType() == Sword` 永远不成立，**锋利附不到任何剑上** |
| `MaceItem` | `Item`（不继承 `ToolItem`） | 重锤附不了锋利、亡灵杀手、节肢杀手 |
| `ShearsItem` | `Item`（不继承 `ToolItem`） | 效率附不到剪刀上 |
| `SpearItem` | `tool::TieredItem`（独立类） | 矛被整体漏掉 |

同一模式的错误出现了**四次**。前三次是逐个差分发现的，第四次是在修完前三个之后才想到
「可能还有别的类也这样」。没有差分，这类问题几乎不可能靠读代码发现——代码没有语法错误，
逻辑看起来也自洽，只是前提假设错了。

#### 其余修复

| 问题 | 表现 | 修法 |
|---|---|---|
| `Breakable`/`Vanishable` 直接 `return true` | 经验修补、耐久能附到泥土、空气、刷怪蛋（各 1124 个物品） | 改用 `isDamageable()`，补上指南针、雕刻南瓜、头颅 |
| 四个护甲部位分支全都 `return item->isArmor()` | 水下速掘/呼吸能附在靴上，深海漫游/冰霜行者能附在头盔上 | 按 `isHelmet`/`isChestplate`/`isLeggings`/`isBoots` 分部位 |
| `isArmor()` 把动物铠甲算进护甲 | `wolf_armor` 被误判可附保护、耐久、经验修补、消失诅咒 | 护甲只认玩家四部件；`durability` 类排除动物铠甲 |
| 六个武器附魔共用一个 `Weapon` 类型 | 斧/重锤/矛各自错配 | 按原版标签拆成 `Weapon`/`SharpWeapon`/`MeleeWeapon`/`FireAspectWeapon`/`SweepingWeapon`/`LungeWeapon` |
| 时运、精准采集与效率共用 `Digger` | 剪刀被误判可附时运 | 新增 `DiggerLoot`（`#enchantable/mining_loot`，不含剪刀） |

#### 一处连带收益

修完这些之后，卡了好几轮的存量失败：

```
ContainerRequestTest.EnchantmentButtonRejectsInsufficientLevelAndCreativeDoesNotConsumeLapis
```

**自己消失了**。附魔台按钮能否点击，正是由「这个物品能不能附这个魔」推导出来的，
枚举判定错误一直在污染下游行为。我前几轮把它当成「无关存量问题」忽略，那个判断是错的。

### 附魔代价曲线：43 个附魔逐级比对，全对

`enchantment_cost` 比较 43 个附魔的最大等级、铁砧代价，以及**每一级**的附魔台等级
区间（`minCost` / `maxCost`）。一次通过 **43/43**。

这条曲线来自数据包的 `min_cost` / `max_cost`：

```json
"min_cost": { "base": 1,  "per_level_above_first": 11 }
```

即第 L 级（从 1 起）的代价是 `base + per_level_above_first * (L - 1)`。Cubium 的
`Enchantment::getMinCost(level)` / `getMaxCost(level)` 与之逐级吻合，包括
`sharpness` 这种 5 级附魔的完整 5 个采样点。

附魔台"刷不刷得出"、铁砧"能不能合"全由这几个数决定，写错一处就会让某些附魔在特定
等级下永远刷不出来——而且不会报错。此前没有任何自动化覆盖。

比对时**按等级顺序**取值（数组有序，不能声明为无序）：代价曲线的形状本身就是要验证
的东西，排序会把它抹平。


### 伤害类型标签：一次「本来就对」的验证

`damage_type_tags` 比较 33 个伤害类型标签各自包含哪些伤害类型，一次通过 **33/33**。

这次的结论是「Cubium 本来就对」——`DamageTypeTags` 与 `DamageTypeTagLoader` 早就存在，
加载流程也与 `EntityTypeTagLoader` 同构（`initialize()` 建对象、加载器填成员）。
能力本身只做了两件事：把标签纳入差分校验范围，以及确认成员集合与数据包一致。

值得记的是它的价值不在于「发现缺陷」，而在于**把一块原本没人验证的区域变成有护栏的**。
这批标签几乎全是玩法开关：

| 标签 | 影响 |
|---|---|
| `bypasses_armor` | 能否穿甲 |
| `is_fall` | 落地保护、摔落靴判定 |
| `burns_armor_stands` | 会不会烧掉装备架 |
| `avoids_guardian_thorns` | 是否触发守卫者荆棘 |

漏一个成员就有一类伤害行为走偏，而且不会有任何东西报错。之前它们没有任何自动化覆盖。

顺带记一个 API 细节：原版 `TagKey` 用 `location()`，而 `ResourceKey` 用 `identifier()`，
两者在同一个方法体里一起出现时很容易写混（这次就写错了一次，javac 直接报出来）。


### 画变体：纯数据包注册表，加载路径踩了两个坑

Cubium 原先完全没有画变体这一层（`painting_variant` 只在物品组件里作为一个类型枚举出现），
画的尺寸、贴图、标题、作者全都无处可查。本能力为此新建了 `PaintingVariants` 注册表，
从数据包 `painting_variant/` 读取全部 51 种，结果 **51/51 一致**。

加载器本身不难，但踩了两个坑，都属于「路径拼错就静默零结果」：

1. **`listResourceStacks` 的目录参数必须带命名空间前缀**。我一开始传的是
   `"painting_variant"`，一个文件都列不到；正确写法是遍历
   `getResourceNamespaces()`，对每个命名空间传 `"<ns>/painting_variant"`。
   参考 `GenericTagLoader` 才发现的。

2. **`ResourceVersion` 是结构体而非字符串**。它是
   `{ packName, content }`，`nlohmann::json::parse` 不能直接吃它，要取
   `.content`；多数据包叠加时优先级最高的是**末尾**那个（`versions.back()`，
   与 `GenericTagLoader` 的 `rbegin()` 一致）。

两个坑的共同表现都是「注册表为空 → 探针全部 missing → 51 处差异」，而不是报错。
这类静默失败在差分能力里很容易被误读成「数据全错」。


### 附魔标签：补上了一个结构性缺口

`entity_type_tags` 那次是「探针漏了初始化」的假缺陷，`enchantment_tags` 这次则是**真的缺东西**：
Cubium 原先只用 `Enchantment::isTreasure()` / `isCurse()` 两个硬编码虚函数表达标签语义，
等于把数据包里的 22 个标签压成了 2 个开关。其余 20 个——附魔台能不能刷出
（`in_enchanting_table`）、能不能交易（`tradeable`）、战利品与生物装备各自允许哪些、
`exclusive_set/*` 的互斥组——都没有对应物。

本能力为此新建了 `EnchantmentTags` 注册表，直接复用通用的两阶段加载器
（`GenericTagLoader`），后者已经处理好了 `#` 引用的递归展开与循环依赖检测：

```
double_trade_price.json = { "values": ["#minecraft:treasure"] }
```

补完之后 **22/22 一致**，一个成员都没错。

过程中确认了一件值得记的事：`exclusive_set/` 是**子目录**，第一次用
`Get-ChildItem -Filter *.json` 只列了顶层 15 个文件，误以为全集就是 15 个。
`GenericTagLoader` 是递归扫描的，那 7 个互斥组标签被自动发现并创建——
这反过来验证了「加载器遇到未预注册的标签要按需 `registerTag`」这条改动是必要的：
没有它，这 7 个标签即使文件在也永远是 `missing`。

TODO: `tooltip_order` 的**顺序**语义当前没有承载（成员集是无序 set）。它决定提示框里
附魔的排列次序，与其它标签的集合语义不同，若后续要用到需另存一份有序数组。


### 唱片机歌曲：两处「比较口径」而非数据缺陷

`jukebox_song` 比较 21 首歌的声音事件、描述翻译键、时长、比较器输出。首轮 21/21
全差异，但**没有一处是数据错的**，全是比较口径没对齐：

1. **描述该取翻译键而不是渲染结果**。我最初用 `Component.getString()`，它会把组件
   解析成当前语言下的显示文本（`"C418 - 11"`），而 Cubium 存的是原始键
   `jukebox_song.minecraft.11`。本能力比的是数据本身，不是渲染结果，改为取
   `TranslatableContents.getKey()`。

2. **`%g` 在两个语言里的尾零处理不同**。同一个 `178.0f`，C++ 的 `%.6g` 打成 `"178"`，
   Java 的却打成 `"178.000"`。统一为「6 位有效数字再去掉尾随零」，即 Java
   `Float.toString` 的口径。这个坑在 `food_properties` 里已经踩过一次，这里是同一个
   问题换了个字段——凡是比较 f32 的地方都得留意。


### 方块支撑：用「探针直接打印」代替几何推理

`block_support` 从 218 处一路收敛，其中台阶侧面那 54 处卡了很久：我反复读源码推导
`isFaceSturdy` 该不该为真，始终不得要领。转折点是**放弃推理、让探针把判定矩阵打出来**——
把同一批方块摆到 `facing=north` 与 `facing=south` 两种台阶上，一眼就能看出是哪一侧判反了。

这一步之后连查出两个独立的方位错误：

- **楼梯形状方位整体错开**。`SHAPE_INDEX_MAP` 的 STRAIGHT 段是 `{12, 5, 3, 10}`，
  而原版 `SHAPE_BOTTOM_STRAIGHT` 在 NORTH 下是北半侧。方位错开后北侧面只剩半格，
  贴上去的墙火把、墙上海扇、墙面头颅全被判为无法存活。
- **按钮附着方向反了**。`canSurvive` 是 `canAttach(world, pos, getConnectedDirection(state).getOpposite())`，
  Cubium 少做一次取反，`facing=north` 的按钮去问了北边的方块，而它实际贴在南边。

两处修完 75 → 61。值得记的是第二个错误**只有在第一个修好之后才浮出来**：台阶两侧
坚固性不同，方向一反就对不上，而先前台阶形状本身就错，掩盖了它。


### 已定位但未修的根因：VoxelShape 的坐标压缩方式与原版不同

`block_support` 剩下的主要差异（箱子 42、下半砖 41、耕地 40、漏斗 39、地毯 38）
指向同一个底层问题。

原版 `VoxelShape` 的坐标集来自**形状实际用到的边界**并补上 padding：

```
箱子 column(14,0,14) 的 Y 坐标集 = [0, 0.875, 1.0]（尺寸 2）
findIndex(Y, 0.9999999) -> 1
SliceShape 取 [p_83179_, p_83179_+1] = [0.875, 1.0]
```

Cubium 的 `VoxelShape(std::shared_ptr<DiscreteVoxelShape>)` 把坐标点按 `i / size`
**均匀分布**，同一个箱子得到 14 个 Y 层，`findIndex` 返回 13，切片变成第 13 层。

层数不同导致 `getFaceShape` 取到的切片范围不同，进而 `SupportType.CENTER`
（`Block.column(2.0, 0.0, 10.0)`）是否被完全包含的结论也不同。这解释了为什么灯笼、
紫水晶簇这类「只需中心一小块坚固」的方块会在箱子、耕地、半砖上判错，而完整方块上正常。

TODO: 让 `VoxelShape` 的坐标集改用「实际边界 + padding」而非均匀分布。这是
`VoxelShape` 的根基改动，影响光照遮挡、形状布尔运算等所有使用方，需要独立评估与
全量回归，不适合夹在差分能力的迭代里做。


### 实体类型标签：一次「假缺陷」

`entity_type_tags` 比较 46 个实体类型标签各自包含哪些实体。第二轮迭代后才全绿，中间
那版曾得出一个**错误结论**，值得记下来。

首版 14 处差异全部是「成员缺失」，缺了 13 个标签（`boat`、`can_equip_saddle`、
`followable_friendly_mobs`、`illager_friends`、`non_controlling_rider`…），看起来
就是 Cubium 少声明了一大批标签。我一度据此写下「补齐这 13 个标签」的 TODO。

真正的根因在**探针自己身上**：它只调了 `EntityTypeTags::initialize()`，而这个函数
只负责把内建的若干标签对象建出来；**成员集来自数据包文件**
（`data/minecraft/tags/entity_type/*.json`）。服务端启动时 `RegistryBootstrap`
会在 `initialize()` 之后接着调 `EntityTypeTagLoader`——探针漏了第二步。

于是同一个现象有两层成因：

- 数据包里的 46 个文件都在，但没被加载；
- 即便加载，`resolveEntityTypeTagWithDependencies` 里是
  `auto* tag = getTag(location); if (tag != nullptr) {...}`——**找不到就静默跳过**，
  不会按需创建。这条在加载器里也补了（改为 `getTag` 空则 `registerTag`），
  属于真实健壮性缺陷，但不是本次差异的主因。

补上数据包加载后 46/46 一致，13 个标签一个不少。

教训：**先确认探针和被测系统走的是同一条初始化路径**，再谈「被测系统有缺陷」。
这条能力里，探针漏一步初始化就会凭空造出 13 个「缺陷」。

另一个独立踩坑：全集**不能**用 oracle 的 `list` 枚举。原版
`BuiltInRegistries.ENTITY_TYPE.getTags()` 在只 Bootstrap、未加载数据包时只返回 4 个
内联标签，拿它当全集会让能力只覆盖那一小撮（表现为「4 个单元全部差异」）。
本能力在 TS 侧显式列出原版的 46 个标签。


### 食物属性：同一个量放在了两套字段语义里

`food_properties` 比较 44 种食物物品的饥饿值、饱和度修正、能否饱腹后继续吃。全绿，
但过程里踩了三个坑，都属于**比较口径**而非实现缺陷：

1. **字段语义不同**。原版 `FoodProperties.saturation` 存的是**已经换算过的饱和值**
   （1.21 组件化后直接写死乘积），Cubium 的 `Food::getSaturationModifier()` 存的是
   **修正系数**，换算发生在 `FoodStats::addStats`（`food × modifier × 2`，与原版
   `FoodConstants.saturationByModifier` 一致）。两侧「吃下去会怎样」完全相同，只是把
   同一个量放在了不同字段里。在探针侧正算 `nutrition × modifier × 2` 会与两侧写死的
   f32 常量差最后一个 ulp（`2.4f` vs `2.4000001f`），最终改为**直接比系数**。

2. **f32 的打印精度**。同一个 `0.3f`，Java 经 Gson 输出 `0.3`，C++ 经 nlohmann 输出
   `0.30000001192092896`。统一按 6 位有效数字输出。

3. **`%.6g` 在两侧语义不同**。C++ 的 `%.6g` 把 0.3 打成 `"0.3"`，Java 的却打成
   `"0.300000"`。统一为「6 位有效数字再去掉尾随零」，即 Java `Float.toString` 的口径。

经验：**先确认两侧比较的是同一个量**，再谈数值相等。这三步里前两步如果没拆开排查，
很容易误判成「Cubium 食物数据全错」。


### 一个经验：先用「完整方块底座」把真实缺陷筛出来

218 处差异里，只有 12 个方块在完整的固体底座上也出错，其余 188 个只在
台阶、栅栏、漏斗、箱子这类「不能支撑」的底座上出错。这个切分很有用：它把
**「判定算错」**与**「根本没实现判定」**分开了。前者要读逻辑，后者只要照原版补方法。


### 第三轮：方向传反（85 → 75）

珊瑚的 `canAttachTo(world, pos, direction)` 里，`direction` 是**从珊瑚指向底座**的
方向（下方珊瑚传 `Down`）。但检查时直接把 `direction` 当成了「底座朝向珊瑚的面」，
于是实际查的是底座的**下表面**。后果是上下半砖的结果完全颠倒：

| 底座 | 原版 | 修复前 |
|---|---|---|
| `oak_slab[type=top]` | 撑得住（真） | 撑不住 |
| `oak_slab[type=bottom]` | 撑不住 | 撑得住 |

改成 `Directions::opposite(direction)` 后 85 → 75。这类「方向取反」的错很难靠读代码
发现——两个方向都"看起来合理"，只有把上下半砖这种**镜像底座**摆进用例才暴露。

剩余 75 处集中在台阶侧面（54）与箱子/漏斗（42/39，彼此有重叠）。已确认其中一部分是
`CollisionShape::getFaceShape` 的问题：它对「触碰该面」的判定用 `maxCoord ≈ 1.0`，
而原版会把触碰面的盒子**投影成满格**再判完整。台阶这类分段拼接的形状因此判错。
TODO: 对齐 `getFaceShape` 的投影语义后继续收敛。

## 下一步候选能力

按「只读、无状态、易对齐」排序：

1. **方块状态 → NBT / 数字 ID 编解码** —— 直接落在存档与网络协议的 1:1 对齐上；
2. **命令解析树** —— 从原版导出 command tree（字面量/参数/重定向），与 `/help` 看到的
   树逐节点比对；`command_argument` 已经把参数层对齐了；
3. **实体 / 附魔缺失项** —— 补齐 `KNOWN_MISSING` 里那 3 个附魔与 32 个实体类型，
   清单随之删条目。

## 结构起点：已查出的问题

`structure_starts` 首轮跑通就查出并修掉的结构生成缺陷（每一条都对着原版源码核实过）：

- **结构集条目抽签不是加权轮盘**：原版 `StructureTemplatePool.getRandomTemplate` 是在
  **按权重逐份展开**的列表上取下标，Cubium 原来用的是"随机数落在哪个权重区间"的另一种
  抽法——起始块因此选错（村庄整座偏移十几格）。现已按下标语义重写；
- **单条目结构集不得消耗抽签随机数**：原版 `list.size() == 1` 时直接生成，Cubium 原来
  一律走抽签，多消耗一次随机数，末地城等结构随之整体错位；
- **村庄不该有专用装配流程**：村庄的结构定义 JSON 与拼图结构完全同构，原版也走同一条
  `JigsawPlacement.addPieces`（含 `start_jigsaw_name` 起始连接点选择）。Cubium 单写的
  `VillageStructure` 先抽旋转再抽起始块、且不按名字过滤起始块，起始块与装配走向都错。
  该类已删除，村庄改走通用 `JigsawStructure`；
- **拼图结构的生物群系校验要用候选生成点**：原版在 `GenerationStub` 的位置（起始块中心、
  投影后的地面线）采样，Cubium 在区块中心 + y=0 采样，群系边界处会把合法结构整片误杀
  （古代城市首轮被误杀就是这个原因）。裸 `JigsawStructure` / 试炼密室 / 前哨站现均按
  候选点校验，且校验位于装配之后（与原版 `findValidGenerationPoint` 的调用位置一致）；
- **掠夺者前哨站的起始高度**：原版是 `start_height: absolute 0` + `project_start_to_heightmap:
  WORLD_SURFACE_WG`，Cubium 原来硬编码成 `nearTerrain` 特判（起始高度 0 或 64），
  只有 1/16 的情形恰好等于投影结果；
- **废弃传送门的模板要在登记起点前加载**：原来模板由 `generate` 惰性加载，起点先登记，
  包围盒于是退化成单点；现在登记前加载并断言模板存在，缺模板会直接报错而不是静默消失。

仍未对齐（各自已是独立的结构实现缺口，不属于装配框架问题）：

- **要塞**：`ConcentricRingsStructurePlacement` 只是按固定环数硬编码落点，没有原版按
  群系搜索后还要求"不是海洋"的那一步；语料里的要塞单元全部缺失；
- **废弃矿井 / 试炼密室 / 末地城 / 海底废墟 / 埋藏宝藏**：构件生成逻辑与原版不同
  （矿井的起始房间与分段规则、海底废墟的模板选取、末地城的塔楼递归），需要逐个按
  1.21.11 重写；
- **村庄 / 古代城市 / 前哨站的构件数已一致，但个别连接点取值仍有差异**（`structure_starts`
  逐片段比对可直接定位）。

`structure_starts` 的探针与 oracle 都只读"本区块内的结构起点"，不写方块、不依赖邻区，
因此可以逐区块独立复算；语料由 oracle 按原版放置规则枚举候选区块后等距取样，再补每格的
邻居区块与若干空白区块，用来覆盖"候选判定不通过"这条路径。

## 村民交易：已查出的问题

村民交易拆成四个能力，各管一段：

- `villager_list` 导出**整张交易表**（每职业 5 个等级的全部条目）。它不经过抽签，所以每一处
  差异都能定位到"哪一级、第几条、哪个字段"，是查表数据最快的手段；
- `villager_gen` 验证**抽取过程**：同一 seed 下抽中哪两条、每条内部抽到什么；
- `villager_price` 单独验证 `MerchantOffer.getAdjustedBuyPrice()` 的价格公式；
- `villager_restock` 单独验证 `VillagerEntity::restock()` 的先更新需求再重置用量。

`villager_list` 首轮跑通就查出并修掉的交易表数据错误（都对着 1.21.11 的 `VillagerTrades` 核实过）：

- 农民的面包 / 南瓜派 / 曲奇 / 蛋糕、牧师的萤石、牧羊人的剪刀：`maxUses` 用了 16，
  原版这几个是 `DEFAULT_SUPPLY`（12）；
- 屠夫的煤炭：经验记为"卖"档，原版是 `XP_LEVEL_1_BUY`（2）；
- 制图师的探险地图装饰类型：`village` / `ocean_explorer` 之类的自造 id 换成原版的
  `village_plains` / `village_desert` / `village_savanna` / `village_taiga` / `village_snowy` /
  `monument` / `mansion` / `jungle_temple` / `swamp_hut` / `trial_chambers`。

`villager_gen` 在表数据对齐之后，又暴露出随机序列的四处错位，同样已修：

- **整批交易共用一条随机流**：原版 `addOffersFromItemListings` 把同一个 `RandomSource` 既用于
  "抽哪一条"、又传给条目的 `getOffer`。项目原来给每条派生独立子流，两段随机互相独立，
  抽取序列整体不同；现在整批共用一条；
- **离线随机源必须是 `RandomSource.create(seed)` 对应的 `LegacyRandomSource`**（Java 48 位
  LCG），不是 Xoroshiro；
- **附魔池的顺序就是标签的声明顺序**：原版按下标取标签内容，项目早先按 id 排序过一遍，同一
  随机数抽到了别的附魔；`non_treasure` / `on_traded_equipment` 的顺序本身是数据驱动写死的，
  不能重排；
- **附魔书的价格公式**：`2 + nextInt(5 + level*10) + 3*level`，命中
  `#double_trade_price`（经验修补）再翻倍、封顶 64。

仍未对齐：**附魔装备 / 附魔书的附魔内容**（390 个单元里的 35 个）。根因是原版逐附魔维护
`supported_items` / `primary_items` / `exclusive_set` / `weight` 四个数据字段，而项目把附魔
写死在代码里、只有粗粒度的 `EnchantmentType` 分类，于是候选列表（哪些附魔可能出现在这件
装备上）与各附魔的等级区间都和原版不同。修法是把附魔定义搬成数据驱动（或至少补齐这四张表），
属于独立的一轮工作；这些单元已在 `villager_gen` 里登记为已知差异，抽取序列、价格、次数、
经验仍然逐字段严格比对。


## 结构起点：已查出的问题

`structure_starts` 首轮跑通就查出并修掉的结构生成缺陷（每一条都对着原版源码核实过）：

- **结构集条目抽签不是加权轮盘**：原版 `StructureTemplatePool.getRandomTemplate` 是在
  **按权重逐份展开**的列表上取下标，Cubium 原来用的是"随机数落在哪个权重区间"的另一种
  抽法——起始块因此选错（村庄整座偏移十几格）。现已按下标语义重写；
- **单条目结构集不得消耗抽签随机数**：原版 `list.size() == 1` 时直接生成，Cubium 原来
  一律走抽签，多消耗一次随机数，末地城等结构随之整体错位；
- **村庄不该有专用装配流程**：村庄的结构定义 JSON 与拼图结构完全同构，原版也走同一条
  `JigsawPlacement.addPieces`（含 `start_jigsaw_name` 起始连接点选择）。Cubium 单写的
  `VillageStructure` 先抽旋转再抽起始块、且不按名字过滤起始块，起始块与装配走向都错。
  该类已删除，村庄改走通用 `JigsawStructure`；
- **拼图结构的生物群系校验要用候选生成点**：原版在 `GenerationStub` 的位置（起始块中心、
  投影后的地面线）采样，Cubium 在区块中心 + y=0 采样，群系边界处会把合法结构整片误杀
  （古代城市首轮被误杀就是这个原因）。裸 `JigsawStructure` / 试炼密室 / 前哨站现均按
  候选点校验，且校验位于装配之后（与原版 `findValidGenerationPoint` 的调用位置一致）；
- **掠夺者前哨站的起始高度**：原版是 `start_height: absolute 0` + `project_start_to_heightmap:
  WORLD_SURFACE_WG`，Cubium 原来硬编码成 `nearTerrain` 特判（起始高度 0 或 64），
  只有 1/16 的情形恰好等于投影结果；
- **废弃传送门的模板要在登记起点前加载**：原来模板由 `generate` 惰性加载，起点先登记，
  包围盒于是退化成单点；现在登记前加载并断言模板存在，缺模板会直接报错而不是静默消失。

仍未对齐（各自已是独立的结构实现缺口，不属于装配框架问题）：

- **要塞**：`ConcentricRingsStructurePlacement` 只是按固定环数硬编码落点，没有原版按
  群系搜索后还要求"不是海洋"的那一步；语料里的要塞单元全部缺失；
- **废弃矿井 / 试炼密室 / 末地城 / 海底废墟 / 埋藏宝藏**：构件生成逻辑与原版不同
  （矿井的起始房间与分段规则、海底废墟的模板选取、末地城的塔楼递归），需要逐个按
  1.21.11 重写；
- **村庄 / 古代城市 / 前哨站的构件数已一致，但个别连接点取值仍有差异**（`structure_starts`
  逐片段比对可直接定位）。

`structure_starts` 的探针与 oracle 都只读"本区块内的结构起点"，不写方块、不依赖邻区，
因此可以逐区块独立复算；语料由 oracle 按原版放置规则枚举候选区块后等距取样，再补每格的
邻居区块与若干空白区块，用来覆盖"候选判定不通过"这条路径。

## 村民交易：已查出的问题

村民交易拆成四个能力，各管一段：

- `villager_list` 导出**整张交易表**（每职业 5 个等级的全部条目）。它不经过抽签，所以每一处
  差异都能定位到"哪一级、第几条、哪个字段"，是查表数据最快的手段；
- `villager_gen` 验证**抽取过程**：同一 seed 下抽中哪两条、每条内部抽到什么；
- `villager_price` 单独验证 `MerchantOffer.getAdjustedBuyPrice()` 的价格公式；
- `villager_restock` 单独验证 `VillagerEntity::restock()` 的先更新需求再重置用量。

`villager_list` 首轮跑通就查出并修掉的交易表数据错误（都对着 1.21.11 的 `VillagerTrades` 核实过）：

- 农民的面包 / 南瓜派 / 曲奇 / 蛋糕、牧师的萤石、牧羊人的剪刀：`maxUses` 用了 16，
  原版这几个是 `DEFAULT_SUPPLY`（12）；
- 屠夫的煤炭：经验记为"卖"档，原版是 `XP_LEVEL_1_BUY`（2）；
- 制图师的探险地图装饰类型：`village` / `ocean_explorer` 之类的自造 id 换成原版的
  `village_plains` / `village_desert` / `village_savanna` / `village_taiga` / `village_snowy` /
  `monument` / `mansion` / `jungle_temple` / `swamp_hut` / `trial_chambers`。

`villager_gen` 在表数据对齐之后，又暴露出随机序列的四处错位，同样已修：

- **整批交易共用一条随机流**：原版 `addOffersFromItemListings` 把同一个 `RandomSource` 既用于
  "抽哪一条"、又传给条目的 `getOffer`。项目原来给每条派生独立子流，两段随机互相独立，
  抽取序列整体不同；现在整批共用一条；
- **离线随机源必须是 `RandomSource.create(seed)` 对应的 `LegacyRandomSource`**（Java 48 位
  LCG），不是 Xoroshiro；
- **附魔池的顺序就是标签的声明顺序**：原版按下标取标签内容，项目早先按 id 排序过一遍，同一
  随机数抽到了别的附魔；`non_treasure` / `on_traded_equipment` 的顺序本身是数据驱动写死的，
  不能重排；
- **附魔书的价格公式**：`2 + nextInt(5 + level*10) + 3*level`，命中
  `#double_trade_price`（经验修补）再翻倍、封顶 64。

仍未对齐：**附魔装备 / 附魔书的附魔内容**（390 个单元里的 35 个）。根因是原版逐附魔维护
`supported_items` / `primary_items` / `exclusive_set` / `weight` 四个数据字段，而项目把附魔
写死在代码里、只有粗粒度的 `EnchantmentType` 分类，于是候选列表（哪些附魔可能出现在这件
装备上）与各附魔的等级区间都和原版不同。修法是把附魔定义搬成数据驱动（或至少补齐这四张表），
属于独立的一轮工作；这些单元已在 `villager_gen` 里登记为已知差异，抽取序列、价格、次数、
经验仍然逐字段严格比对。

## 世界生成、实体物理与物品：已查出的问题

这一轮补齐三块能力时，harness 查出七处真实缺陷，全部已修。它们有个共同点：**都不崩溃、不报错，
只让游戏表现细微偏离**，靠肉眼或普通单测几乎无法定位。

### 雕刻掩码的三处问题（carving_mask）

原版 `CarvingMask` 的索引公式是 `x & 15 | (z & 15) << 4 | (y - minY) << 8`，
掩码记录「这一格已经刻过」，雕刻器每次写入前都要查它。

- **索引漏了 X/Z 掩码**：Cubium 写成 `x | (z << 4) | ((y - minY) << 8)`，少了 `& 15`。
  Y 位移用的还是 `CHUNK_SHIFT + SECTION_SHIFT`（= 8），恰好与 `<< 8` 同值所以看不出来；
  真正的差别是 X/Z 越界时的行为。补上掩码后与原版逐位一致。
- **构造函数带世界常量兜底**：`CarvingMask(chunkX, chunkZ, minY = -64, height = 384)` 有默认参数，
  调用点 `ChunkPrimer::carvingMask()` 直接用了默认值。下界（`minY=0, height=128`）与
  末地（`0/256`）因此拿到主世界的基准，`(y - minY)` 整体错位——洞穴在区块边界处多刻或少刻，
  表现在玩家眼里是「有断层的洞穴」。现在掩码的几何由调用方传入，并在重复取用时断言一致。
- **两个从未被读取的成员**：`m_chunkX` / `m_chunkZ` 只在构造里赋值，原版 `CarvingMask`
  根本没有区块坐标字段，一并删除。

另外，原版 `carveEllipsoid` 的循环边界把 Y 夹在 `[minGenY + 1, minGenY + genDepth - 1 - j1]`、
X/Z 夹在 `[0, 15]`，`BitSet.set(负索引)` 在原版逻辑里不可达。第一版探针构造了 Y 越界的场景，
在原版侧直接抛 `IndexOutOfBoundsException`——那是**测了一整段两边都不该走的代码**，
已改为只覆盖真实可达的几何。

### 朝向计算的 45° 边界（direction_rotate）

`fromYRot` 比 `floor(yaw / 90 + 0.5)`。`-ffast-math` 允许把除法改写成乘倒数，
而 `1/90` 无法用二进制精确表示：`yaw = -45` 时算出略小于 `-0.5` 的值，加 0.5 后
落到零的负侧，`floor` 得 `-1`，`&-1 & 3` 变成 3（East）——原版是 South。
`yaw` 为 45° 的奇数倍（±45、±135、±225、±315）全部会踩到，
表现为**按玩家朝向放置的方块在斜 45° 时朝反方向**（熔炉、织布机、中继器、按钮等一大票方块）。

修法是让那次除法在 double 下精确完成（`volatile` 中转），不改任何数值。

### 密度函数的区间传播（density_nested）

`Lerp` 的 `minValue/maxValue` 原本按「先算 `d * (e - s)` 的区间，再加上 `s` 的区间」合并。
但 `s` 同时出现在因子 `(e - s)` 里和最后的加项上，**两者不独立**。
`d ∈ [0,1]`、`s = 1`、`e = -3` 时原实现得到 `-3 + (-1) = -4`，真实下界是 `-3`。

这个下界会传给上游的 `Clamp`、`RangeChoice` 与缓存调度，是地形判定偏移的来源之一。
该式对 `d`、`s`、`e` 都是线性的，极值必定落在 8 个角点上，逐个枚举即可。

### 潜行边缘保护退化（edge_backoff）

原版 `Player.maybeBackOffFromEdge` 逐 0.05 步回退，**X 与 Z 分别处理**，最后再处理斜向组合，
每个循环在 `|位移| <= 0.05` 时归零跳出。这样玩家贴边斜走时只削掉会让他掉下去的那个分量，
另一个分量保留，因此能沿边缘滑行。

Cubium 原本是一次性取移动后的整个位置判定，没有支撑就把 X/Z **全部归零**——
表现是贴着方块边缘斜向走时**完全走不动**。

### 物品冷却的两处防御性分支（item_cooldowns）

- `setCooldown` 对 `ticks <= 0` 特判成 `removeCooldown`，原版 `addCooldown` 无条件 put；
- `getCooldownProgress` 用 `duration > 0 ? clamp(...) : 0`，把 0/0 的 NaN 掩盖成 0。
  原版 `Mth.clamp` 对 NaN 返回 NaN（两条比较都为 false，返回原值），而 `std::clamp`
  在 `-ffast-math` 下对 NaN 的行为不保证，所以这里改用显式比较。

`hasCooldown` 也顺带对齐：原版看的是**进度是否为正**而不是剩余 tick 是否为正，
两者只在时长为负这种非常规输入上分叉。
