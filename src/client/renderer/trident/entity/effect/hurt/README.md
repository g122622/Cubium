# 受伤闪烁效果

本目录包含实体受伤闪烁效果实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `HurtFlashEffect.hpp` | 受伤闪烁效果头文件 |
| `HurtFlashEffect.cpp` | 受伤闪烁效果实现 |

## 功能详解

### HurtFlashEffect（受伤闪烁效果）

用于渲染实体受伤时的红色闪烁效果。

**实现方式**：

本项目采用**着色器内置计算**方式实现受伤闪烁，而非 MC 1.16.5 的纹理采样方式。

| 方面 | MC 1.16.5 方案 | 本项目方案 |
|------|---------------|-----------|
| 实现位置 | OverlayTexture.java + 着色器 | entity.frag 着色器 |
| 资源需求 | 16x16 动态纹理 | 无 |
| UV 计算 | Java 端计算打包 UV | 着色器内计算 |
| 颜色混合 | 纹理采样 + 混合 | 直接 mix() 计算 |

**着色器实现参考** (`shaders/entity.frag`)：

```glsl
// 计算受伤闪烁强度
float computeHurtFlashIntensity() {
    if (pc.hurtTime <= 0.0) {
        return 0.0;
    }
    float progress = 1.0 - (pc.hurtTime / 10.0);
    float intensity = 1.0 - progress;
    // 使用 sin 函数创建闪烁效果
    float flash = sin(pc.hurtTime * 3.14159 * 0.3) * 0.5 + 0.5;
    return intensity * flash;
}

// 应用受伤闪烁效果
if (shouldApplyHurtEffect()) {
    float hurtIntensity = computeHurtFlashIntensity();
    vec3 hurtColor = vec3(1.0, 0.0, 0.0);
    color = mix(color, hurtColor, hurtIntensity * 0.5);
}
```

**使用方法**：

```cpp
// 初始化（无资源需要加载）
HurtFlashEffect::initialize();

// 检查实体是否受伤
if (HurtFlashEffect::isHurt(livingEntity)) {
    // 获取覆盖层UV（用于兼容性，当前着色器不使用）
    i32 packedOverlay = HurtFlashEffect::getPackedOverlay(livingEntity, false);

    // 获取受伤进度
    f64 progress = HurtFlashEffect::getHurtProgress(livingEntity);

    // 应用闪烁效果到颜色（CPU端计算，备用）
    Vector4f flashColor = HurtFlashEffect::applyHurtFlash(livingEntity, baseColor);
}

// 清理（无资源需要释放）
HurtFlashEffect::cleanup();
```

**覆盖层UV计算**（保留用于兼容性）：

`getPackedOverlay()` 方法参考 MC 1.16.5 `OverlayTexture.java` 实现：

```
U = hurtTime / 10.0 * 15.0
V = (hurtTime > 0 || deathTime > 0) ? 3 : 10
packed = U | (V << 16)
```

**注意**：当前着色器实现不使用纹理采样，此方法保留用于：
1. 未来可能的功能扩展
2. 与 MC 1.16.5 代码对照参考
3. 其他层渲染器可能需要覆盖层数据

**受伤时间**：
- hurtTime 从 10 递减到 0
- 闪烁强度在受伤开始时最强

**颜色叠加**：
- 红色增加
- 绿色减少
- 蓝色减少

**数据流**：

```
LivingEntity.hurtTime()
        │
        ▼
EntityRendererManager::renderWithPipeline()
        │ 传递 hurtTime / 10.0f 归一化值
        ▼
EntityPipeline::drawMesh()
        │ 作为 push constant
        ▼
entity.vert → entity.frag
        │ 在片段着色器计算闪烁强度
        ▼
mix(color, hurtColor, intensity)
```

**参考**：
- MC 1.16.5 `LivingRenderer.getPackedOverlay()`
- MC 1.16.5 `OverlayTexture.java`
- 本项目 `shaders/entity.frag` 受伤效果实现

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::hurt {
    class HurtFlashEffect;
}
```

## 依赖关系

```
HurtFlashEffect.hpp
├── common/core/Types.hpp
└── common/util/math/Vector4.hpp

HurtFlashEffect.cpp
├── HurtFlashEffect.hpp
└── common/entity/core/LivingEntity.hpp
```

## 性能考虑

- **着色器方案优势**：无需额外纹理内存，无需 CPU-GPU 数据传输
- **纹理方案优势**：可实现更复杂的覆盖层效果（如道德效果渐变）
- 本项目选择着色器方案，在保持视觉效果的同时简化资源管理
