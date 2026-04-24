# 怪物渲染器

本目录包含怪物实体的渲染器实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `MonsterRenderers.hpp` | 怪物渲染器头文件集合 |
| `MonsterRenderers.cpp` | 怪物渲染器实现 |

## 渲染器详解

### ZombieRenderer（僵尸渲染器）

继承自 `LivingRenderer<LivingEntity, ZombieModel>`。

**特点**：
- 阴影大小：0.5
- 纹理：`minecraft:textures/entity/zombie/zombie.png`

**层渲染器**：
- `ArmorLayer` - 盔甲层
- `HeldItemLayer` - 手持物品层
- `HeadLayer` - 头部物品层

**参考**：MC 1.16.5 ZombieRenderer

### SkeletonRenderer（骷髅渲染器）

继承自 `LivingRenderer<LivingEntity, SkeletonModel>`。

**特点**：
- 阴影大小：0.5
- 纹理：`minecraft:textures/entity/skeleton/skeleton.png`

**层渲染器**：
- `ArmorLayer` - 盔甲层
- `HeldItemLayer` - 手持物品层

**参考**：MC 1.16.5 SkeletonRenderer

### CreeperRenderer（苦力怕渲染器）

继承自 `LivingRenderer<LivingEntity, CreeperModel>`。

**特点**：
- 阴影大小：0.5
- 纹理：`minecraft:textures/entity/creeper/creeper.png`

**层渲染器**：
- `EnergyGlintLayer` - 发光效果（充能状态）

**参考**：MC 1.16.5 CreeperRenderer

### SpiderRenderer（蜘蛛渲染器）

继承自 `LivingRenderer<LivingEntity, SpiderModel>`。

**特点**：
- 阴影大小：0.7（蜘蛛阴影稍大）
- 纹理：`minecraft:textures/entity/spider/spider.png`

**层渲染器**：
- `EyesLayer` - 发光眼睛
- `SaddleLayer` - 鞍（洞穴蜘蛛）

**参考**：MC 1.16.5 SpiderRenderer

### EndermanRenderer（末影人渲染器）

继承自 `LivingRenderer<LivingEntity, EndermanModel>`。

**特点**：
- 阴影大小：0.5
- 纹理：`minecraft:textures/entity/enderman/enderman.png`
- 特殊状态：携带方块、尖叫/攻击

**层渲染器**：
- `HeldItemLayer` - 手持方块
- `EyesLayer` - 发光眼睛

**状态更新**：
```cpp
void updateEndermanState(LivingEntity& entity) {
    m_model.setCarrying(entity.isCarrying());
    m_model.setAttacking(entity.isScreaming());
}
```

**参考**：MC 1.16.5 EndermanRenderer

## 命名空间

```cpp
namespace mc::client::renderer::entity::renderer::monster {
    class ZombieRenderer;
    class SkeletonRenderer;
    class CreeperRenderer;
    class SpiderRenderer;
    class EndermanRenderer;
}
```

## 注册渲染器

```cpp
#include "MonsterRenderers.hpp"

// 注册所有怪物渲染器
mc::client::renderer::entity::renderer::monster::registerMonsterRenderers();
```

## 依赖关系

```
MonsterRenderers.hpp
├── core/LivingRenderer.hpp
├── model/monster/ZombieModel.hpp
├── model/monster/SkeletonModel.hpp
├── model/monster/CreeperModel.hpp
├── model/monster/SpiderModel.hpp
└── model/monster/EndermanModel.hpp

MonsterRenderers.cpp
├── MonsterRenderers.hpp
└── core/EntityRendererManager.hpp
```

## 使用示例

```cpp
// 创建渲染器
auto zombieRenderer = std::make_unique<ZombieRenderer>();

// 渲染僵尸
zombieRenderer->render(zombieEntity, partialTicks);

// 获取纹理
auto texture = zombieRenderer->getEntityTexture(zombieEntity);
```

## 扩展渲染器

添加新的怪物渲染器：

1. 在 `model/monster/` 创建模型类
2. 在 `MonsterRenderers.hpp` 添加渲染器类声明
3. 在 `MonsterRenderers.cpp` 实现渲染器
4. 在 `registerMonsterRenderers()` 中注册

## 注意事项

- 所有渲染器继承自 `LivingRenderer`
- 需要实现 `getEntityTexture()` 方法
- 层渲染器在 `setupLayers()` 中添加
- 阴影大小和透明度在构造函数中设置
