# 特殊实体渲染器（Special Entity Renderers）

本目录包含难以归类到 animal/monster/projectile/vehicle 的特殊实体渲染器，涵盖末影水晶、潜影贝子弹、闪电、下落方块、TNT、物品展示框、画、栓绳结、盔甲架、烟花火箭等。

## 目录结构

```
special/
└── SpecialEntityRenderers.hpp/cpp  # 所有特殊实体渲染器定义与实现
```

## 渲染器清单

| 渲染器 | 对应 MC 1.21.11 | 实现状态 | 说明 |
|--------|----------------|---------|------|
| `EnderCrystalRenderer` | `EndCrystalRenderer` | 部分实现 | 末影水晶旋转动画，TODO 浮动偏移与光束 |
| `ShulkerBulletRenderer` | `ShulkerBulletRenderer` | 已实现 | 潜影贝子弹，`isFullbright()` 返回 true |
| `LlamaSpitRenderer` | `LlamaSpitRenderer` | 已实现 | 羊驼唾沫投射物 |
| `SpectralArrowRenderer` | `SpectralArrowRenderer` | 已实现 | 光灵箭 |
| `WitherSkullRenderer` | `WitherSkullRenderer` | 已实现 | 凋灵之首，`isFullbright()` 返回 true |
| `DragonFireballRenderer` | `DragonFireballRenderer` | 已实现 | 龙火球，`isFullbright()` 返回 true |
| `EvokerFangsRenderer` | `EvokerFangsRenderer` | 已实现 | 唤魔者尖牙 |
| `LightningBoltRenderer` | `LightningBoltRenderer` | 已实现 | 闪电（程序化生成顶点，GPU 管线路径） |
| `AreaEffectCloudRenderer` | `AreaEffectCloudRenderer` | 部分实现 | 区域效果云 |
| `FallingBlockRenderer` | `FallingBlockRenderer` | **已完整实现** | 下落方块（沙子、砾石、铁砧等） |
| `ItemFrameRenderer` | `ItemFrameRenderer` | TODO | 物品展示框 |
| `PaintingRenderer` | `PaintingRenderer` | TODO | 画 |
| `LeashKnotRenderer` | `LeashKnotRenderer` | TODO | 栓绳结 |
| `ArmorStandRenderer` | `ArmorStandRenderer` | TODO | 盔甲架 |
| `TNTRenderer` | `TntRenderer` | **已完整实现** | 点燃的 TNT 实体（含闪烁缩放与白色闪烁） |
| `FireworkRocketRenderer` | `FireworkRocketRenderer` | 部分实现 | 烟花火箭 |

## 内部模块关系

```
┌──────────────────────────────────────────────────────────────────┐
│ SpecialEntityRenderers                                           │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│ 【基于模型的简单渲染器】                                          │
│ EnderCrystalRenderer / ShulkerBulletRenderer /                   │
│ LlamaSpitRenderer / SpectralArrowRenderer /                      │
│ WitherSkullRenderer / DragonFireballRenderer /                   │
│ EvokerFangsRenderer                                              │
│   └── 持有 model::projectile::*Model，调用 setAngles + render     │
│                                                                  │
│ 【GPU 管线渲染器（renderLayersPipelineClient 完成全部渲染）】     │
│ FallingBlockRenderer                                             │
│   ├── entity.fallingBlockState() 读取方块状态                    │
│   ├── BlockMeshBuilder::buildBlockMesh() 构建方块网格             │
│   ├── buildFallingBlockModelMatrix() translate(-0.5,0,-0.5)      │
│   ├── 切换到 ChunkTextureAtlas 渲染后恢复 EntityTextureAtlas      │
│   └── drawMesh(overlayColor=方块tint)                             │
│                                                                  │
│ TNTRenderer                                                      │
│   ├── entity.tntBlockState() 读取方块状态                        │
│   ├── entity.tntFuse() 读取引信剩余 tick                          │
│   ├── BlockMeshBuilder::buildBlockMesh() 构建方块网格             │
│   ├── buildTntModelMatrix(fuse) 完整 PoseStack 变换链             │
│   │   └── 内部调用 calculateTntFlashScale(fuse) 应用闪烁缩放      │
│   ├── isTntFlashFrame(fuse) 判定白色闪烁帧                        │
│   ├── 切换到 ChunkTextureAtlas 渲染后恢复 EntityTextureAtlas      │
│   └── drawMesh(overlayColor=tint或白色0.5alpha)                   │
│                                                                  │
│ LightningBoltRenderer                                            │
│   ├── _generateLightningMesh() 程序化生成闪电四边形条带           │
│   └── renderLayersPipelineClient 直接提交 GPU 管线                │
└──────────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `core/EntityRenderer.hpp` | 实体渲染器基类 |
| `core/AnimationContext.hpp` | 动画上下文（partialTicks 等） |
| `pipeline/EntityPipeline.hpp` | GPU 渲染管线（drawMesh、createMesh、setTextureAtlas） |
| `pipeline/EntityTextureAtlas.hpp` | 实体纹理图集（渲染后恢复） |
| `util/BlockMeshBuilder.hpp` | 方块网格构建（FallingBlock/TNT 复用） |
| `trident/chunk/ChunkMesher.hpp` | `getDefaultBlockTintColor()` 方块着色颜色 |
| `trident/chunk/ChunkRenderer.hpp` | `ChunkTextureAtlas` 方块纹理图集结构 |
| `model/projectile/ProjectileModels.hpp` | 末影水晶/潜影贝子弹等模型 |
| `client/world/entity/ClientEntity.hpp` | 客户端实体（fallingBlockState/tntBlockState/tntFuse） |
| `common/entity/entities/effect/EffectEntities.hpp` | 闪电/区域效果云等实体类型 |
| `common/util/math/MathUtils.hpp` | `clamp` 等数学工具 |
| `common/util/math/Vector4.hpp` | `Vector4f`（overlayColor 通道） |
| `common/world/block/Block.hpp` | `BlockState` |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `renderer/RendererRegistration.cpp` | 通过工厂注册所有特殊实体渲染器 |
| `core/EntityRendererManager.cpp` | 调用 `renderLayersPipelineClient()` / 注入 `ChunkTextureAtlas` / `EntityTextureAtlas` |

## FallingBlockRenderer 实现要点

### 数据流

```
ClientEntity.fallingBlockState() ──► BlockState*
                                         │
                                         ▼
                          BlockMeshBuilder::buildBlockMesh()
                                         │
                                         ▼
                              ModelVertex[] + u32[] indices
                                         │
                                         ▼
                   pipeline.createMesh() ──► EntityMesh*（按 BlockState* 缓存）
                                         │
                                         ▼
                   drawMesh(modelMatrix, position, overlayColor=tint)
```

### 变换链

```
M = translate(-0.5, 0, -0.5)
```

方块网格顶点已在 `BlockMeshBuilder` 中乘以 1/16 转换为世界单位（0-1 范围）。实体原点位于方块底部中心，`translate(-0.5, 0, -0.5)` 将方块底部中心对齐实体原点。

### 纹理图集切换

方块纹理 UV 基于 `ChunkTextureAtlas`（区块纹理图集），而非实体纹理图集。渲染前通过 `pipeline.setTextureAtlas()` 切换，渲染后恢复为 `EntityTextureAtlas`，避免污染后续实体渲染。

## TNTRenderer 实现要点

### 数据流

```
ClientEntity.tntBlockState() ──► BlockState*
ClientEntity.tntFuse()        ──► i32 fuse
                                     │
                                     ▼
                   fuseRemaining = fuse - partialTicks + 1
                                     │
                    ┌────────────────┼────────────────┐
                    ▼                                 ▼
        calculateTntFlashScale(fuse)      isTntFlashFrame(fuse)
        = 1 + (1-fuse/10)^4 * 0.3         = (fuse > -1) && ((int)fuse/5 % 2 == 0)
                    │                                 │
                    ▼                                 ▼
        buildTntModelMatrix(fuse)         overlayColor = (1,1,1,0.5) 白色闪烁
        ┌──────────────────────┐          （否则用方块 tint 色）
        │ M = T(0,0.5,0)       │
        │   * [S(flashScale)]  │
        │   * R_y(-90°)        │
        │   * T(-0.5,-0.5,0.5) │
        │   * R_y(90°)         │
        └──────────────────────┘
                                     │
                                     ▼
                   drawMesh(modelMatrix, position, overlayColor)
```

### 闪烁缩放公式（对齐 MC 1.21.11 TntRenderer）

```
if (fuseRemaining >= 0 && fuseRemaining < 10) {
    f = 1 - fuseRemaining / 10;
    f = clamp(f, 0, 1);
    f *= f; f *= f;   // 4 次方
    scale = 1 + f * 0.3;
} else {
    scale = 1;
}
```

边界值：
- `fuse < 0` 或 `fuse >= 10` → `scale = 1.0`
- `fuse = 0` → `scale = 1.3`（最大）
- `fuse = 5` → `scale = 1.01875`
- `fuse = 1` → `scale = 1.19683`

### 白色闪烁帧判定（对齐 MC TntMinecartRenderer `OverlayTexture.pack(10)`）

```
if (fuseRemaining > -1) {
    flashFrame = ((int)fuseRemaining / 5) % 2 == 0;
} else {
    flashFrame = false;
}
```

闪烁周期（每 5 tick 交替）：
- `fuse = 0-4` → 闪烁（白色半透明覆盖，alpha=0.5）
- `fuse = 5-9` → 不闪烁
- `fuse = 10-14` → 闪烁
- `fuse = 15-19` → 不闪烁

### 白色闪烁实现方式

MC `OverlayTexture.pack(10)` 通过 overlay 通道传递白色半透明覆盖。本项目实体管线的 `overlayColor` 通道对应此机制：着色器在 `overlayColor.a > 0` 时执行 `mix(color, overlayColor.rgb, overlayColor.a)`。

**关键**：`hurtTime` 通道在 `shaders/entity.frag` 中产生**红色**闪烁（`vec3(1.0, 0.0, 0.0)`），不适用于白色闪烁。TNT 白色闪烁必须使用 `overlayColor = Vector4f(1.0f, 1.0f, 1.0f, 0.5f)`，不能复用 `hurtTime`。

### 变换链矩阵乘法顺序（右乘，对应 MC PoseStack）

MC `PoseStack` 使用右乘语义：`current = current * newTransform`，因此最终矩阵 `M = T1 * [S] * R1 * T2 * R2`，顶点 `v` 变换为 `T1 * [S] * R1 * T2 * R2 * v`，最右侧的 `R2` 最先作用于顶点。

`buildTntModelMatrix()` 按以下顺序右乘构造矩阵：
1. `T1 = translate(0, 0.5, 0)`（抬高半个方块）
2. `S = scale(flashScale)`（仅 `fuse < 10` 时）
3. `R1 = rotateY(-90°)`
4. `T2 = translate(-0.5, -0.5, 0.5)`
5. `R2 = rotateY(90°)`

矩阵布局：行主序 `std::array<f64, 16>`，索引 `[row*4+col]`，平移分量位于 `m[3]`、`m[7]`、`m[11]`。

## 容易踩的坑

### 1. TNT 白色闪烁不能用 hurtTime

`shaders/entity.frag` 中 `hurtTime > 0` 产生红色闪烁（`vec3 hurtColor = vec3(1.0, 0.0, 0.0); color = mix(color, hurtColor, hurtIntensity * 0.5)`），而 MC `OverlayTexture.pack(10)` 是白色半透明覆盖。TNT 白色闪烁必须通过 `overlayColor = Vector4f(1, 1, 1, 0.5)` 实现，**不能**复用 `hurtTime = 1.0f`。

### 2. 纹理图集切换必须配对

`FallingBlockRenderer` / `TNTRenderer` 渲染前切换到 `ChunkTextureAtlas`，渲染后必须恢复 `EntityTextureAtlas`。如果忘记恢复，后续实体渲染会使用错误的纹理图集导致贴图错乱。

### 3. 方块网格按 BlockState* 缓存

`_getOrCreateBlockMesh()` 按 `BlockState*` 指针缓存 `EntityMesh`。项目中方块状态指针来自 `BlockRegistry`，是稳定的，指针作为键安全有效。切勿对临时 `BlockState` 对象取地址作为缓存键。

### 4. 引信插值公式

`fuseRemaining = entity.tntFuse() - partialTicks + 1.0f`。`+1` 对齐 MC `TntRenderer.extractRenderState` 的偏移，`- partialTicks` 实现帧间插值。漏掉 `+1` 会导致闪烁提前一 tick 结束。

### 5. PoseStack 右乘语义

MC `PoseStack` 使用右乘：`current = current * newTransform`。实现 `buildTntModelMatrix` 时必须按 `T1 * S * R1 * T2 * R2` 顺序右乘，不能按左乘顺序。若顺序错误，顶点变换结果会完全不同（例如 `translate(0, 0.5, 0)` 的 Y 平移会被内层 scale 放大）。

### 6. BlockMeshBuilder 回退路径

`BlockMeshBuilder::buildBlockMesh()` 依赖 `BlockModelCache::modelCache()`，资源包未加载时回退到单位立方体。测试环境或资源包加载失败时，TNT/FallingBlock 会渲染为单位立方体而非真实方块模型，这是预期行为。

### 7. isFullbright() 覆盖

`ShulkerBulletRenderer`、`WitherSkullRenderer`、`DragonFireballRenderer` 重写 `isFullbright()` 返回 true，对齐 MC Java 中 `getBlockLightLevel() = 15`。这些实体在黑暗中也会以全亮光照渲染。

### 8. 单元测试入口

`TNTRenderer::calculateTntFlashScale`、`isTntFlashFrame`、`buildTntModelMatrix` 与 `FallingBlockRenderer::buildFallingBlockModelMatrix` 均为 `static noexcept` 方法，专门提取出来供 `tests/client/renderer/trident/entity/renderer/special/SpecialRendererMatrixTest.cpp` 直接调用，无需模拟 Vulkan 管线。修改这些方法的签名或行为时必须同步更新测试。
