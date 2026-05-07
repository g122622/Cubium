# 功能方块模块 (Functional Blocks)

本目录包含各种功能性方块的实现，这些方块提供了特定的游戏功能，如存储、合成、红石交互等。

## 目录结构

```
functional/
├── BedBlock.hpp/cpp          # 床方块 (16色，双格结构)
├── BrewingStandBlock.hpp/cpp # 酿造台 (药水酿造)
├── CauldronBlock.hpp/cpp     # 炼药锅 (储水、物品清洗)
├── CompostableItems.hpp/cpp  # 可堆肥物品注册表
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

### CauldronBlock
```cpp
- LEVEL_0_3: 水位 (0-3, 0=空, 3=满)
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
├── CauldronBlock
│   ├── PotionUtils (药水工具类)
│   ├── DyeableArmorItem (可染色盔甲)
│   └── 音效系统
├── ComposterBlock
│   ├── CompostableItems (可堆肥物品注册表)
│   ├── TickManager (tick调度)
│   └── ItemDropHelper (物品掉落)
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
- [x] ComposterBlock: 完整堆肥概率表、骨粉产出、玩家交互
- [x] CauldronBlock: 水桶/玻璃瓶/水瓶交互、皮革盔甲清洗
- [ ] CakeBlock: 食物恢复逻辑
- [ ] BeaconBlock: 方块实体、效果范围计算
- [ ] BarrelBlock: 方块实体、容器GUI
- [ ] LecternBlock: 方块实体、书籍交互
- [ ] BellBlock: 动画系统、声音播放
- [ ] JukeboxBlock: 方块实体、音乐唱片系统
- [ ] RespawnAnchorBlock: 维度检测、爆炸逻辑
- [ ] LodestoneBlock: 指南针绑定系统

## 已实现功能详解

### ComposterBlock (堆肥桶)

完整的 MC 1.16.5 堆肥桶实现：

**功能**:
- 8层填充等级 (LEVEL_0_8)
- 等级7→8 转换：20 tick 延迟后产出骨粉
- 玩家右键交互：添加可堆肥物品
- 比较器输出：等级值

**CompostableItems 注册表**:
```cpp
// 堆肥概率表
30%: 种子类、干海带、甜浆果
50%: 西瓜片
65%: 苹果、农作物、地狱疣
85%: 面包、曲奇、烤马铃薯
100%: 南瓜派
```

**交互流程**:
1. 玩家右键放置可堆肥物品
2. 概率性增加等级 (播放成功/失败音效)
3. 等级7时调度 20 tick 延迟
4. 等级8时右键收获骨粉

### CauldronBlock (炼药锅)

完整的 MC 1.16.5 炼药锅交互实现：

**功能**:
- 4级水位 (LEVEL_0_3)
- 雨天自动填充水
- 比较器输出：水位值

**交互支持**:
| 物品 | 操作 | 水位变化 | 音效 |
|------|------|----------|------|
| 水桶 | 装水 | → 3 | ITEM_BUCKET_EMPTY |
| 空桶 | 取水 | 3 → 0 | ITEM_BUCKET_FILL |
| 玻璃瓶 | 取水 | -1 | ITEM_BOTTLE_FILL |
| 水瓶 | 倒水 | +1 | ITEM_BOTTLE_EMPTY |
| 皮革盔甲 | 清洗颜色 | -1 | 无 |

**创造模式**: 不消耗物品

## 参考

- Minecraft 1.16.5 源码: `net.minecraft.block.*`
