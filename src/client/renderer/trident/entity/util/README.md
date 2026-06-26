# 工具类

本目录包含实体渲染系统的工具类（阴影、名称标签、世界文本渲染）。

## 目录结构

```
util/
├── NameTagRenderer.hpp/cpp     # 名称标签渲染器（委托 WorldTextRenderer 渲染）
├── ShadowRenderer.hpp/cpp      # 阴影渲染器（方块级阴影，参考 MC 1.16.5）
└── WorldTextRenderer.hpp/cpp   # 世界空间文本渲染器（billboard + 视锥/背面剔除）
```

## 内部模块关系

```
┌──────────────────┐
│ NameTagRenderer  │
│ (名称标签渲染器)  │
└────────┬─────────┘
         │ 委托渲染
         ▼
┌──────────────────┐
│ WorldTextRenderer│◄─────┐
│ (世界文本渲染器)  │      │
└────────┬─────────┘      │
         │ 剔除优化        │ 独立使用
         ▼                │
┌──────────────────┐      │
│    Frustum       │      │
│  (视锥体剔除)     │      │
└──────────────────┘      │
                          │
┌──────────────────┐      │
│ ShadowRenderer   │──────┘
│ (阴影渲染器)      │
└──────────────────┘
```

**关系说明：**
- `NameTagRenderer` 是 `WorldTextRenderer` 的高层封装，负责名称标签特有的位置计算和样式设置
- `WorldTextRenderer` 提供通用的世界空间文本渲染能力（billboard、剔除、背景面板）
- `ShadowRenderer` 独立工作，不依赖其他工具类

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/entity` | Entity、ClientEntity 实体数据 |
| `common/world` | IWorld 世界接口（方块查询） |
| `common/util/math/frustum` | Frustum 视锥体剔除 |
| `client/renderer/trident/entity/pipeline` | EntityPipeline 渲染管线 |
| `client/ui` | Font、Glyph 字体系统（WorldTextRenderer） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `client/renderer/trident/entity/core/EntityRenderer` | 调用 `renderShadow()`、`renderNameTag()` |
| `client/renderer/trident/entity/core/EntityRendererManager` | 初始化/清理工具类、设置相机信息 |

## 容易踩的坑

### 1. 相机信息必须每帧设置

`ShadowRenderer`、`WorldTextRenderer` 的视锥体剔除和背面剔除依赖相机信息。`ShadowRenderer` 的阴影距离衰减也需要相机位置。如果忘记调用 `setCameraPosition()`（由 `EntityRendererManager::setCameraInfo()` 每帧调用），阴影将在任何距离都保持最大透明度，名称标签和世界文本的剔除功能也会失效或崩溃。

### 2. 阴影渲染的方块检测条件

`ShadowRenderer.renderBlockShadow()` 有三个必须同时满足的条件：
- 下方方块渲染类型 != INVISIBLE
- 当前位置光照等级 > 3
- 下方方块有不透明碰撞形状

如果自定义方块的碰撞形状设置不正确，阴影渲染会出错。

### 3. 初始化顺序

三个工具类都需要在 `EntityPipeline` 初始化后调用 `initialize()`。清理时按相反顺序调用 `cleanup()`。

### 4. 阴影透明度计算细节

透明度受以下因素影响：
- 到相机的距离衰减（距离平方 > 256 即距离 > 16 格时消失，参考 MC EntityRenderer.extractShadow()）
- 实体到地面的高度（距离衰减）
- 幼年实体减半
- 方块位置亮度

### 5. 名称标签位置计算

名称标签 Y 坐标 = `entity.y + entity.height + 0.3`（MC 1.16.5 标准）。如果自定义实体的 height 设置不正确，名称标签位置会偏移。
