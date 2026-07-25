# 地图系统 (Map System)

本目录实现了 Minecraft 1.16.5 的地图物品系统，包括地图数据存储、颜色系统、装饰物标记等。

## 目录结构

```
map/
├── MaterialColor.hpp/cpp   # 地图颜色系统 - 59种颜色定义和阴影计算
├── MapData.hpp/cpp         # 地图数据核心 - 128x128像素、装饰物、旗帜、展示框标记
├── MapDecoration.hpp/cpp   # 地图装饰物 - 27种装饰类型定义、序列化、字符串转换
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

MapBanner → MapDecoration (通过DyeColor映射到DecorationType)
MapBanner → BannerEntity (通过fromWorld从世界获取旗帜信息)
MapFrame → MapDecoration (生成FRAME类型装饰)
MapData → IWorld (下界旋转随机化、旗帜验证)
MapData → MaterialColor (颜色查找表 + 阴影计算)
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
world/IWorld.hpp            - 世界接口 (下界旋转、旗帜验证)
world/blockentity/interactive/BannerEntity.hpp - 旗帜方块实体 (fromWorld)
network/codec/PacketSerializer.hpp - 网络序列化（已迁至 codec/）
network/codec/PacketDeserializer.hpp - 网络反序列化（已迁至 codec/）
entity/serialization/NbtHelper.hpp - NBT辅助读取
```

### 外部对本模块的依赖

```
ServerWorld                 - 持有MapDataManager实例，每tick调用更新
FilledMapItem               - 创建/更新地图数据，更新地形像素、旗帜交互
ItemFrameEntity             - 添加/移除MapFrame标记
MapDataPacket               - 读取MapData发送给客户端（位于 `common/network/codec/MapDataPacket.hpp`）
存档系统                    - 通过NBT持久化地图数据
```

## 容易踩的坑

### MaterialColor初始化

`MaterialColor::initialize()` **必须在使用任何颜色API之前调用**，否则断言失败。颜色查找表是静态填充的，未初始化时访问会导致 MC_ASSERT 失败。

### 颜色编码格式

地图像素字节编码为 `colorIndex * 4 + shadeIndex`：
- `colorIndex` (高6位): MaterialColorId 枚举值 (0-58)
- `shadeIndex` (低2位): 阴影级别 (0-3)

`MaterialColor::pixelToArgb()` 会自动解析此格式，不要手动解析。

### 地图中心对齐规则

`MapData::calculateMapCenter()` 将地图中心对齐到缩放网格：
```
centerX = floor((x + 64) / (128 * (1 << scale))) * (128 * (1 << scale)) + (64 * (1 << scale)) - 64
```

### 装饰物坐标范围

`MapDecoration` 的坐标是 `i8` 类型 (-128 ~ 127)，地图有效范围是 -63 ~ 63：
- 范围外但 < 320 单位：使用 `PLAYER_OFF_MAP` 类型
- 范围外且 >= 320 单位：使用 `PLAYER_OFF_LIMITS` 类型（需要 `unlimitedTracking`）

### 下界旋转随机化

在下界维度中，`MapData::calculateRotation()` 使用基于游戏时间的伪随机旋转值而非实际朝向：
```
i = gameTime / 10; rotation = ((i * i * 34187121 + i * 121) >> 15) & 15
```
模拟指南针在下界失灵的效果。其他维度使用实际朝向角度。

### ITextComponent序列化

装饰物和旗帜的自定义名称(`ITextComponent`)在NBT和网络序列化中采用JSON桥接模式：
- 写入NBT: `ITextComponent::toJson().dump()` → `nbt::string_tag`
- 读取NBT: `nbt::string_tag` → `nlohmann::json::parse()` → `ITextComponent::fromJson()`
- 网络序列化: 与NBT模式相同，通过 `PacketSerializer::writeString()` 传输JSON字符串
- JSON解析失败时回退为纯文本 `StringTextComponent`

### 维度ID序列化

`MapData` 的维度字段在NBT中的序列化兼容两种格式：
- 字符串格式（1.16+）: `dimension: "minecraft:overworld"/"minecraft:the_nether"/"minecraft:the_end"` — 写入格式
- 整数格式（旧版MC）: `dimension: 0/-1/1`，对应 `MapDimensionId::Overworld/Nether/End` — 读取时兼容

维度ID与字符串之间的转换使用 `MapDimensionId.hpp` 中的集中式工具函数 `dimensionIdToString()` / `dimensionIdFromString()` / `dimensionNameToId()`，不应在各处重复实现转换逻辑。

### DecorationType 字符串转换

`decorationTypeFromString()` 和 `decorationTypeToString()` 提供 `DecorationType` 枚举与字符串的双向转换，支持 MC 1.16.5 格式（如 `"mansion"`、`"red_x"`）和 1.21.11 格式（如 `"minecraft:mansion"`、`"minecraft:red_x"`）。`decorationTypeFromString()` 无法识别的字符串返回 `std::nullopt`。

### 旗帜交互

`MapData::tryAddBanner()` 实现toggle行为：如果旗帜已存在则移除，不存在则添加。同时检查装饰物数量上限（256个非FRAME装饰）。需要 `BannerEntity` 方块实体存在才能添加旗帜标记。

`MapData::addBanner()` 直接添加旗帜标记和对应装饰物，不进行世界交互检查，用于NBT反序列化或测试场景。

`MapData::removeStaleBanners()` 检查指定坐标上的旗帜是否仍然有效（旗帜方块实体仍存在且颜色匹配），移除失效的旗帜标记。此方法在 `FilledMapItem::_updateMapData` 的像素扫描循环中逐像素调用，与MC Java版 `MapItem.update` 中调用 `checkBanners` 的行为一致。

### 地图锁定机制

`MapData::lockFrom()` 会完全复制源地图的颜色数据并设置 `locked=true`，锁定后的地图不能再更新地形，但可以更新装饰物。用于地图复制物品。

### MapIdTracker持久化

`MapIdTracker` 的数据存储在 `data/idcounts.dat` 的 `map` 字段中。读取时需要 +1，因为存储的是"已分配的最大ID"：
```cpp
m_nextMapId = nbt_helper::tryGetInt(tag, "map").value_or(-1) + 1;
```
