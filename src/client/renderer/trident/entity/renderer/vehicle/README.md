#载具渲染器(Vehicle Renderers)

本目录包含船和矿车等可乘坐实体的渲染器实现。渲染器同时实现 `core::EntityRenderer` 与 `core::PipelineMeshProvider`，通过
            GPU 管线路径（`EntityRendererManager::renderWithPipeline`）渲染。

        ##目录结构

``` vehicle /
├── VehicleRenderers.hpp #船和矿车的模型、渲染器、矩阵工具定义
└── VehicleRenderers.cpp #船和矿车的模型与渲染器实现
```

        ##内部模块关系

```
┌───────────────────────────────────────────────────────────────────────┐
│ VehicleRenderers                                │
├───────────────────────────────────────────────────────────────────────┤
│ namespace matrix{...} #行主序 4x4 矩阵工具（identity /
        translation /  │
│ #rotation / scale / multiply / rotationAxis）       │
│                                                                        │
│ BoatModel #5 个船体面 +
    2 个桨 + noWater              │
│    └── generateMesh(vertices, indices, parentMatrix, scale)            │
│                                                                        │
│ BoatRenderer : EntityRenderer,
    PipelineMeshProvider                   │
│    ├── generateMesh() #输出像素空间几何体（scale = 1.0）         │
│    ├── computeCustomModelMatrix() #注入船的完整变换链                    │
│    └── _buildBoatModelMatrix() #读取同步状态构建矩阵                    │
│                                                                        │
│ MinecartModel #6 个面（底 / 左 / 右 / 前 / 后 / 内部底）             │
│    └── generateMesh(vertices, indices, parentMatrix, scale)            │
│                                                                        │
│ MinecartRenderer : EntityRenderer,
                         PipelineMeshProvider               │
│    ├── generateMesh() #输出像素空间几何体                     │
│    ├── computeCustomModelMatrix() #注入矿车变换链 +
        TNT 闪烁             │
│    ├── _buildMinecartModelMatrix() #读取同步状态构建矩阵                 │
│    ├── calculateTntFlashScale() #TNT 闪烁缩放因子（公开供测试）          │
│    └── isTntFlashFrame() #TNT 是否处于白色闪烁帧（公开供测试）     │
└───────────────────────────────────────────────────────────────────────┘
```

        ##上下游外部依赖关系

        ## #上游依赖（本模块依赖）

    | 模块 | 用途 | | -- -- --| -- -- --|
    | `core / EntityRenderer.hpp` | 实体渲染器基类（含 `computeCustomModelMatrix` 虚方法） |
    | `model / core / ModelRenderer.hpp` | 模型部件渲染器（`generateMesh` 接口） |
    | `client / world / entity / ClientEntity.hpp` |
    客户端实体（读取 `dataManager()`、`fuseTimer()`、`getInterpolatedYaw / Pitch`） |
    | `common / entity / entities / vehicle / BoatEntity.hpp` | 船实体类（`DataParameter` 静态访问器） |
    | `common / entity / entities / vehicle / MinecartEntity.hpp` |
    矿车实体基类与 TNT 子类（`DataParameter` 静态访问器） |
    | `common / util / math / MathConstants.hpp` | `PI`、`PI_DOUBLE` |
    | `common / util / math / MathUtils.hpp` | `clampedLerp`、`clamp`、`DEG_TO_RAD` |
    | `common / resource / ResourceLocation.hpp` | 资源路径 |
    | `common / entity / core / EntityRegistry.hpp` | `EntityTypes::TNT_MINECART` 类型字符串 |

    ## #下游依赖（依赖本模块）

    | 模块 | 用途 | | -- -- --| -- -- --| | `renderer / RendererRegistration.cpp` | 通过工厂注册船和矿车渲染器 |
    | `core / EntityRendererManager.cpp` | 通过 `getPipelineMeshProvider()` / `computeCustomModelMatrix()` 渲染 |

    ##渲染流程

``` EntityRendererManager.renderWithPipeline(entity)
    │
    ├── getPipelineMeshProvider() ───► BoatRenderer / MinecartRenderer
    │   └── generateMesh(entity, vertices, indices)
    │         ├── _setupPaddleAnimation() （仅船：根据桨状态设置桨角度）
    │         └── m_model.generateMesh(vertices, indices, identity, 1.0)
    │
    ├── computeCustomModelMatrix(entity, partialTicks, outMatrix, outHurt, outDeath)
    │   ├── 船：_buildBoatModelMatrix()
    │   │ translate(0, 0.375, 0) * rotateY(180 - yaw)
    │   │ * [hurt shake rotateX] * [bubble tilt] * scale(-1, -1, 1) * rotateY(90)
    │   └── 矿车：_buildMinecartModelMatrix()
    │ translate(0, 0.375, 0) * rotateY(180 - yaw) * rotateZ(-pitch)
    │ * [hurt shake rotateX] * scale(-1, -1, 1) *
        [TNT flash scale]
    │
    └── drawMesh(cmd, mesh, modelMatrix, pos, MODEL_SCALE, overlayColor, hurtTime, ...)
```

        ##容易踩的坑

        ## #1. 船的木材类型纹理

        船有
        10 种木材类型（橡木、云杉、白桦、丛林、金合欢、深色橡木、红树木、樱花、苍白橡木、竹），每种对应不同的纹理。`BoatRenderer::
            getTexture()` 通过 `BoatType` 枚举索引静态纹理数组，确保枚举值与数组索引一致。

        ## #2. ModelRenderer 的纹理尺寸

`BoatModel` 使用 128×64 纹理，`MinecartModel` 使用
        64×32 纹理。调用 `setTextureSize()` 设置正确的纹理尺寸，否则 UV 坐标会计算错误。

        ## #3. 模型旋转角度单位

`ModelRenderer::setRotateAngleX
        / Y / Z()` 接受弧度值。代码中使用 `PI_DOUBLE`（即 2π）来计算旋转角度，例如 `PI_DOUBLE /
        2.0` 表示 90°，`PI_DOUBLE *
        1.5` 表示 270°。

        ## #4. 矿车内部底板偏移

`MinecartModel::setInsideOffset()` 用于调整内部底板的 Y 偏移，当乘客乘坐时需要调整此值。矿车的
        6 个面存储在 `m_sides[5]` 数组中，第 6 个元素（索引 5）是内部底板。

        ## #5. 自定义模型矩阵的注入

`BoatRenderer` 和 `MinecartRenderer` 重写 `computeCustomModelMatrix()` 返回
        true，使 `EntityRendererManager` 跳过默认的 `rotateY(yaw) *
        scale(-1, -1, 1) *
        translate(0, 1.501, 0)` 模型矩阵，改用渲染器提供的完全自定义矩阵。这是载具渲染按
        MC `AbstractBoatRenderer` / `AbstractMinecartRenderer` 变换链正确呈现的关键。

        ## #6. 像素空间几何体

`generateMesh` 使用 `scale = 1.0`（像素单位）输出几何体，由 `drawMesh` 的 `MODEL_SCALE =
                             1 / 16` 统一缩放到世界单位。切勿在 `generateMesh` 中传入 `1 /
            16`，否则会产生双重缩放。

            ## #7. 矩阵布局约定

            项目中所有 4x4 矩阵均为行主序 `std::array<f64, 16>`，索引 `[row * 4 +
                col]`，平移列位于 `m[3]`、`m[7]`、`m[11]`。本目录 `namespace matrix` 提供构造与组合原语，
            * *不要 *
            *与 `ModelRenderer` 内部的 `_identityMatrix` 等私有工具混用——行为一致但作用域不同。

            ## #8. TNT 矿车闪烁的两条通道

        - **缩放通道 * *：`_buildMinecartModelMatrix` 内调用 `calculateTntFlashScale(fuse)` 计算 `1 + (1 - fuse / 10) ^
    4 * 0.3`，通过模型矩阵的 `scale` 应用。 - ** 白色闪烁通道**：当前通过 `outHurtTime =
                                 1.0f` 复用着色器红色闪烁因子近似（因 `drawMesh` 暂不支持自定义 `overlayColor`）。TODO
    : 后续扩展 `EntityRenderer` 接口允许渲染器自定义 `overlayColor`，以完全对齐
      MC `TntMinecartRenderer` 的 `OverlayTexture
          .pack(10)` 白色叠加。

      ## #9. 桨动画的近似实现

      MC Java 的 `AbstractBoat.getRowingTime(side, partialTicks)` 依赖 `paddlePositions[side]`（每 tick
      推进的相位），当前 `paddlePositions` 未通过 `DataParameter` 同步到客户端。本渲染器使用 `(
          ticksExisted + partialTicks) *
    0.79` 作为相位近似，视觉上呈现划桨循环，但与原版存在偏差。TODO
    : 后续同步 `paddlePositions` 数组到 `ClientEntity` 后切换为真实插值。

      ## #10. 气泡柱倾斜未同步

      MC Java 的 `AbstractBoat.getBubbleAngle(
          partialTicks)` 依赖 `m_prevRockingAngle` / `m_rockingAngle`，当前这两个字段未通过 `DataParameter` 同步到客户端，渲染器使用
    0 占位。TODO : 后续同步 `rockingAngle` 后启用气泡柱倾斜效果。
