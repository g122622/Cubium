# 功能方块模块 (Functional Blocks)

本目录包含各种功能性方块的实现，这些方块提供了特定的游戏功能，如存储、合成、红石交互等。

## 目录结构

```
functional/
├── BedBlock.hpp/cpp          # 床方块 (16色，双格结构)
├── BrewingStandBlock.hpp/cpp # 酿造台 (药水酿造)
├── ComposterBlock.hpp/cpp    # 堆肥桶 (堆肥系统)
├── CakeBlock.hpp/cpp         # 蛋糕 (可食用，7片)
├── BeaconBlock.hpp/cpp       # 信标 (增益效果)
├── BarrelBlock.hpp/cpp       # 木桶 (存储容器)
├── LecternBlock.hpp/cpp      # 讲台 (书籍展示，红石输出)
├── GrindstoneBlock.hpp/cpp   # 砂轮 (修复/祛魔)
├── StonecutterBlock.hpp/cpp  # 切石机 (石材切割)
├── LoomBlock.hpp/cpp         # 织布机 (旗帜图案)
├── BellBlock.hpp/cpp         # 钟 (声音/动画)
├── JukeboxBlock.hpp/cpp      # 唱片机 (音乐播放)
├── RespawnAnchorBlock.hpp/cpp # 重生锚 (下界重生点)
├── LodestoneBlock.hpp/cpp    # 磁石 (指南针绑定)
├── CartographyTableBlock.hpp/cpp # 制图台 (地图处理)
├── FletchingTableBlock.hpp/cpp   # 制箭台 (箭矢制作)
├── SmithingTableBlock.hpp/cpp    # 锻造台 (装备升级)
└── README.md
```

## 方块分类

### 容器类方块
| 方块 | 描述 | 特殊功能 |
|------|------|---------|
| BarrelBlock | 木桶 | 6方向放置，容器GUI |
| BrewingStandBlock | 酿造台 | 3瓶槽位，酿造药水 |
| JukeboxBlock | 唱片机 | 播放音乐唱片 |

### 工作站方块
| 方块 | 描述 | 特殊功能 |
|------|------|---------|
| GrindstoneBlock | 砂轮 | 修复物品，移除附魔 |
| StonecutterBlock | 切石机 | 石材切割配方 |
| LoomBlock | 织布机 | 旗帜图案制作 |
| CartographyTableBlock | 制图台 | 地图复制/扩展 |
| FletchingTableBlock | 制箭台 | 箭矢制作 |
| SmithingTableBlock | 锻造台 | 装备升级 |

### 特殊功能方块
| 方块 | 描述 | 特殊功能 |
|------|------|---------|
| BedBlock | 床 | 双格结构，设置重生点 |
| BeaconBlock | 信标 | 增益效果，金字塔基座 |
| ComposterBlock | 堆肥桶 | 8层堆肥，产出骨粉 |
| CakeBlock | 蛋糕 | 7片可食用，比较器输出 |
| LecternBlock | 讲台 | 书籍展示，红石脉冲 |
| BellBlock | 钟 | 声音/动画，多方向附着 |
| RespawnAnchorBlock | 重生锚 | 4级充能，下界重生 |
| LodestoneBlock | 磁石 | 指南针绑定 |

## 状态属性

### BedBlock
```cpp
- HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
- BED_PART: 部分 (HEAD, FOOT)
- OCCUPIED: 是否被占用
```

### BrewingStandBlock
```cpp
- HAS_BOTTLE_0: 第一个槽位是否有瓶子
- HAS_BOTTLE_1: 第二个槽位是否有瓶子
- HAS_BOTTLE_2: 第三个槽位是否有瓶子
```

### ComposterBlock
```cpp
- LEVEL_0_8: 填充等级 (0-8)
```

### CakeBlock
```cpp
- BITES_0_6: 已吃的片数 (0-6)
```

### BarrelBlock
```cpp
- FACING: 朝向 (6个方向)
- OPEN: 是否打开
```

### LecternBlock
```cpp
- HORIZONTAL_FACING: 朝向
- POWERED: 是否发出红石信号
- HAS_BOOK: 是否有书
```

### BellBlock
```cpp
- HORIZONTAL_FACING: 朝向
- BELL_ATTACHMENT: 附着类型 (FLOOR, CEILING, SINGLE_WALL, DOUBLE_WALL)
- POWERED: 是否被激活
```

### JukeboxBlock
```cpp
- HAS_RECORD: 是否有唱片
```

### RespawnAnchorBlock
```cpp
- CHARGES_0_4: 充能等级 (0-4)
```

## 依赖关系

```
Block (基类)
├── BedBlock
│   └── 双方块结构处理
├── BrewingStandBlock
│   └── 容器方块实体
├── ComposterBlock
│   └── 堆肥系统
├── CakeBlock
│   └── 可食用方块
├── BeaconBlock
│   └── 增益效果系统
├── BarrelBlock
│   └── 容器方块实体
├── LecternBlock
│   └── 红石信号输出
├── GrindstoneBlock
│   └── 附着检测
├── StonecutterBlock
│   └── 配方处理
├── LoomBlock
│   └── 配方处理
├── BellBlock
│   └── 多方向附着
├── JukeboxBlock
│   └── 音乐播放
├── RespawnAnchorBlock
│   └── 充能系统
├── LodestoneBlock
│   └── 指南针绑定
├── CartographyTableBlock
├── FletchingTableBlock
└── SmithingTableBlock
```

## 待实现功能

- [ ] BedBlock: 爆炸逻辑（下界/末地）、睡眠交互
- [ ] BrewingStandBlock: 方块实体、酿造配方
- [ ] ComposterBlock: 完整堆肥概率表、骨粉产出
- [ ] CakeBlock: 食物恢复逻辑
- [ ] BeaconBlock: 方块实体、效果范围计算
- [ ] BarrelBlock: 方块实体、容器GUI
- [ ] LecternBlock: 方块实体、书籍交互
- [ ] BellBlock: 动画系统、声音播放
- [ ] JukeboxBlock: 方块实体、音乐唱片系统
- [ ] RespawnAnchorBlock: 维度检测、爆炸逻辑
- [ ] LodestoneBlock: 指南针绑定系统

## 参考

- Minecraft 1.16.5 源码: `net.minecraft.block.*`
