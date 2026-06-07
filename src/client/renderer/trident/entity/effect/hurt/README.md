# 受伤闪烁效果

本目录包含实体受伤闪烁效果实现。

## 目录结构

```
hurt/
├── HurtFlashEffect.hpp      # 受伤闪烁效果头文件
└── HurtFlashEffect.cpp      # 受伤闪烁效果实现
```

## 内部模块关系

本目录仅包含一个静态工具类 `HurtFlashEffect`，无内部模块划分。

## 上下游外部依赖关系

**本目录依赖**：
- `common/core/Types.hpp` - 基础类型定义
- `common/util/math/Vector4.hpp` - 四维向量
- `common/util/math/MathUtils.hpp` - 数学工具函数
- `common/entity/core/LivingEntity.hpp` - 生物实体（获取 hurtTime、deathTime）

**被依赖**：
- `EntityRendererManager` - 实体渲染管理器，调用 `isHurt()`、`getPackedOverlay()`
- `EntityPipeline` - 实体渲染管线，通过 push constant 传递 hurtTime 到着色器
- `entity.frag` - 片段着色器，计算闪烁强度并应用效果

## 容易踩的坑

1. **着色器方案 vs 纹理方案差异**：本项目采用着色器内置计算实现受伤闪烁，而非 MC 1.16.5 的 OverlayTexture 纹理采样方式。`getPackedOverlay()` 方法保留用于兼容性，但当前着色器不使用此值。

2. **hurtTime 递减方向**：hurtTime 从 10 递减到 0，受伤开始时 hurtTime=10，结束时 hurtTime=0。进度计算应为 `1.0 - (hurtTime / 10.0)`。

3. **initialize/cleanup 实际无操作**：由于采用着色器方案，这两个方法仅设置初始化标志，不加载/释放任何资源。

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::hurt {
    class HurtFlashEffect;
}
```
