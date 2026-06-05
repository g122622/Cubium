# registry/ - 原版方块注册

原版方块的静态引用和注册实现。每个方块类别拆分为独立的头文件和源文件，便于维护和编译。

## 目录结构

```
registry/
├── VanillaBlocks.hpp       # 主入口，VanillaBlocks 类继承所有分类结构体
├── VanillaBlocks.cpp       # initialize() 实现，调用各分类注册函数
├── BaseBlocks.hpp/.cpp     # 基础方块、矿石、矿物、原木、木板、冰、玻璃等
├── BuildingBlocks.hpp/.cpp # 建筑、功能、含水、石砖、虫蚀、石英、海晶、紫珀、骨块等
├── BuildingVariantBlocks.hpp/.cpp # 楼梯、台阶、墙、门、栅栏门、活板门、染色玻璃板、特殊方块
├── ColoredBlocks.hpp/.cpp  # 染色方块：羊毛、地毯、染色玻璃、混凝土、陶瓦
├── NaturalBlocks.hpp/.cpp  # 自然方块：冰变种、粘液、珊瑚、海洋方块、仙人掌等
├── NetherBlocks.hpp/.cpp   # 下界方块、末地方块、下界扩展植物
├── RedstoneBlocks.hpp/.cpp # 红石方块、铁轨方块
├── SignBannerBlocks.hpp/.cpp # 告示牌、旗帜
├── VegetationBlocks.hpp/.cpp # 植被：草、花、蘑菇、树苗、南瓜西瓜、竹子
└── README.md
```

## 设计说明

`VanillaBlocks` 类通过多重继承组合所有分类结构体，每个结构体位于 `mc::block_registry` 命名空间中：

- `BaseBlocks` - 基础方块（空气、石头、泥土、水、岩浆等）、矿石、矿物方块、原木、木板
- `BuildingBlocks` - 建筑方块（砖块、书架等）、功能方块（工作台、箱子等）、石砖系列、虫蚀方块、石英、海晶、紫珀
- `BuildingVariantBlocks` - 楼梯/台阶/墙/门/栅栏门/活板门/染色玻璃板、特殊方块（刷怪笼、屏障等）
- `ColoredBlocks` - 16色方块系列（羊毛、地毯、染色玻璃、混凝土、混凝土粉末、陶瓦）
- `NaturalBlocks` - 自然扩展方块（冰变种、珊瑚、海洋方块、粘液块、蜂蜜块等）
- `NetherBlocks` - 下界方块、下界扩展植物、末地方块、传送门、信标等
- `RedstoneBlocks` - 红石机械、按钮、压力板、活塞、铁轨
- `SignBannerBlocks` - 告示牌（8种木材×2形态）、旗帜（16色×2形态）
- `VegetationBlocks` - 植被（草、花、蘑菇、树苗）、南瓜西瓜系列、竹子

## 初始化顺序

`VanillaBlocks::initialize()` 按以下顺序调用各注册函数，确保依赖关系正确：

1. `registerBaseBlocks()` - 必须最先，注册流体和基础方块
2. `registerBuildingBlocks()` - 石砖等被后续引用
3. `registerNetherBlocks()` - 末地方块、下界扩展
4. `registerVegetationBlocks()` - 植被、南瓜西瓜
5. `registerNaturalBlocks()` - 珊瑚引用 AIR（来自 BaseBlocks）
6. `registerColoredBlocks()` - 独立的颜色方块
7. `registerRedstoneBlocks()` - 红石机械
8. `registerSignBannerBlocks()` - 告示牌和旗帜
9. `registerBuildingVariantBlocks()` - 引用 BaseBlocks 和 BuildingBlocks 的方块状态

最后初始化 `BlockTags`。

## 外部依赖

- 被整个项目引用：约 250+ 文件通过 `#include "world/block/VanillaBlocks.hpp"` 使用
- 依赖 `BlockRegistry` 进行方块注册
- 依赖各 `blocks/` 子目录的具体方块类型
- 依赖 `fluid/` 子目录的流体注册
