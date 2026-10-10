# 红石方块

本目录实现信号源、二极管、机械消费者及铁轨，与公共红石查询、世界邻居通知和计划刻协作。

## 目录结构

```text
redstone/
├── RedstoneBlock.hpp/.cpp             # 恒定弱信号电源
├── RedstoneTorchBlock.hpp/.cpp         # 附着反相、计划刻与烧毁
├── RedstoneWallTorchBlock.hpp/.cpp     # 侧面附着的反相火把
├── RedstoneWireBlock.hpp/.cpp          # 连接形状和衰减传播
├── RedstoneDiodeBlock.hpp/.cpp         # 二极管方向、主输入与输出通知
├── RedstoneRepeaterBlock.hpp/.cpp      # 延迟、脉冲扩展与侧面锁存
├── RedstoneComparatorBlock.hpp/.cpp    # 模拟输出、比较、减法和容器读取
├── ObserverBlock.hpp/.cpp              # 方块变化检测和短脉冲
├── AbstractButtonBlock.hpp/.cpp        # 按钮状态与复位调度
├── StoneButtonBlock.hpp/.cpp           # 石质按钮
├── WoodButtonBlock.hpp/.cpp            # 木质按钮和箭矢检测
├── LeverBlock.hpp/.cpp                 # 拉杆交互、持续输出和附着通知
├── AbstractPressurePlateBlock.hpp/.cpp # 压力板信号与实体检测
├── StonePressurePlateBlock.hpp/.cpp    # 仅生物触发的压力板
├── WoodPressurePlateBlock.hpp/.cpp     # 所有有效实体触发的压力板
├── WeightedPressurePlateBlock.hpp/.cpp # 按实体数量计算模拟信号
├── DaylightDetectorBlock.hpp/.cpp      # 日光强度转换
├── PistonBlock.hpp/.cpp                # 活塞推动和拉回
├── PistonStructureHelper.hpp/.cpp      # 推动链、黏性分支及数量限制
├── PistonHeadBlock.hpp/.cpp            # 活塞头存活与基座关联
├── MovingPistonBlock.hpp/.cpp          # 移动动画代理
├── DispenserBlock.hpp/.cpp             # 红石触发的物品发射
├── DropperBlock.hpp/.cpp               # 红石触发的物品投掷
├── TripWireBlock.hpp/.cpp              # 绊线接触检测
├── TripWireHookBlock.hpp/.cpp          # 绊线连接与信号
├── NoteBlock.hpp/.cpp                  # 乐器和音高
├── TNTBlock.hpp/.cpp                   # 红石、火焰与投射物引燃
├── TargetBlock.hpp/.cpp                # 投射物命中强度
├── RedstoneLampBlock.hpp/.cpp          # 即时点亮与延迟熄灭
├── AbstractRailBlock.hpp/.cpp          # 支撑、含水和连接入口
├── RailState.hpp/.cpp                  # 轨道连接、斜坡及道岔
├── RailBlock.hpp/.cpp                  # 普通铁轨与三连接道岔
├── PoweredRailBlock.hpp/.cpp           # 带电轨道的八格传电
├── ActivatorRailBlock.hpp/.cpp         # 复用带电轨道逻辑的激活轨
└── DetectorRailBlock.hpp/.cpp          # 矿车接触触发与周期复查
```

## 内部模块关系

- `RedstoneDiodeBlock` 为中继器和比较器提供方向、输入和下游通知；两者分别实现锁存与模拟输出更新。
- `RedstoneWallTorchBlock` 复用火把计划刻，并通过虚函数选择侧面附着。
- `AbstractRailBlock` 管理支撑和放置连接；`RailState` 负责连接传播。
- `ActivatorRailBlock` 继承 `PoweredRailBlock`，共用有界传电；搜索按方块实例区分类型，动力轨和激活轨互不传电。
- `PistonBlock` 使用 `PistonStructureHelper` 计算移动集合，动画和实体位移由 `PistonBlockEntity` 处理。

## 上下游外部依赖关系

- 上游：`IWorld`、`BlockState`、`RedstonePower`、`RedstoneSystem`、计划刻、方块实体及物品交互接口。
- 下游：原版方块注册、世界放置和邻居通知、矿车运动、玩家交互及结构加载。
- 红石方块不依赖服务端专属类型；服务端负责世界写入、模拟输出通知和计划刻执行。

## 容易踩的坑

- 信号查询的 `side` 从接收者指向信号源。二极管 `facing` 指向主输入，输出位于反方向。实体面的几何方向不能直接当作查询方向。
- `getSignal` 包含导体邻居提供的强信号；红石块仅输出弱信号，不能把它当作强充能实心方块的电源。红石火把只强充能上方导体。
- 红石线前后都连接中继器，并连接比较器侧端。计算外部输入时须暂时关闭红石线自身输出，防止反馈维持高电平；台阶和导体另一侧的消费者需要间接通知。
- 中继器延迟为档位乘二。已安排的短正脉冲必须输出并扩展至档位延迟，短负脉冲在计划刻采样时可被过滤。侧面仅二极管能锁存，解锁后仍需要延迟。
- 比较器即使持续通电，输出强度变化也要更新方块实体并通知下游。输出只在计划刻采样；隔导体的容器模拟值会覆盖低于十五的导体信号。
- 火把累计的是六十刻内的熄灭事件，复亮不计数；拆除不清历史；历史窗口控制是否允许复燃，已有计划刻控制何时重新采样，普通更新不替换已有的被动复查。墙火把须检查侧面附着，不能复用下方检测。支撑丢失必须移除。
- 活塞推动范围使用建筑上下界，不能使用地表高度；数量上限包含全部黏性分支。使用方块的推动反应虚函数，不能绕过为活塞头等方块定义的阻挡规则。
- 活塞头动画代理位于底座前一格，完成后恢复为活塞头。移动集合扩展时应复制坐标，不能保存会因容器扩容失效的引用。黏液和蜂蜜都黏住普通方块，但彼此互斥。
- 带电铁轨只沿同类型、同轴轨道追踪真实电源，最多传八格；其他已带电轨道不能成为无限续传的独立电源。斜坡更新还须通知上下位置。
- 强制放置轨道即使形状未变，也必须向邻轨传播连接。普通形状通知不能重算并覆盖斜坡；拆除普通弯轨一端不自动变直。
- 探测轨由矿车接触触发，激活后每二十刻复查，不依赖随机刻。矿车即使静止也必须执行方块接触回调。
- 木、石压力板持久化 `powered`，测重压力板持久化 `power`；共享逻辑须使用信号读写接口。石质压力板过滤生物，其余过滤不触发压力板的特殊实体；潜行玩家不触发绊线。
- 铁轨含水状态须安排流体刻。普通轨支持十种形状，动力、激活和探测轨只支持直轨及斜坡六种形状。
- 活塞头依赖类型、伸出状态和朝向均匹配的底座；玩家破坏头部时级联处理基座，正常收回不能误走破坏路径。
- TNT 引燃须遵守 `tntExplodes`；仅生成点燃实体与生成后移除原方块是不同操作，调用者须选择正确入口。
