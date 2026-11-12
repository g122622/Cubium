# 功能方块模块

## 目录结构

```text
functional/
├── AnvilBlock.hpp/cpp             # 铁砧与损坏变体
├── BarrelBlock.hpp/cpp            # 木桶容器
├── BeaconBlock.hpp/cpp            # 信标交互
├── BedBlock.hpp/cpp               # 床的双方块结构与睡眠
├── BellBlock.hpp/cpp              # 钟的附着、敲击与红石触发
├── BrewingStandBlock.hpp/cpp      # 酿造台交互
├── CakeBlock.hpp/cpp              # 蛋糕食用
├── CandleCakeBlock.hpp/cpp        # 蜡烛蛋糕点燃与食用
├── CartographyTableBlock.hpp/cpp  # 制图台交互
├── CompostableItems.hpp/cpp       # 可堆肥物品概率表
├── ComposterBlock.hpp/cpp         # 堆肥等级与漏斗容器
├── CraftingTableBlock.hpp/cpp     # 工作台交互
├── FletchingTableBlock.hpp/cpp    # 制箭台方块
├── GrindstoneBlock.hpp/cpp        # 砂轮附着与形状
├── JukeboxBlock.hpp/cpp           # 唱片机交互
├── LecternBlock.hpp/cpp           # 讲台形状、书籍交互与红石脉冲
├── LodestoneBlock.hpp/cpp         # 磁石指南针绑定
├── LoomBlock.hpp/cpp              # 织布机交互
├── RespawnAnchorBlock.hpp/cpp     # 重生锚充能与维度检查
├── SmithingTableBlock.hpp/cpp     # 锻造台交互
├── StonecutterBlock.hpp/cpp       # 切石机交互
├── ChiseledBookshelfBlock.hpp/cpp # 雕纹书架（六槽占用状态与比较器输出）
├── DecoratedPotBlock.hpp/cpp      # 饰纹陶罐（放入/取出物品、投射物碎裂）
├── BrushableBlock.hpp/cpp         # 可刷方块（可疑沙/可疑沙砾，刷扫音效与完成目标）
├── SnifferEggBlock.hpp/cpp        # 嗅探兽蛋（三阶段孵化）
└── README.md                     # 模块关系与使用约束
```

## 内部模块关系

各方块继承 `Block`；铁砧与可刷方块使用 `FallingBlock`，蜡烛蛋糕使用装饰模块的 `AbstractCandleBlock`。`ComposterBlock` 依赖 `CompostableItems` 并实现 `ISidedInventoryProvider`。讲台、钟、容器及考古方块将持久状态和持续行为交给对应方块实体。

## 上下游外部依赖关系

- 上游：`BlockState` 与属性系统、`CollisionShape`、`IWorld`、放置上下文、玩家与物品、`TickManager`、红石系统、音效、掉落及统计系统。
- 下游：方块注册模块构造各实例；世界更新调用放置、邻居变化与计划刻；物品交互和方块实体调用对应入口。
- 方块实体关联包括 `BarrelEntity`、`BeaconEntity`、`BrewingStandEntity`、`JukeboxEntity`、`LecternEntity`、`BellBlockEntity`、`DecoratedPotBlockEntity` 和 `BrushableBlockEntity`。

## 容易踩的坑

### 讲台形状与脉冲

- 北向外形由底座、立柱及三级斜台组成。斜台 Y 区间是 10..14、12..16、14..18 像素，Z 区间是 1..5.333333、5.333333..9.666667、9.666667..14；五参数 `boxZ` 的 Y/Z 区间不能交换。
- 碰撞和支撑只用底座与立柱；外形可高出单位格，不能裁掉 y=16..18 的台面。
- 北→东→南→西为顺时针旋转，单次盒体映射 `(x,z) → (1-z,x)`；四槽缓存必须显式映射 `Direction`，不能直接使用枚举值。
- 翻页通过 `LecternEntity` 调用 `pulse`，充能两 tick 后复位；放书、移书和充能讲台移除时需要通知下方邻居。`getStrongPower` 只在 `Direction::Up` 返回强信号。

### 床与重生锚

- 床的 HEAD/FOOT 是两个方块，放置前必须确认头部位置可替换。创造模式拆 FOOT 时需同时移除 HEAD 并避免额外掉落。
- `FACING` 从床脚指向床头：脚部连接方向等于 FACING，头部相反。形状必须按两半各自的外侧放置两条床腿，不能只按 FACING 共用形状。
- 床颜色由构造参数传入，各色实例分别注册。睡眠检查占用、距离、头顶空间及附近怪物；不允许睡眠的维度中使用会爆炸。
- 重生锚使用维度的 `respawnAnchorWorks()` 判定，不能以维度名称代替。

### 堆肥桶

- 等级 7→8 需要延迟 20 tick。外形随等级变化，碰撞始终使用空桶外壁，不能用完整方块代替。
- 等级 0..6 的临时输入容器只接受上方输入；等级 7 无槽位；等级 8 的骨粉只允许下方提取。容器 `setChanged` 负责堆肥或清空，不能假定它是长期持有的方块实体容器。

### 砂轮、铁砧与炼药锅

- 砂轮有三种附着面 × 四朝向，支撑失效时掉落；默认方向通过不能替代其他附着面与朝向的检查。
- 铁砧截面沿 X/Z 不对称，应按水平朝向轴取形状。`column` 的尺寸是完整直径。
- 空、水/细雪、岩浆炼药锅使用不同方块类；水位跨空/非空边界时要替换方块类型。

### 考古与嗅探兽蛋

- 考古四种方块各占一对文件。可刷方块的两种音效及刷完后的目标方块由构造参数绑定；刷扫进度由方块实体维护。
- 嗅探兽蛋由计划刻驱动三阶段孵化，不能改成随机刻。下方加速标签会改变总时长；同类型 HATCH 属性更新不会重新触发本项目的 `onBlockAdded`，因此 tick 必须显式调度下一阶段。
- 饰纹陶罐是否掉落陶片由 CRACKED 决定；罐内物品、陶罐物品和陶片掉落的职责不能重复执行。

### 蜡烛蛋糕、比较器与统计

- 蜡烛蛋糕没有 BITES/CANDLES 属性，食用后转为完整蛋糕，比较器固定输出 14，亮度固定为单根蜡烛的 3；它使用 `CANDLE_CAKES` 标签。
- 有比较器输出的方块需同时实现能力声明与信号查询，不能假定所有容器或交互方块采用同一信号公式。
- 交互统计应在成功交互后授予，常量来自 `common/stats/Stats.hpp`，并确保在 `StatRegistry` 注册；堆肥桶没有专属交互统计。

### 钟

- 附着方式包括 Floor、Ceiling、SingleWall、DoubleWall，碰撞缓存按附着面与方向组织。
- 双面墙钟失去一侧支撑时降为单面墙并调整朝向；单面墙升级双面墙时，新增支撑方向必须为 `FACING` 的反向。
- Y 轴命中或高度大于 0.8124 的命中无效；地面钟要求命中轴与朝向轴相同，墙面钟要求不同，吊钟允许任意水平轴。
- 玩家、投射物和红石上升沿均经统一敲钟入口驱动方块实体、音效和 `BLOCK_ACTIVATE` 游戏事件；成功玩家交互授予 `BELL_RING`。
