# 地图系统 (Map System)

本目录实现了 Minecraft 1.16.5 的地图物品系统，包括地图数据存储、颜色系统、装饰物标记等。

## 目录结构

```
map/
├── README.md               # 本文件
├── MaterialColor.hpp/cpp   # 地图颜色系统 - 61种颜色定义和阴影计算
├── MapData.hpp/cpp         # 地图数据核心 - 128x128像素数据、装饰物、旗帜标记
├── MapDecoration.hpp/cpp   # 地图装饰物 - 玩家标记、旗帜图标等
├── MapBanner.hpp           # 旗帜标记 - 记录旗帜位置和颜色
├── MapFrame.hpp            # 展示框标记 - 记录展示框位置和旋转
├── MapIdTracker.hpp/cpp    # 地图ID追踪器 - 分配唯一地图ID
└── MapDataManager.hpp/cpp  # 地图数据管理器 - CRUD、持久化、tick更新
```

## 核心概念

### 地图颜色编码

每个地图像素占用1字节，编码为 `colorIndex * 4 + shadeIndex`：
- `colorIndex` (0-58): MaterialColorId 枚举值，对应61种颜色
- `shadeIndex` (0-3): 阴影级别
  - 0: 中等偏暗 (亮度 180/255)
  - 1: 中等偏亮 (亮度 220/255)
  - 2: 最亮 (亮度 255/255)
  - 3: 最暗 (亮度 135/255)

### 缩放级别

| scale | 覆盖范围 (blocks) | 每像素代表 (blocks) |
|-------|-------------------|-------------------|
| 0     | 128×128           | 1                 |
| 1     | 256×256           | 2                 |
| 2     | 512×512           | 4                 |
| 3     | 1024×1024         | 8                 |
| 4     | 2048×2048         | 16                |

### 装饰物类型

27种装饰类型，包括玩家标记、旗帜、结构图标等。部分类型有地图颜色用于物品栏显示。

### 持久化

地图数据通过 MapDataManager 管理，使用 RocksDB 存储。每个地图以 `map_{id}` 为键存储NBT格式数据。ID追踪器存储在 `idcounts` 键下。

## 命名空间

```cpp
namespace mc::world::map {
    class MaterialColor;
    class MapData;
    class MapDecoration;
    class MapBanner;
    class MapFrame;
    class MapIdTracker;
    class MapDataManager;
}
```
