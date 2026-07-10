# 工具类

本目录包含实体渲染系统的工具类（方块网格构建、阴影、名称标签、世界文本渲染）。

## 目录结构

```
util/
├── BlockMeshBuilder.hpp/cpp    # 方块网格构建工具（BlockState → 顶点+索引，供 FallingBlock/TNT/末影人手持方块复用）
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
│WorldTextRenderer │◄─────┐
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

┌──────────────────┐   下落方块 / TNT / 末影人手持方块
│ BlockMeshBuilder │◄──── 渲染器调用 buildBlockMesh()
│ (方块网格构建)    │      生成 ModelVertex 数组 + 索引数组
└────────┬─────────┘
         │ 读取
         ▼
┌──────────────────┐
│ BlockModelCache  │  方块外观缓存（元素、面、纹理区域）
└──────────────────┘
```

**关系说明：**
- `NameTagRenderer` 是 `WorldTextRenderer` 的高层封装，负责名称标签特有的位置计算和样式设置
- `WorldTextRenderer` 提供通用的世界空间文本渲染能力（billboard、剔除、背景面板）
- `ShadowRenderer` 独立工作，不依赖其他工具类
- `BlockMeshBuilder` 是方块实体渲染的网格数据来源，从 `BlockModelCache` 读取方块外观并生成 GPU 管线可用的顶点/索引数组

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/entity` | Entity、ClientEntity 实体数据 |
| `common/world` | IWorld 世界接口（方块查询）、BlockState |
| `common/world/block/model` | BlockModelCache、BlockAppearance 方块外观 |
| `common/util/math/frustum` | Frustum 视锥体剔除 |
| `client/renderer/trident/entity/pipeline` | EntityPipeline 渲染管线 |
| `client/renderer/trident/entity/model/core` | ModelVertex 顶点结构 |
| `client/ui` | Font、Glyph 字体系统（WorldTextRenderer） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `client/renderer/trident/entity/core/EntityRenderer` | 调用 `renderShadow()`、`renderNameTag()` |
| `client/renderer/trident/entity/core/EntityRendererManager` | 初始化/清理工具类、设置相机信息 |
| `client/renderer/trident/entity/renderer/special/FallingBlockRenderer` | 调用 `BlockMeshBuilder::buildBlockMesh()` 构建下落方块网格 |
| `client/renderer/trident/entity/renderer/special/TNTRenderer` | 调用 `BlockMeshBuilder::buildBlockMesh()` 构建 TNT 方块网格 |
| `client/renderer/trident/entity/layer/HeldBlockLayer` | 调用 `BlockMeshBuilder::buildBlockMesh()` 构建末影人手持方块网格 |

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

### 6. BlockMeshBuilder 回退路径

`BlockMeshBuilder::buildBlockMesh()` 依赖 `BlockModelCache::modelCache()`，该缓存需要资源包加载完成后才可用。在测试环境或资源包未加载时，`buildBlockMesh` 会回退到 `buildFallbackCubeMesh()`（24 顶点 / 36 索引单位立方体）。调用方无需区分两条路径，但应预期在无资源包环境下得到的是单位立方体而非真实方块模型。

### 7. BlockMeshBuilder 顶点坐标系

`buildBlockMesh` 生成的顶点坐标已乘以 1/16，从像素范围（0-16）转换为世界单位（0-1）。调用方（如 FallingBlockRenderer）只需应用方块级别的平移/旋转/缩放变换，不需要再做单位换算。回退路径 `buildFallbackCubeMesh` 同样输出 0-1 范围顶点，保持一致的坐标系约定。
