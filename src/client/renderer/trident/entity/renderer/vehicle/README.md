# 载具渲染器 (Vehicle Renderers)

本目录包含船和矿车等可乘坐实体的渲染器实现。

## 目录结构

```
vehicle/
├── VehicleRenderers.hpp     # 船和矿车的模型与渲染器定义
└── VehicleRenderers.cpp     # 船和矿车的模型与渲染器实现
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                     VehicleRenderers                         │
├─────────────────────────────────────────────────────────────┤
│  BoatModel          │  MinecartModel                         │
│  (船体模型部件)      │  (矿车六面模型)                         │
│  BoatRenderer       │  MinecartRenderer                      │
│  (船渲染+纹理选择)   │  (矿车渲染)                             │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `core/EntityRenderer.hpp` | 实体渲染器基类 |
| `model/core/ModelRenderer.hpp` | 模型部件渲染器 |
| `common/util/math/MathConstants.hpp` | 数学常量（PI、PI_DOUBLE） |
| `common/resource/ResourceLocation.hpp` | 资源路径 |
| `common/entity/BoatEntity` | 船实体类（前向声明） |
| `common/entity/AbstractMinecartEntity` | 矿车实体基类（前向声明） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `renderer/RendererRegistration.cpp` | 通过工厂注册船和矿车渲染器 |
| `core/EntityRendererManager.cpp` | 通过工厂创建渲染器实例 |

## 容易踩的坑

### 1. 船的木材类型纹理

船有 6 种木材类型（橡木、云杉、白桦、丛林、金合欢、深色橡木），每种对应不同的纹理。`BoatRenderer::getTexture()` 通过 `BoatType` 枚举索引静态纹理数组，确保枚举值与数组索引一致。

### 2. ModelRenderer 的纹理尺寸

`BoatModel` 使用 128×64 纹理，`MinecartModel` 使用 64×32 纹理。调用 `setTextureSize()` 设置正确的纹理尺寸，否则 UV 坐标会计算错误。

### 3. 模型旋转角度单位

`ModelRenderer::setRotateAngleX/Y/Z()` 接受弧度值。代码中使用 `PI_DOUBLE`（即 2π）来计算旋转角度，例如 `PI_DOUBLE / 2.0` 表示 90°，`PI_DOUBLE * 1.5` 表示 270°。

### 4. 矿车内部底板偏移

`MinecartModel::setInsideOffset()` 用于调整内部底板的 Y 偏移，当乘客乘坐时需要调整此值。矿车的 6 个面存储在 `m_sides[5]` 数组中，第 6 个元素（索引 5）是内部底板。
