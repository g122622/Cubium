# 原版内容的装配与注册数据

## 目录结构树

```text
vanilla/
├── AgriculturalBlocks.cpp  # 模块接口与实现
├── AgriculturalBlocks.hpp  # 农作物方块的静态引用
├── BambooBlocks.cpp  # 模块接口与实现
├── BambooBlocks.hpp  # 竹木系列方块（1.20 足迹与故事）的静态引用
├── BaseBlocks.cpp  # 模块接口与实现
├── BaseBlocks.hpp  # 基础方块、矿石、矿物方块、原木、石头变种、泥土变种、砂岩的静态引用
├── BlockMapColorTable.gen.cpp  # 查原版方块状态的地图颜色
├── BlockMapColorTable.hpp  # 某个方块状态相对所属方块的默认地图颜色例外
├── BlockStatePredicates.cpp  # 模块接口与实现
├── BlockStatePredicates.hpp  # 恒为 false（原版 Blocks.never）。
├── BuildingBlocks.cpp  # 模块接口与实现
├── BuildingBlocks.hpp  # 建筑、功能、含水方块的静态引用
├── BuildingVariantBlocks.cpp  # 模块接口与实现
├── BuildingVariantBlocks.hpp  # 楼梯、台阶、墙、栅栏、门、活板门、栅栏门、染色玻璃板的静态引用
├── CandleBlocks.cpp  # 模块接口与实现
├── CandleBlocks.hpp  # 蜡烛方块和蜡烛蛋糕方块的静态引用
├── CaveBlocks.cpp  # 模块接口与实现
├── CaveBlocks.hpp  # 洞穴方块（1.17 洞穴与山崖 Part 1）的静态引用
├── CherryBlocks.cpp  # 模块接口与实现
├── CherryBlocks.hpp  # 樱花系列方块（1.20 足迹与故事）的静态引用
├── ColoredBlocks.cpp  # 模块接口与实现
├── ColoredBlocks.hpp  # 染色方块（羊毛、地毯、染色玻璃、混凝土、陶瓦、潜影盒）的静态引用
├── CopperBlocks.cpp  # 模块接口与实现
├── CopperBlocks.hpp  # 铜方块（1.17 洞穴与山崖 + 1.21 扩展）的静态引用
├── DeepslateBlocks.cpp  # 模块接口与实现
├── DeepslateBlocks.hpp  # 深板岩系列方块的静态引用
├── FlowerPotBlocks.cpp  # 模块接口与实现
├── FlowerPotBlocks.hpp  # 花盆方块的静态引用
├── GardenBlocks.cpp  # 模块接口与实现
├── GardenBlocks.hpp  # 1.21.2+ 花园觉醒植被方块的静态引用
├── HarvestToolTable.cpp  # 模块接口与实现
├── HarvestToolTable.hpp  # 单个方块的采集规则（对齐原版 Tool.Rule 的 minesAndDrops 分支）
├── MangroveBlocks.cpp  # 模块接口与实现
├── MangroveBlocks.hpp  # 红树林木材系列方块的静态引用（1.19 荒野更新）
├── MudBlocks.cpp  # 模块接口与实现
├── MudBlocks.hpp  # 泥巴系列方块的静态引用（1.19 荒野更新）
├── NaturalBlocks.cpp  # 模块接口与实现
├── NaturalBlocks.hpp  # 自然方块（冰、粘液、珊瑚、海洋、仙人掌等）的静态引用
├── NetherBlocks.cpp  # 模块接口与实现
├── NetherBlocks.hpp  # 下界方块（灵魂沙、玄武岩、黑石、下界扩展等）的静态引用
├── PaleGardenBlocks.cpp  # 模块接口与实现
├── PaleGardenBlocks.hpp  # 苍白花园方块系列的静态引用
├── RedstoneBlocks.cpp  # 模块接口与实现
├── RedstoneBlocks.hpp  # 红石方块、铁轨方块的静态引用
├── SculkBlocks.cpp  # 模块接口与实现
├── SculkBlocks.hpp  # 幽匿系列方块的静态引用（1.19 荒野更新）
├── ShelfBlocks.cpp  # 模块接口与实现
├── ShelfBlocks.hpp  # 书架方块系列（1.21.4+ 木质变体）的静态引用
├── SignBannerBlocks.cpp  # 模块接口与实现
├── SignBannerBlocks.hpp  # 告示牌和旗帜的静态引用
├── TrailsBlocks.cpp  # 足迹与故事版本其他方块（1.20 考古和装饰）的注册
├── TrailsBlocks.hpp  # 上述方块的静态引用
├── TrialBlocks.cpp  # 模块接口与实现
├── TrialBlocks.hpp  # 试炼密室方块系列的静态引用
├── TuffBlocks.cpp  # 模块接口与实现
├── TuffBlocks.hpp  # 凝灰岩方块系列的静态引用
├── VanillaBlocks.cpp  # 模块接口与实现
├── VanillaBlocks.hpp  # 原版方块静态引用
├── VegetationBlocks.cpp  # 模块接口与实现
├── VegetationBlocks.hpp  # 植被方块（草、花、蘑菇、树苗、南瓜西瓜等）的静态引用
├── WildBlocks.cpp  # 模块接口与实现
└── WildBlocks.hpp  # 1.19 荒野更新其他方块的静态引用
```

## 内部模块关系

BlockRegistry 持有方块及状态索引，vanilla 装配各行为族的原版实例；地图颜色快照仅由注册层绑定到公共模型。

## 上下游外部依赖关系

Block、BlockState 与各具体方块提供注册对象；服务器启动、物品映射、存档和协议编码查询注册结果。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。
