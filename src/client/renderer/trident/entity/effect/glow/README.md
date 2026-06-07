# 发光效果

本目录实现实体发光轮廓效果，用于发光药水效果和团队发光颜色渲染。

## 目录结构

```
glow/
├── GlowEffect.hpp     # 发光效果管理器（静态工具类）
├── GlowEffect.cpp     # 发光效果实现
└── README.md          # 本文档
```

## 内部模块关系

`GlowEffect` 是纯静态工具类，无法实例化，所有方法均为静态方法：

```
GlowEffect（静态工具类）
├── initialize()           # 初始化发光效果系统
├── cleanup()              # 清理发光效果系统
├── hasGlowEffect()        # 检测实体是否发光
├── getGlowColor()         # 获取发光颜色
├── renderGlow()           # 渲染单个实体发光轮廓
├── renderAllGlowing()     # 渲染所有发光实体
└── _generateGlowMesh()    # 生成发光轮廓网格（内部方法）
```

## 上下游外部依赖关系

### 上游依赖（谁依赖了这个目录）

- `entity/effect/` 父目录的特效系统集成
- 实体渲染循环调用 `hasGlowEffect()` 和 `renderGlow()`

### 下游依赖（这个目录依赖了谁）

```
GlowEffect.hpp
├── ModelRenderer.hpp      # 模型顶点类型
├── Types.hpp              # 基础类型（f64等）
└── Vector4.hpp            # 颜色向量

GlowEffect.cpp
├── Entity.hpp             # 实体基类（isGlowing(), getTeam()）
├── LivingEntity.hpp       # 生物实体（hasEffect()）
├── EffectType.hpp         # 效果类型枚举（Glowing）
├── Team.hpp               # 团队类（getColor()）
└── TextStyle.hpp          # 颜色转换（getFormattingColor(), isColor()）
```

## 容易踩的坑

1. **发光鱿鱼不存在于 MC 1.16.5**：发光鱿鱼（Glow Squid）是 MC 1.17+ 添加的实体，本项目目标版本为 MC 1.16.5，不要错误引用。

2. **团队颜色获取链**：`Entity::getTeam()` 在基类中默认返回 `nullptr`，只有 `ServerPlayer` 重写了该方法。客户端实体需要通过其他方式获取团队信息。

3. **后处理管线尚未完成**：当前 `renderGlow()` 和 `renderAllGlowing()` 仅有框架代码，等待渲染管线支持多渲染目标(MRT)和模糊着色器后才能完整实现。

4. **初始化必须在使用前调用**：调用任何渲染方法前必须先调用 `initialize()`，否则不会生效。

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::glow {
    class GlowEffect;
}
```
