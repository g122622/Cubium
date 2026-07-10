# 怪物渲染器

本目录包含怪物实体的渲染器实现，所有渲染器继承自 `LivingRenderer` 基类。

## 目录结构

```
monster/
├── MonsterRenderers.hpp         # 基础怪物渲染器声明（僵尸、骷髅、苦力怕、蜘蛛、末影人、烈焰人）
├── MonsterRenderers.cpp         # 基础怪物渲染器实现
├── MonsterVariantRenderers.hpp  # 怪物变体渲染器（僵尸村民、溺尸、尸壳、流浪者、洞穴蜘蛛、巨人）
├── MonsterVariantRenderers.cpp  # 怪物变体渲染器实现（空文件，内联实现）
├── SpecialMonsterRenderers.hpp  # 特殊怪物渲染器（凋灵、史莱姆、守卫者、潜影贝、蠹虫、末影螨、灾厄村民等）
└── SpecialMonsterRenderers.cpp  # 特殊怪物渲染器实现（空文件，内联实现）
```

## 内部模块关系

```
                    ┌──────────────────┐
                    │  LivingRenderer  │ （core/）
                    │  <TEntity,TModel>│
                    └────────┬─────────┘
                             │ 继承
          ┌──────────────────┼──────────────────┐
          │                  │                  │
          ▼                  ▼                  ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│MonsterRenderers │ │MonsterVariant   │ │SpecialMonster   │
│ (基础怪物)       │ │Renderers (变体) │ │Renderers (特殊) │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
         └───────────────────┴───────────────────┘
                             │ 使用
                             ▼
              ┌──────────────────────────┐
              │     Layer 渲染器层        │
              │ (HeldItemLayer, EyesLayer│
              │  EnergyGlintLayer等)     │
              └──────────────────────────┘
```

## 上下游依赖关系

### 上游依赖（本目录依赖的模块）

- `core/LivingRenderer.hpp` - 生物渲染器基类，提供动画参数计算、模型渲染、层渲染器管理
- `core/EntityRendererManager.hpp` - 渲染器管理器，用于注册渲染器
- `model/monster/*.hpp` - 怪物模型类（ZombieModel、SkeletonModel 等）
- `layer/effect/*.hpp` - 效果层渲染器（EyesLayer、EnergyGlintLayer）
- `layer/equipment/*.hpp` - 装备层渲染器（HeldItemLayer）
- `layer/entity/*.hpp` - 实体层渲染器（HeldBlockLayer）
- `common/entity/entities/monster/*.hpp` - 怪物实体类（如 EndermanEntity）

### 下游依赖（依赖本目录的模块）

- `renderer/RendererRegistration.cpp` - 通过 `#include "monster/MonsterRenderers.hpp"` 等引入所有怪物渲染器并注册到 RendererFactory
- `renderer/RendererFactory.hpp` - 渲染器工厂，根据实体类型创建对应渲染器

## 容易踩的坑

- **层渲染器注册**：层渲染器在 `_setupLayers()` 中通过 `addLayer<T>()` 添加到 `m_layers`（LivingRenderer 基类管理）。`EndermanRenderer` 额外直接持有 `HeldBlockLayer`（`std::unique_ptr`），便于注入方块/实体纹理图集。
- **末影人状态更新（CPU 路径）**：`EndermanRenderer::render()` 需要先调用 `_updateEndermanState()` 更新模型状态（携带方块、尖叫），再调用基类 `render()`。这是 CPU 渲染路径。
- **末影人状态更新（GPU 管线路径）**：`EntityRendererManager::_createModelForEntity` 中的 enderman 分支通过 `ClientEntity::endermanHeldBlockState()`/`endermanScreaming()` 推送 `setCarrying`/`setAttacking` 到 `EndermanModel`，对应 CPU 路径的 `_updateEndermanState`。
- **末影人手持方块层调用链**：`EntityRendererManager::renderWithPipeline` → `EndermanRenderer::renderLayersPipelineClient(ClientEntity&, ...)` → `HeldBlockLayer::shouldRender/renderPipeline`。`HeldBlockLayer` 是 `LayerRenderer<ClientEntity>`，通过 `ClientEntity::endermanHeldBlockState()` 读取镜像字段（由 `EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM` 同步）。
- **末影人纹理图集注入**：`EndermanRenderer::setTextureAtlas`（实体图集）和 `setChunkTextureAtlas`（方块图集）将图集指针传递给 `HeldBlockLayer`。方块图集来自 `ChunkRenderer::textureAtlas()`，通过 `EntityRendererManager::setChunkTextureAtlas` 注入。`HeldBlockLayer` 渲染方块前切换到方块图集，渲染后恢复为实体图集。
- **阴影大小设置**：在构造函数中通过 `setShadowSize()` 设置，不同实体阴影大小不同（如蜘蛛 0.7，史莱姆 0.25，潜影贝/蠹虫/末影螨为 0）
- **纹理路径**：使用 `ResourceLocation("minecraft", "textures/entity/...")` 格式，注意纹理路径与 MC 1.16.5 一致
- **渲染器复用**：部分渲染器可复用同一模型但不同纹理（如 `WitherSkeletonRenderer` 复用 `SkeletonModel`）
- **内联实现**：`MonsterVariantRenderers.cpp` 和 `SpecialMonsterRenderers.cpp` 为空文件，所有渲染器类在头文件中内联实现
