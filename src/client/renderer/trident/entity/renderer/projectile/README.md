# 投掷物渲染器

本目录包含投掷物类实体的渲染器实现。

## 目录结构

```
projectile/
├── BillboardRenderers.hpp/cpp        # Billboard 渲染器基类及物品投掷物（雪球、鸡蛋、末影珍珠等）
├── ExperienceOrbRenderer.hpp/cpp     # 经验球渲染器（颜色动画、图标UV、浮动动画）
├── FireballRenderers.hpp/cpp         # 火球渲染器（恶魂火球、烈焰人小火球）
├── FishingBobberRenderer.hpp/cpp     # 钓鱼浮标渲染器
├── ItemEntityRenderer.hpp/cpp        # 物品实体渲染器（掉落物）
├── ProjectileRenderers.hpp/cpp       # 箭矢、光灵箭、三叉戟渲染器
└── README.md
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │  EntityRenderer │ ← core/EntityRenderer.hpp (基类)
                    │  (抽象基类)      │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ItemEntityRenderer│ │ExperienceOrb    │ │ItemBillboard    │
│                 │ │Renderer         │ │Renderer         │
│ (掉落物3D渲染)   │ │ (经验球Billboard)│ │ (抽象基类)       │
└─────────────────┘ └─────────────────┘ └────────┬────────┘
                                                │
                     ┌──────────────────────────┴──────────────────────┐
                     │                     │                           │
                     ▼                     ▼                           ▼
          ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
          │SnowballRenderer  │   │EyeOfEnderRenderer│   │FireballRenderer  │
          │EggRenderer       │   │(fullbright=true) │   │SmallFireball     │
          │EnderPearlRenderer│   └──────────────────┘   │Renderer         │
          │PotionRenderer    │                          │(fullbright=true)│
          │ExperienceBottle  │                          └──────────────────┘
          │(均继承自         │
          │ItemBillboard)    │
          └──────────────────┘

ArrowRenderer / SpectralArrowRenderer / TridentRenderer：独立实现，直接继承 EntityRenderer
FishingBobberRenderer：独立实现，使用 LINE_LIST 拓扑渲染浮标和钓线
```

## 上下游外部依赖关系

**上游（本目录依赖）：**
- `core/EntityRenderer.hpp` - 渲染器基类
- `core/PipelineMeshProvider` - 自定义网格生成接口（FishingBobber、ItemBillboard 使用）
- `pipeline/EntityTextureAtlas.hpp` - 实体纹理图集
- `model/core/ModelRenderer.hpp` - 模型渲染器

**下游（依赖本目录）：**
- `core/EntityRendererManager.cpp` - 渲染器注册与管理
- `renderer/RendererRegistration.cpp` - 渲染器注册入口

## 容易踩的坑

1. **ItemEntityRenderer 需要设置纹理图集**：使用前必须调用 `setItemTextureAtlas()` 设置物品纹理图集，否则无法获取物品纹理

2. **经验球渲染由管线管理器接管**：`ExperienceOrbRenderer::render()` 当前为空操作，实际渲染逻辑（颜色动画、图标UV映射、浮动、缩放）由 `EntityRendererManager::renderWithPipeline()` 中的 ExperienceOrb 特殊路径处理。颜色动画通过 `overlayColor` 传入着色器（注意：当前使用 `mix()` 线性混合，与 MC 原版顶点颜色乘法 `texColor * vertexColor` 有差异，TODO: 后续应改为顶点颜色乘法以完全对齐 MC）

3. **经验球纹理为 64×64 精灵图集**：`experience_orb.png` 包含 4列×3行共 11 个 16×16 图标（索引 0-10），UV 映射由 `ExperienceOrbRenderer::calculateIconUV()` 根据 XP 值选择图标。XP 值变化（合并）时通过 `xpOrbIconIndex` 自动触发网格重建

4. **经验球光照是增强的**：`ExperienceOrbRenderer` 使用增强光照（fullbright 因子 7/15 ≈ 0.467），对应 MC Java 中 `getBlockLightLevel() = clamp(worldLight + 7, 0, 15)` 的行为，确保经验球在黑暗中也有一定可见度

5. **Billboard 渲染器的 fullbright 参数**：继承 `ItemBillboardRenderer` 时需注意 fullbright 参数，末影之眼、火球等发光实体需要 `fullbright=true`。fullbright 通过 `EntityRenderer::isFullbright()` 虚方法传递到渲染管线，在着色器中将光照混合到最大亮度 1.0，使实体在黑暗中也清晰可见。对应 MC Java 中 `EntityRenderer.getBlockLightLevel()` 返回 15 的行为。

6. **FishingBobberRenderer 使用 LINE_LIST 拓扑**：与其他渲染器不同，钓线使用线段而非三角形渲染，需注意管线配置。

   **网络同步状态驱动渲染**：`generateMesh()` 通过 `ClientEntity::fishingBiting()` 与 `ClientEntity::fishingHookedEntityId()` 读取服务端同步过来的镜像字段（对应 MC 1.21.11 `FishingHook.onSyncedDataUpdated()`）：
   - `fishingBiting() == true`（咬钩，由 `DATA_BITING_PARAM` 同步）：浮标 Y 偏移从 0.25 下沉到 0.15，模拟 MC 中 `DATA_BITING` 触发的 `-0.4*random[0.6,1.0]` 向下速度造成的视觉下沉；钓线端点同步下沉。
   - `fishingHookedEntityId() > 0`（钩住实体，由 `DATA_HOOKED_ENTITY_PARAM` 同步，+1 偏移）：钓线绷紧（下垂量减半），模拟钓线连接到附近被钩实体而非远端玩家。
   - 镜像字段由 `ClientEntity::syncMetadataFromDataManager()` 的 `fishing_bobber` 分支从 `ir::play::SetEntityData` 反序列化写入。

   **已知限制**（TODO）：`PipelineMeshProvider::generateMesh` 接口仅传入 `ClientEntity&`，无世界查找回调，因此钓线另一端目前仍使用固定偏移占位（浮标上方 1.5 格），而非实际的玩家手持位置或被钩实体位置。后续若扩展接口传入世界查找回调，可据此解析 `fishingHookedEntityId()` 对应的实体位置并将钓线连接到该实体。

7. **ItemEntity 动画参数是硬编码的**：浮动周期 `/10`、旋转周期 `/20`、基础偏移 `0.25` 等常量在 ItemEntityRenderer 中定义，修改需同步 MC 原版

## 参考

- MC 1.16.5 ItemEntityRenderer
- MC 1.16.5 ExperienceOrbRenderer
- MC 1.16.5 ArrowRenderer / TridentRenderer
- MC 1.16.5 FishingBobberRenderer
