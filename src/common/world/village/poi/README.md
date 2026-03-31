# POI（兴趣点）系统

本目录实现了村庄系统中的兴趣点(Point of Interest)管理，包括床位、工作站、钟等特殊方块。

## 目录结构

```
poi/
├── PointOfInterestType.hpp/cpp   # POI类型枚举和辅助函数
├── PointOfInterest.hpp/cpp       # POI数据结构
├── PointOfInterestStorage.hpp/cpp # POI存储和查询
└── README.md                      # 本文档
```

## 核心类

### PointOfInterestType

POI类型枚举，定义所有可作为兴趣点的方块类型：

- **床位类型** (16种颜色的床) - 用于村民睡眠和重生
- **工作站类型** (12种) - 对应村民职业
- **其他类型** - 钟、下界传送门、磁石、避雷针

### PointOfInterest

表示单个POI的数据结构：

```cpp
PointOfInterest poi(BlockPos(100, 64, 200), PointOfInterestType::Smoker);
poi.acquire(villagerId, gameTime);  // 村民占用
poi.release(villagerId);             // 村民释放
```

### PointOfInterestStorage

POI存储和查询管理器，提供：

- **注册/注销** POI
- **空间查询** - 最近POI、范围内所有POI
- **占用管理** - 票据系统
- **区块级索引** - 高效的空间查询

## 使用方法

### 注册POI

```cpp
PointOfInterestStorage storage;

// 当方块被放置时注册POI
storage.registerPOI(BlockPos(100, 64, 200), PointOfInterestType::Smoker);

// 当方块被破坏时注销POI
storage.unregisterPOI(BlockPos(100, 64, 200));
```

### 查询POI

```cpp
// 查找最近的未占用烟熏炉
auto bedPos = storage.findNearestFree(
    villagerPos,
    PointOfInterestType::Smoker,
    48.0f  // 最大搜索距离
);

if (bedPos.has_value()) {
    // 找到了，村民可以前往工作
}
```

### 占用POI

```cpp
// 村民占用工作站点
storage.acquirePOI(jobSite, villagerId, gameTime);

// 村民释放工作站点（离开或被解雇）
storage.releasePOI(jobSite, villagerId);
```

## 线程安全

所有公共方法都是线程安全的，使用内部互斥锁保护。

## 与MC Java对齐

- 参考 MC 1.16.5 `PointOfInterestType`, `PointOfInterest`, `PointOfInterestStorage`
- 票据系统(tickets)用于管理POI占用
- 区块级索引用于高效空间查询
