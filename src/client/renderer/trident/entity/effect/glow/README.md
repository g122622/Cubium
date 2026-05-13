# 发光效果

本目录包含发光轮廓效果实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `GlowEffect.hpp` | 发光效果头文件 |
| `GlowEffect.cpp` | 发光效果实现 |

## 功能详解

### GlowEffect（发光效果）

用于渲染实体的发光轮廓，如：
- 发光鱿鱼（Glow Squid）
- 发光药水效果（Glowing Effect）
- 团队发光颜色

**发光效果来源检测**：

```cpp
bool GlowEffect::hasGlowEffect(Entity& entity) {
    // 1. 检查 Entity 的发光标志
    if (entity.isGlowing()) return true;

    // 2. 检查发光药水效果（仅 LivingEntity）
    if (auto* living = dynamic_cast<LivingEntity*>(&entity)) {
        if (living->hasEffect(::mc::entity::effect::EffectType::Glowing)) return true;
    }

    return false;
}
```

**发光效果来源**：
1. `Entity::setGlowing(true)` - 直接设置发光标志
2. 发光药水效果（`EffectType::Glowing`）
3. 发光鱿鱼实体类型（未实现）
4. 团队发光规则（未实现）

**使用方法**：

```cpp
// 初始化
GlowEffect::initialize();

// 检查实体是否发光
if (GlowEffect::hasGlowEffect(entity)) {
    // 获取发光颜色
    Vector4f color = GlowEffect::getGlowColor(entity);

    // 渲染发光轮廓
    GlowEffect::renderGlow(entity, partialTicks, color);
}

// 渲染所有发光实体
GlowEffect::renderAllGlowing(partialTicks);

// 清理
GlowEffect::cleanup();
```

**发光颜色**：
- 默认：白色 (1, 1, 1, 1)
- 发光鱿鱼：青色
- 团队成员：团队颜色

**渲染流程**：
1. 渲染实体到发光缓冲区
2. 应用模糊和膨胀效果
3. 将轮廓合成到主画面

**参考**：MC 1.16.5 发光轮廓渲染系统

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::glow {
    class GlowEffect;
}
```

## 依赖关系

```
GlowEffect.hpp
├── Types.hpp
└── Vector3.hpp

GlowEffect.cpp
├── GlowEffect.hpp
└── Entity.hpp
```
