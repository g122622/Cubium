# 着火效果

本目录包含实体着火效果实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `FireEffect.hpp` | 着火效果头文件 |
| `FireEffect.cpp` | 着火效果实现 |

## 功能详解

### FireEffect（着火效果）

用于渲染实体身上的火焰效果。

**使用方法**：

```cpp
// 初始化
FireEffect::initialize();

// 检查实体是否燃烧
if (FireEffect::isBurning(entity)) {
    // 渲染火焰效果
    FireEffect::renderFire(entity, partialTicks);
}

// 清理
FireEffect::cleanup();
```

**火焰渲染**：
- 在实体底部放置火焰四边形
- 使用动画纹理创建摇曳效果
- 火焰广告牌效果（始终面向摄像机）

**火焰位置**：
- 底部：实体边界框底部
- 两侧：实体边界框两侧
- 高度：根据实体高度调整

**动画效果**：
- UV 动画：火焰纹理滚动
- 位置偏移：火焰摇曳

**参考**：MC 1.16.5 EntityRenderer.renderFire()

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::fire {
    class FireEffect;
}
```

## 依赖关系

```
FireEffect.hpp
├── Types.hpp
└── Vector3.hpp

FireEffect.cpp
├── FireEffect.hpp
└── Entity.hpp
```
