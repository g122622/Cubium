# 层渲染器系统

本目录包含实体层渲染器，用于在基础模型上添加额外渲染层（盔甲、手持物品、披风、特效等）。

## 目录结构

```
layer/
├── core/                      # 层渲染器核心
│   ├── LayerRenderer.hpp      # 层渲染器基类模板
│   └── README.md
├── equipment/                 # 装备层渲染器
│   ├── ArmorLayer.hpp/cpp     # 盔甲层（支持皮革染色）
│   ├── HeldItemLayer.hpp/cpp  # 手持物品层（主手/副手）
│   ├── HeadLayer.hpp/cpp      # 头部物品层（头盔、南瓜等）
│   └── README.md
├── cosmetic/                  # 外观层渲染器
│   ├── CapeLayer.hpp/cpp      # 斗篷层（动态摆动动画）
│   ├── ElytraLayer.hpp/cpp    # 鞘翅层（滑翔展开动画）
│   └── README.md
├── entity/                    # 实体特性层渲染器
│   ├── ArrowLayer.hpp/cpp     # 箭矢附着层
│   ├── HeldBlockLayer.hpp/cpp # 方块持有层（末影人）
│   ├── SaddleLayer.hpp/cpp    # 鞍层（可骑乘实体）
│   ├── SheepWoolLayer.hpp/cpp # 羊毛层（支持染色和彩虹羊）
│   ├── VillagerLayer.hpp      # 村民多层纹理层
│   ├── WolfCollarLayer.hpp/cpp # 狼项圈层
│   └── README.md
└── effect/                    # 效果层渲染器
    ├── EnergyGlintLayer.hpp/cpp # 附魔光效层
    ├── EyesLayer.hpp/cpp      # 发光眼睛层（蜘蛛、末影人等）
    └── README.md
```

## 内部模块关系

```
core::LayerRenderer<TEntity> (基类)
        ↑
   ┌────┼────────────┬────────────┬────────────┐
   │    │            │            │            │
equipment/       cosmetic/     entity/      effect/
ArmorLayer       CapeLayer     SaddleLayer   EnergyGlintLayer
HeldItemLayer    ElytraLayer   SheepWoolLayer EyesLayer
HeadLayer                      VillagerLayer
                               ArrowLayer
                               HeldBlockLayer
                               WolfCollarLayer
```

所有层渲染器均继承自 `core::LayerRenderer<TEntity>` 基类：
- `renderPipeline()` - GPU 管线路径渲染（主要方法，所有子类必须实现）
- `shouldRender()` - 条件渲染检查

注意：旧的 CPU 路径 `render()` 方法已从所有子类中移除，基类保留空实现以兼容旧代码路径。

## 上下游外部依赖关系

**依赖了谁（上游）：**
- `core/LayerRenderer.hpp` - 所有层渲染器的基类
- `core/AnimationContext.hpp` - 动画上下文（骨骼动画数据）
- `core/IEntityRenderer.hpp` - 实体渲染器接口（获取父模型）
- `pipeline/EntityPipeline.hpp` - 实体渲染管线（GPU 网格创建和绘制）
- `model/core/ModelRenderer.hpp` - 模型渲染器（ModelVertex）
- 各实体类：`LivingEntity`, `Player`, `SheepEntity`, `WolfEntity` 等

**被谁依赖（下游）：**
- `LivingRenderer` 及其子类 - 在 `_setupLayers()` 中添加层渲染器
- `PlayerRenderer` - 添加 HeldItemLayer、HeadLayer、CapeLayer、ElytraLayer、ArmorLayer
- `SheepRenderer` - 添加 SheepWoolLayer
- `WolfRenderer` - 添加 WolfCollarLayer
- `VillagerRenderer` - 添加 VillagerLayer
- `EndermanRenderer` - 添加 HeldBlockLayer、EyesLayer
- `CreeperRenderer` - 添加 EnergyGlintLayer
- `SpiderRenderer` - 添加 EyesLayer

## 渲染顺序

层渲染器按添加顺序依次渲染，MC 1.16.5 PlayerRenderer 层顺序：
1. HeldItemLayer → 2. HeadLayer → 3. CapeLayer → 4. ElytraLayer

**注意**：披风在鞘翅之前渲染，这样鞘翅可以正确覆盖披风。

## 容易踩的坑

1. **CPU 路径已移除**：所有层渲染器的 `render()` 方法已从子类中移除，所有渲染逻辑均在 `renderPipeline()` GPU 管线路径中实现。基类 `LayerRenderer::render()` 保留空实现以兼容 `LivingRenderer::render()` 旧路径。

2. **模板类显式实例化**：`ElytraLayer`、`SaddleLayer`、`SheepWoolLayer`、`ArrowLayer`、`EnergyGlintLayer`、`EyesLayer` 均为模板类，需在 cpp 末尾显式实例化。`HeldBlockLayer` 和 `WolfCollarLayer` 已迁移为 `LayerRenderer<ClientEntity>` 非模板类，无需显式实例化。

3. **ClientEntity 层 vs LivingEntity 层**：`HeldBlockLayer` 和 `WolfCollarLayer` 模板参数为 `ClientEntity`，通过 `ClientEntity` 的元数据镜像字段读取状态（如 `endermanHeldBlockState()`、`wolfTamed()`）。其他层（如 `SaddleLayer`、`SheepWoolLayer`）模板参数为 `LivingEntity` 或其派生类。`ClientEntity` 层由渲染器的 `renderLayersPipelineClient(ClientEntity&, ...)` 分发，`LivingEntity` 层由 `LivingRenderer::renderLayersPipeline(Entity&, ...)` 分发。

3. **shouldRender 条件检查**：层渲染器必须重写 `shouldRender()` 来检查渲染条件（如装备槽位是否有物品、是否开启了特定选项），避免不必要的渲染调用。

4. **叠加混合模式必须恢复**：`EnergyGlintLayer` 和 `EyesLayer` 使用 `BlendMode::Additive`，渲染完成后必须恢复 `BlendMode::Alpha`。

5. **更多细节**：各子目录的 README.md 有更详细的坑点说明，开发前务必阅读。
