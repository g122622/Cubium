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

**使用方法**：

```cpp
// 初始化
HurtFlashEffect::initialize();

// 检查实体是否受伤
if (HurtFlashEffect::isHurt(livingEntity)) {
    // 获取覆盖层UV
    i32 packedOverlay = HurtFlashEffect::getPackedOverlay(livingEntity, false);

    // 获取受伤进度
    f64 progress = HurtFlashEffect::getHurtProgress(livingEntity);

    // 应用闪烁效果到颜色
    Vector4f flashColor = HurtFlashEffect::applyHurtFlash(livingEntity, baseColor);
}

// 清理
HurtFlashEffect::cleanup();
```

**覆盖层UV计算**：
```
U = hurtTime / 10.0 * 16.0
V = 0

道德影响时 U = 3.0
```

**打包格式**：
```
packed = (int)(u * 16) << 16 | (int)(v * 16)
```

**受伤时间**：
- hurtTime 从 10 递减到 0
- 闪烁强度在受伤开始时最强

**颜色叠加**：
- 红色增加
- 绿色减少
- 蓝色减少

**参考**：MC 1.16.5 LivingRenderer.getPackedOverlay()

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::hurt {
    class HurtFlashEffect;
}
```

## 依赖关系

```
HurtFlashEffect.hpp
├── Types.hpp
└── LivingEntity.hpp

HurtFlashEffect.cpp
├── HurtFlashEffect.hpp
└── LivingEntity.hpp
```
