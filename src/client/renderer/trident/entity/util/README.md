# 工具类

本目录包含实体渲染系统的工具类。

## 文件列表

| 文件 | 描述 |
|------|------|
| `ShadowRenderer.hpp/cpp` | 阴影渲染器 |
| `NameTagRenderer.hpp/cpp` | 名称标签渲染器 |

## ShadowRenderer

负责在实体下方渲染阴影圆盘。阴影大小根据实体尺寸和与地面的距离动态调整。

### 主要功能

- 阴影圆盘生成
- 透明度随高度衰减
- 支持不同阴影半径

### 使用方法

```cpp
// 初始化（游戏启动时调用一次）
ShadowRenderer::initialize(16);  // 16 边形圆盘

// 渲染阴影
ShadowRenderer::renderShadow(entity, partialTicks, 0.5, 0.8);

// 清理（游戏关闭时调用）
ShadowRenderer::cleanup();
```

### 阴影透明度计算

```cpp
// 透明度随高度衰减
f64 maxDistance = 16.0;
f64 distanceFactor = 1.0 - (height / maxDistance);
f64 alpha = baseAlpha * distanceFactor * shadowRadius;
```

### 参考

- MC 1.16.5 EntityRenderer.renderShadow()
- MC 1.16.5 阴影渲染逻辑

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

1. **阴影渲染**需要与地面高度检测配合，目前使用简化实现
2. **名称标签渲染**需要与文本渲染系统集成，目前使用占位实现
3. 两个类都是静态工具类，不需要实例化
