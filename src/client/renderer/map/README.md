# 地图渲染 (Map Renderer)

本目录实现了客户端地图渲染系统，将 MapData 的颜色数据转换为可视化的地图纹理。

## 文件说明

| 文件 | 描述 |
|------|------|
| `MapRenderer.hpp/cpp` | 地图渲染器 - 管理地图纹理缓存和2D绘制 |

## 核心功能

### MapRenderer

将 MapData 中 128x128 的颜色字节数组转换为 RGBA 像素数据，通过 GuiRenderer 绘制到屏幕上。

- `updateMapTexture()`: 比较 MapData 与缓存，按需更新 RGBA 纹理数据
- `renderMap()`: 在指定位置绘制地图（含边框和装饰图标）
- `renderDecorations()`: 在地图上绘制玩家标记、旗帜等装饰图标
- `removeMap()`: 移除指定地图的缓存
- `clear()`: 清除所有缓存

### 颜色转换

MapData 中每个像素为 1 字节，编码为 `colorIndex * 4 + shadeIndex`：
- 通过 `MaterialColor::pixelToArgb()` 转换为 ARGB 颜色
- `colorIndex=0` (AIR) 表示透明像素，不绘制

### 装饰图标渲染

目前使用简单的彩色方块表示装饰图标，未来可替换为纹理图集图标。

## 使用方式

```cpp
MapRenderer mapRenderer;
mapRenderer.setGuiRenderer(&guiRenderer);

// 更新地图纹理
mapRenderer.updateMapTexture(mapId, mapData);

// 绘制地图
mapRenderer.renderMap(mapId, screenX, screenY, 128.0, true, &mapData);
```

## 相关系统

- **地图数据**: `src/common/world/map/` - MapData, MaterialColor
- **客户端缓存**: `src/client/world/ClientMapDataCache.hpp` - 客户端地图数据缓存
- **制图台屏幕**: `src/client/ui/screen/CartographyScreen.hpp` - 制图台GUI
- **地图查看屏幕**: `src/client/ui/screen/MapScreen.hpp` - 全屏地图查看
- **第一人称渲染**: `FirstPersonRenderer::renderMapFirstPerson()` - 手持地图渲染
