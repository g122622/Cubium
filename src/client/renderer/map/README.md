# 地图渲染 (Map Renderer)

本目录实现了客户端地图渲染系统，将 MapData 的颜色数据转换为可视化的地图纹理。

## 目录结构树

```
src/client/renderer/map/
├── MapRenderer.hpp     # 地图渲染器 - 管理地图纹理缓存和2D绘制
└── MapRenderer.cpp     # 实现文件
```

## 内部模块关系

单个文件模块，无内部依赖。

MapRenderer 的核心流程：
1. 从 `MapData` 获取 128x128 颜色字节数组
2. 通过 `MaterialColor::pixelToArgb()` 转换为 RGBA 像素
3. 使用 `GuiRenderer` 绘制到屏幕（边框 + 地图像素 + 装饰图标）

## 上下游外部依赖关系

**依赖方（上游）**：
- `src/client/ui/screen/MapScreen.hpp` - 地图查看屏幕
- `src/client/ui/minecraft/screens/CartographyScreen.hpp` - 制图台GUI（地图预览）
- `src/client/renderer/trident/firstperson/FirstPersonRenderer.hpp` - 手持地图渲染

**被依赖方（下游）**：
- `src/common/world/map/MapData.hpp` - 地图数据
- `src/common/world/map/MaterialColor.hpp` - 颜色转换
- `src/common/world/map/MapDecoration.hpp` - 装饰物定义
- `src/client/renderer/trident/gui/GuiRenderer.hpp` - GUI渲染器
- `src/client/world/ClientMapDataCache.hpp` - 客户端地图数据缓存

## 容易踩的坑

### 逐像素绘制性能

当前实现使用 `GuiRenderer::fillRect()` 逐像素绘制 128x128 = 16384 个小矩形。对于性能敏感场景，应考虑：
- 使用纹理上传 API 替代逐像素绘制
- 批量合并相邻同色像素

### 颜色编码格式

MapData 中每个像素为 1 字节，编码为 `colorIndex * 4 + shadeIndex`：
- `colorIndex=0` (AIR) 表示透明像素，不绘制
- 阴影级别 0-3 对应不同亮度

### 装饰图标坐标

装饰物 `x/y` 为 -128~127 的字节值，映射到地图像素坐标时需要 `+128` 后再除以 256.0。

### GuiRenderer 依赖

必须通过 `setGuiRenderer()` 设置 GUI 渲染器后才能正常渲染，否则 `renderMap()` 会静默返回。
