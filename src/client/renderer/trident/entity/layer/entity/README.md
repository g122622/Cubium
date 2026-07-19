# 实体特性层渲染器

本目录包含实体特性相关的层渲染器。

## 目录结构

```
entity/
├── ArrowLayer.hpp/cpp         # 箭矢附着层（渲染生物身上插着的箭矢）
├── HeldBlockLayer.hpp/cpp     # 方块持有层（渲染末影人手持的方块）
├── SaddleLayer.hpp/cpp        # 鞍层（渲染可骑乘实体的鞍）
├── SheepWoolLayer.hpp/cpp     # 羊毛层（渲染羊的羊毛，支持染色和彩虹羊）
├── VillagerLayer.hpp          # 村民多层纹理层（根据职业和生物群系叠加纹理）
└── WolfCollarLayer.hpp/cpp    # 狼项圈层（渲染驯服狼的项圈）
```

## 内部模块关系

所有层渲染器均继承自 `layer::core::LayerRenderer<TEntity>` 基类，实现统一的渲染接口：
- `renderPipeline()` - GPU 管线路径渲染（主要方法）
- `render()` - CPU 路径渲染（已废弃）
- `shouldRender()` - 条件渲染检查

| 渲染器 | 关联实体 | 关联模型 |
|--------|----------|----------|
| SaddleLayer | 可骑乘实体（马、猪等） | 通用模板 |
| SheepWoolLayer | SheepEntity | BipedModel |
| WolfCollarLayer | ClientEntity（狼） | 自建环形网格（不依赖 WolfModel） |
| ArrowLayer | LivingEntity | 通用模板 |
| HeldBlockLayer | ClientEntity（末影人） | 自建方块网格（基于 BlockModelCache） |
| VillagerLayer | VillagerEntity / ZombieVillagerEntity | VillagerModel |

## 上下游外部依赖关系

**依赖了谁（上游）：**
- `layer::core::LayerRenderer` - 层渲染器基类
- `core::IEntityRenderer` - 实体渲染器接口（获取父模型）
- `core::AnimationContext` - 动画上下文
- `pipeline::EntityPipeline` - 实体渲染管线（GPU 网格创建和绘制）
- `pipeline::EntityTextureAtlas` - 实体纹理图集（VillagerLayer UV 重映射）
- `model::*Model` - 各类实体模型
- 实体类：`LivingEntity`, `SheepEntity`, `WolfEntity`, `EndermanEntity`, `VillagerEntity` 等

**被谁依赖（下游）：**
- `LivingRenderer` 及其子类 - 在构造函数中添加各类层渲染器
- 具体实体渲染器如 `SheepRenderer`, `WolfRenderer`, `VillagerRenderer`, `EndermanRenderer` 等

## 容易踩的坑

1. **CPU 路径已废弃**：`render()` 方法保留用于向后兼容，新代码应实现 `renderPipeline()` 方法。基类默认实现为空，若只实现 `render()` 而外部调用 `renderPipeline()` 会导致不渲染。

2. **模板类显式实例化**：`SaddleLayer`, `SheepWoolLayer`, `ArrowLayer` 均为模板类，使用时需要显式实例化（如 `template class SaddleLayer<::mc::LivingEntity>;`）。`HeldBlockLayer` 和 `WolfCollarLayer` 已迁移为 `LayerRenderer<ClientEntity>` 非模板类，无需显式实例化。

3. **HeldBlockLayer 使用 ClientEntity 而非 EndermanEntity**：`HeldBlockLayer` 模板参数为 `ClientEntity`，通过 `entity.endermanHeldBlockState()` 读取元数据镜像字段（由 `ClientEntity::syncMetadataFromDataManager` 从 `EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM` 同步）。这是因为 `EndermanRenderer::renderLayersPipelineClient` 直接接收 `ClientEntity&`，层无需访问服务端 `EndermanEntity`。

4. **HeldBlockLayer 纹理图集切换**：方块纹理 UV 基于 AtlasManager 的 blocks atlas，而非实体纹理图集。渲染前通过 `setBlockAtlas`（注入 blocks atlas 的 imageView/sampler 句柄）与 `setEntityTextureAtlas`（注入实体图集）准备图集。渲染时切换到方块图集，渲染后恢复为实体图集，避免污染后续实体渲染。两个图集句柄由 `EndermanRenderer::setTextureAtlas`/`setBlockAtlas` 注入，最终来源为 `EntityRendererManager::setTextureAtlas`/`setBlockAtlas`（blocks atlas 句柄来自 `TridentEngine` 持有的 AtlasManager）。

5. **HeldBlockLayer 网格缓存**：按 `BlockState*` 指针缓存方块网格（`std::unordered_map<const BlockState*, std::unique_ptr<EntityMesh>>`），方块状态指针来自 `BlockRegistry` 是稳定的。缓存随 `HeldBlockLayer` 实例生命周期销毁（即 `EndermanRenderer` 销毁时自动清理）。

6. **VillagerLayer 静态网格缓存**：使用静态 `std::unordered_map` 缓存网格，配合 `std::shared_mutex` 实现线程安全。务必在渲染前调用 `setTextureAtlas()` 设置纹理图集，否则 UV 重映射不会生效。

7. **网格缓存生命周期**：各渲染器使用 `static std::unique_ptr<pipeline::EntityMesh>` 缓存网格（如 WolfCollarLayer），在整个程序生命周期内有效。HeldBlockLayer 使用实例级缓存（随 EndermanRenderer 销毁）。

8. **彩虹羊检测**：`SheepWoolLayer::isRainbowSheep()` 检查实体自定义名称是否为 "jeb_" 或 "jeb"，颜色每 2 tick 变化一次。

9. **项圈颜色边界**：`WolfCollarLayer` 需检查颜色索引是否 < 16，超出范围时回退到默认红色（索引 14）。

10. **WolfCollarLayer 使用 ClientEntity 而非 WolfEntity**：与其他层类似，`WolfCollarLayer` 模板参数为 `ClientEntity`，通过 `entity.wolfTamed()`/`entity.wolfCollarColor()` 读取元数据镜像字段（由 `ClientEntity::syncMetadataFromDataManager` 从 `TameableEntity::DATA_TAMED_PARAM`/`WolfEntity::DATA_COLLAR_COLOR_PARAM` 同步）。这是因为 `WolfRenderer::renderLayersPipelineClient` 直接接收 `ClientEntity&`，层无需访问服务端 `WolfEntity`。

## 参考

- MC 1.21.11 LayerRenderer
- MC 1.21.11 SaddleLayer / SheepWoolLayer / WolfCollarLayer
- MC 1.21.11 StuckInBodyLayer（箭矢附着）
- MC 1.21.11 CarriedBlockLayer（末影人手持方块，1.16.5 为 HeldBlockLayer）
- MC 1.21.11 VillagerLevelPendantLayer（村民多层纹理）
