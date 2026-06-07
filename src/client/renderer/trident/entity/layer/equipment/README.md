# 装备层渲染器

本目录包含装备相关的层渲染器，用于在实体模型上渲染盔甲、手持物品和头部物品。

## 目录结构

```
equipment/
├── ArmorLayer.hpp/cpp      # 盔甲层渲染器（支持皮革染色）
├── HeldItemLayer.hpp/cpp   # 手持物品层渲染器（主手/副手）
├── HeadLayer.hpp/cpp       # 头部物品层渲染器（头盔、南瓜等）
└── README.md
```

## 内部模块关系

三个层渲染器相互独立，均继承自 `core::LayerRenderer<TEntity>`：

```
LayerRenderer<TEntity> (基类)
        ↑
   ┌────┼────┐
   │    │    │
ArmorLayer  HeldItemLayer  HeadLayer
```

各层渲染器的职责：
- **ArmorLayer**: 渲染头盔、胸甲、护腿、靴子四部位盔甲
- **HeldItemLayer**: 渲染主手和副手物品，物品跟随手臂动画
- **HeadLayer**: 渲染头部槽位物品（南瓜、玩家头颅等）

## 上下游外部依赖关系

**被谁依赖（下游）：**
- `PlayerRenderer` - 玩家渲染器添加所有三层
- `MonsterRenderers` - 部分怪物渲染器（僵尸、骷髅等）添加盔甲层

**依赖了谁（上游）：**
- `core/LayerRenderer.hpp` - 层渲染器基类
- `pipeline/EntityPipeline.hpp` - GPU 渲染管线，用于 `createMesh()`、`updateMesh()`、`drawMesh()`
- `item/ItemMeshBuilder.hpp` - 物品网格构建器
- `model/base/BipedModel.hpp` - 双足模型（`translateHand()` 手臂变换）
- `AnimationContext` - 动画上下文（骨骼动画数据）
- `LivingEntity` - 生物实体（装备槽位访问）

## 容易踩的坑

1. **ArmorLayer 渲染顺序**：必须按 Chest → Legs → Feet → Head 顺序渲染，确保正确的图层叠加效果。

2. **HeldItemLayer 主手判断**：需要根据 `entity.isRightHanded()` 判断主手位置，右手主手时右手渲染主手物品，左手主手时相反。

3. **网格缓存失效**：ArmorLayer 和 HeldItemLayer 使用静态缓存（按 itemId），当物品属性变化但 itemId 不变时（如耐久度），缓存不会更新。

4. **CPU 路径已废弃**：三个层渲染器的 `render()` 方法已废弃，只实现 `renderPipeline()` 方法使用 GPU 管线路径。

5. **HeadLayer 头部变换**：优先使用 `parentModel->getModelHead()->getTransformMatrix()` 获取头部变换，仅在无父模型时回退到 `computeHeadTransform()` 硬编码变换。

## 参考

- MC 1.16.5 BipedArmorLayer
- MC 1.16.5 HeldItemLayer
- MC 1.16.5 HeadLayer
