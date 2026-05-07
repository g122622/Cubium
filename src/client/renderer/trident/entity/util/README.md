# 工具类

本目录包含实体渲染系统的工具类。

## 文件列表

| 文件 | 描述 |
|------|------|
| `ShadowRenderer.hpp/cpp` | 阴影渲染器 |
| `NameTagRenderer.hpp/cpp` | 名称标签渲染器 |

## ShadowRenderer

负责在实体下方渲染阴影。实现了 MC 1.16.5 风格的方块级阴影渲染。

### 核心算法

阴影渲染遵循 MC 1.16.5 EntityRendererManager 的实现：

1. **范围计算**: 遍历实体周围 `[x-radius, x+radius] × [y-radius, y] × [z-radius, z+radius]` 范围内的方块
2. **方块检测**: 对每个方块位置检查：
   - 下方方块渲染类型 != INVISIBLE
   - 当前位置光照等级 > 3
   - 下方方块有不透明碰撞形状
3. **透明度计算**: `alpha = (baseAlpha - heightDiff/2) × 0.5 × brightness`
4. **形状裁剪**: 根据方块碰撞箱绘制阴影四边形

### 主要功能

- 方块级阴影渲染（参考 MC 1.16.5）
- 透明度随高度衰减
- 根据亮度调整阴影强度
- 幼年实体阴影减半
- 支持不同阴影半径

### 使用方法

```cpp
#include "client/renderer/trident/entity/util/ShadowRenderer.hpp"

// 初始化（游戏启动时调用一次）
ShadowRenderer::initialize(pipeline);

// GPU 管线路径 - Entity 版本（推荐）
ShadowRenderer::renderShadow(cmd, entity, partialTicks, shadowRadius, shadowAlpha, pipeline);

// GPU 管线路径 - ClientEntity 版本
ShadowRenderer::renderShadow(cmd, clientEntity, partialTicks, shadowRadius, shadowAlpha, pipeline);

// 清理（游戏关闭时调用）
ShadowRenderer::cleanup();
```

### 阴影参数说明

| 参数 | 说明 | 典型值 |
|------|------|--------|
| `shadowRadius` | 阴影半径（方块） | Pig: 0.5, Cow: 0.7, Player: 0.5 |
| `shadowAlpha` | 基础透明度 | 0.8 (大多数实体), 0.75 (物品) |
| `partialTicks` | 部分 tick（插值） | 0.0-1.0 |

### 透明度计算详解

```cpp
// 距离衰减：实体离地面越远，阴影越淡
f64 distanceFactor = 1.0 - (heightAboveGround / 256.0);

// 幼体减半
f64 sizeMultiplier = entity.isChild() ? 0.5 : 1.0;

// 亮度因子：暗处阴影更淡
f64 brightness = world.getBrightness(blockPos);

// 最终透明度
f64 alpha = baseAlpha * distanceFactor * sizeMultiplier * brightness;
```

### 方块阴影渲染流程

```
renderShadow()
    │
    ├── 计算搜索范围 [minX, maxX] × [minY, maxY] × [minZ, maxZ]
    │
    └── 对每个方块位置调用 renderBlockShadow()
            │
            ├── 检查渲染类型 != INVISIBLE
            ├── 检查光照等级 > 3
            ├── 检查 hasOpaqueCollisionShape()
            │
            └── 绘制阴影四边形
                    │
                    ├── 获取方块碰撞箱
                    ├── 计算纹理坐标（径向渐变）
                    └── 应用透明度
```

### 参考

- MC 1.16.5 `EntityRendererManager.renderShadow()`
- MC 1.16.5 `EntityRendererManager.renderBlockShadow()`

## NameTagRenderer

负责在实体上方渲染名称标签。支持自定义颜色、背景和可见性控制。

### 主要功能

- 名称标签渲染
- 可见距离控制
- 背景颜色自定义
- 随距离缩放

### 使用方法

```cpp
// 渲染名称标签
NameTagRenderer::renderNameTag(entity, entity.getDisplayName(), partialTicks);

// 设置最大可见距离
NameTagRenderer::setMaxDistance(64.0);

// 设置样式
NameTagRenderer::setScale(0.025);
NameTagRenderer::setShowBackground(true);
NameTagRenderer::setBackgroundColor(0, 0, 0, 128);  // 半透明黑色背景

// 检查是否应该渲染
if (NameTagRenderer::shouldRenderNameTag(entity, distanceToCamera)) {
    NameTagRenderer::renderNameTag(entity, displayName, partialTicks);
}
```

### 名称标签位置计算

```cpp
// 名称标签位于实体高度之上
f64 nameTagY = entity.y() + entity.height() + 0.3;
```

### 缩放计算

```cpp
// 远距离时稍微放大以保持可读性
f64 distanceScale = 1.0 + std::log(distanceToCamera) * 0.1;
f64 scale = baseScale * distanceScale;
```

### 参考

- MC 1.16.5 EntityRenderer.renderNameTag()
- MC 1.16.5 名称标签渲染逻辑

## 命名空间

```cpp
namespace mc::client::renderer::entity::util {
    class ShadowRenderer;
    class NameTagRenderer;
}
```

## 注意事项

1. **阴影渲染**使用方块级渲染，根据方块碰撞箱裁剪阴影形状
2. **名称标签渲染**需要与文本渲染系统集成
3. 两个类都是静态工具类，使用前需初始化

## 测试

单元测试位于 `tests/client/renderer/entity/ShadowRendererTest.cpp`，覆盖：
- 透明度计算逻辑
- 阴影范围计算
- 方块阴影条件检测
