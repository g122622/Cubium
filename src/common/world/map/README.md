# 地图系统 (Map System)

本目录实现了 Minecraft 1.16.5 的地图物品系统，包括地图数据存储、颜色系统、装饰物标记等。

## 目录结构

```
map/
├── MaterialColor.hpp/cpp   # 地图颜色系统 - 59种颜色定义和阴影计算
├── MapData.hpp/cpp         # 地图数据核心 - 128x128像素、装饰物、旗帜、展示框标记
├── MapDecoration.hpp/cpp   # 地图装饰物 - 27种装饰类型定义和序列化
├── MapBanner.hpp/cpp       # 旗帜标记 - 旗帜位置/颜色记录和装饰映射
├── MapFrame.hpp/cpp        # 展示框标记 - 展示框位置/旋转记录
├── MapIdTracker.hpp/cpp    # 地图ID追踪器 - 自增ID分配
└── MapDataManager.hpp/cpp  # 地图数据管理器 - CRUD、tick更新
```

## 内部模块关系

```
MapDataManager (管理器门面)
    ├── MapData (核心数据)
    │   ├── MapDecoration[] (装饰物集合)
    │   ├── MapBanner[] (旗帜标记集合)
    │   ├── MapFrame[] (展示框标记集合)
    │   └── MapInfo[] (玩家追踪信息)
    └── MapIdTracker (ID分配器)

MaterialColor (独立工具类，被MapData渲染时使用)
    └── 颜色查找表 + 阴影计算

依赖关系：
- MapBanner → MapDecoration (通过DyeColor映射到DecorationType)
- MapFrame → MapDecoration (生成FRAME类型装饰)
- MapData → MaterialColor, MapDecoration, MapBanner, MapFrame
- MapDataManager → MapData, MapIdTracker
```

## 上下游外部依赖关系

### 本模块依赖

```
core/Types.hpp              - 基础类型定义 (i8/u8/i32/f32/f64等)
util/nbt/Nbt.hpp            - NBT序列化
util/assert/AssertMacros.hpp - 断言宏
util/text/ITextComponent.hpp - 文本组件 (装饰物自定义名称)
util/color/DyeColor.hpp     - 染料颜色枚举 (旗帜颜色映射)
world/block/BlockPos.hpp    - 方块坐标 (旗帜/展示框位置)
world/dimension/MapDimensionId.hpp - 维度ID (地图所属维度)
network/packet/PacketSerializer.hpp - 网络序列化 (MapDecoration)
network/packet/PacketDeserializer.hpp - 网络反序列化
entity/serialization/NbtHelper.hpp - NBT辅助读取
```

### 外部对本模块的依赖

```
ServerWorld                 - 持有MapDataManager实例，每tick调用更新
FilledMapItem               - 创建/更新地图数据，更新地形像素
ItemFrameEntity             - 添加/移除MapFrame标记
BannerBlockEntity           - 添加/移除MapBanner标记
MapItemSavedData (网络同步) - 读取MapData发送给客户端
存档系统                    - 通过NBT持久化地图数据
```

## 容易踩的坑

### MaterialColor初始化

`MaterialColor::initialize()` **必须在使用任何颜色API之前调用**，否则断言失败。颜色查找表是静态填充的，未初始化时访问会导致 MC_ASSERT 失败。

### 颜色编码格式

地图像素字节编码为 `colorIndex * 4 + shadeIndex`：
- `colorIndex` (高6位): MaterialColorId 枚举值 (0-58)
- `shadeIndex` (低2位): 阴影级别 (0-3)

**注意**：`MaterialColor::pixelToArgb()` 会自动解析这个格式，不要手动解析。

### 地图中心对齐规则

`MapData::calculateMapCenter()` 将地图中心**对齐到缩放网格**，不是简单的坐标取整：
```
centerX = floor((x + 64) / (128 * (1 << scale))) * (128 * (1 << scale)) + (64 * (1 << scale)) - 64
```
不同缩放级别的地图覆盖区域不同，但网格对齐确保相邻地图无缝衔接。

### 装饰物坐标范围

`MapDecoration` 的坐标是 `i8` 类型 (-128 ~ 127)，地图有效范围是 -63 ~ 63：
- 范围外但 < 320 单位：使用 `PLAYER_OFF_MAP` 类型
- 范围外且 >= 320 单位：使用 `PLAYER_OFF_LIMITS` 类型（需要 `unlimitedTracking`）

### 地图锁定机制

`MapData::lockFrom()` 会**完全复制**源地图的颜色数据并设置 `locked=true`，锁定后的地图不能再更新地形，但可以更新装饰物。用于地图复制物品。

### MapIdTracker持久化

`MapIdTracker` 的数据应存储在 `data/idcounts.dat` 的 `map` 字段中。**读取时需要 +1** 因为存储的是"已分配的最大ID"，而不是"下一个ID"：
```cpp
m_nextMapId = nbt_helper::tryGetInt(tag, "map").value_or(-1) + 1;
```
