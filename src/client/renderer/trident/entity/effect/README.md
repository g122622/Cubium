# 实体特效系统

本目录包含实体渲染特效的实现。

## 目录结构

```
effect/
├── glow/              # 发光效果
│   ├── GlowEffect.hpp
│   ├── GlowEffect.cpp
│   └── README.md
├── fire/              # 着火效果
│   ├── FireEffect.hpp
│   ├── FireEffect.cpp
│   └── README.md
├── hurt/              # 受伤闪烁
│   ├── HurtFlashEffect.hpp
│   ├── HurtFlashEffect.cpp
│   └── README.md
└── README.md          # 本文档
```

## 特效详解

### GlowEffect（发光效果）

用于渲染实体的发光轮廓。

**用途**：
- 发光鱿鱼（Glow Squid）
- 发光药水效果（Glowing Effect）
- 团队发光颜色

**渲染流程**：
1. 渲染实体到发光缓冲区
2. 应用模糊和膨胀效果
3. 将轮廓合成到主画面

**命名空间**：`mc::client::renderer::entity::effect::glow`

### FireEffect（着火效果）

用于渲染实体身上的火焰效果。

**火焰位置**：
- 底部：实体边界框底部
- 两侧：实体边界框两侧

**动画效果**：
- UV 动画：火焰纹理滚动
- 位置偏移：火焰摇曳

**命名空间**：`mc::client::renderer::entity::effect::fire`

### HurtFlashEffect（受伤闪烁）

用于渲染实体受伤时的红色闪烁效果。

**覆盖层UV计算**：
```
U = hurtTime / 10.0 * 16.0
V = 0
道德影响时 U = 3.0
```

**命名空间**：`mc::client::renderer::entity::effect::hurt`

## 使用示例

```cpp
#include "glow/GlowEffect.hpp"
#include "fire/FireEffect.hpp"
#include "hurt/HurtFlashEffect.hpp"

// 初始化所有特效
GlowEffect::initialize();
FireEffect::initialize();
HurtFlashEffect::initialize();

// 渲染循环中
void renderEntity(Entity& entity, f64 partialTicks) {
    // 检查受伤闪烁
    if (auto* living = dynamic_cast<LivingEntity*>(&entity)) {
        if (HurtFlashEffect::isHurt(*living)) {
            i32 overlay = HurtFlashEffect::getPackedOverlay(*living);
            // 应用覆盖层到渲染
        }
    }

    // 渲染实体模型
    // ...

    // 检查燃烧效果
    if (FireEffect::isBurning(entity)) {
        FireEffect::renderFire(entity, partialTicks);
    }

    // 检查发光效果
    if (GlowEffect::hasGlowEffect(entity)) {
        Vector4f color = GlowEffect::getGlowColor(entity);
        GlowEffect::renderGlow(entity, partialTicks, color);
    }
}

// 清理
GlowEffect::cleanup();
FireEffect::cleanup();
HurtFlashEffect::cleanup();
```

## 渲染顺序

实体特效的推荐渲染顺序：

1. **主体渲染**：渲染实体模型
2. **受伤闪烁**：在模型渲染时应用覆盖层
3. **着火效果**：在实体底部渲染火焰
4. **发光效果**：后处理生成发光轮廓

## 依赖关系

```
effect/
├── glow/
│   ├── Types.hpp
│   ├── Vector3.hpp
│   └── Entity.hpp
├── fire/
│   ├── Types.hpp
│   ├── Vector3.hpp
│   └── Entity.hpp
└── hurt/
    ├── Types.hpp
    └── LivingEntity.hpp
```

## 扩展特效

添加新特效的步骤：

1. 在 `effect/` 下创建新目录（如 `freeze/`）
2. 创建特效类头文件和实现文件
3. 实现静态方法：
   - `initialize()` - 初始化资源
   - `cleanup()` - 清理资源
   - `render()` - 渲染特效
4. 创建 `README.md` 文档
5. 在主渲染流程中集成新特效

## 性能考虑

- **发光效果**：需要额外的帧缓冲区，开销较大
- **着火效果**：粒子动画，需要限制火焰数量
- **受伤闪烁**：仅修改UV，开销最小

## 参考

所有特效参考 MC 1.16.5 对应实现：
- 发光效果：`RenderType.getOutline()`
- 着火效果：`EntityRenderer.renderFire()`
- 受伤闪烁：`LivingRenderer.getPackedOverlay()`
