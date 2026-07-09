# 方块粒子 (Block Particles)

方块粒子用于方块相关的视觉效果，使用 TERRAIN_SHEET 渲染类型从方块纹理图集获取纹理。

## 目录结构

```
block/
├── DiggingParticle.hpp     # 挖掘粒子 + 方块标记粒子 + 方块碎裂粒子
├── DiggingParticle.cpp     # 挖掘粒子、方块标记粒子、方块碎裂粒子实现
├── ComposterParticle.hpp   # 堆肥桶粒子（棕色灰尘，重力下落+旋转漂移）
├── ComposterParticle.cpp   # 堆肥桶粒子实现
├── LeavesParticle.hpp      # 树叶粒子（CherryLeaves/PaleOakLeaves/TintedLeaves）
├── LeavesParticle.cpp      # 树叶粒子实现
├── WaxParticle.hpp         # 涂蜡粒子（WaxOn 蜂蜜黄）+ 除蜡粒子（WaxOff 铜锈绿）
├── WaxParticle.cpp         # 涂蜡/除蜡粒子实现
├── ScrapeParticle.hpp      # 铜氧化刮削粒子（铜棕色灰尘）
├── ScrapeParticle.cpp      # 刮削粒子实现
├── DustPillarParticle.hpp  # 尘柱粒子（重锤砸地攻击产生，使用方块纹理）
├── DustPillarParticle.cpp  # 尘柱粒子实现
└── ItemParticle.hpp/cpp    # 物品粒子（Item/ItemSlime/ItemCobweb/ItemSnowball 共用）
```

## 内部模块关系

- `DustPillarParticle` 继承自 `DiggingParticle`，复用方块纹理渲染逻辑（`_initializeBlockTexture()`、`buildVertices()` 等）。
- `DustPillarParticle` 重写重力（1.0 vs DiggingParticle 的 0.03）和生命周期（20-40 tick vs 16-24 tick），以匹配 MC Java 的 DustPillarProvider 行为。
- `DustPillarParticle` 在构造函数中重写速度：X/Z 替换为 `nextGaussian() / 30.0`（极低水平扩散），Y 保留传入值并叠加 `nextGaussian() / 2.0`（先扬后抑的抛物线），匹配 MC Java 的 `DustPillarProvider.setParticleSpeed()` 行为。
- `BlockMarkerParticle` 和 `BlockCrumbleParticle` 与 `DiggingParticle` 共存于同一文件，共享方块纹理初始化逻辑和 `buildVertices()` 渲染逻辑。
- `BlockMarkerParticle` 不移动、不受重力，用于结构方块静态标记显示。
- `BlockCrumbleParticle` 比 `DiggingParticle` 更小（0.05 vs 0.1）且生命周期更短（15 vs 20 tick）。
- `ComposterParticle`、`WaxOnParticle`、`WaxOffParticle`、`ScrapeParticle` 均继承自 `Particle`，使用 `PARTICLE_SHEET_OPAQUE` 渲染类型和 `falling_dust`/`wax_on`/`wax_off`/`scrape` 纹理，行为类似 `FallingDustParticle`（重力下落 + 旋转 + 水平漂移 + 淡出 + 淡入缩放）。
- `ItemParticle` 用于物品破碎效果（对应 MC Java 1.21.11 的 `ItemParticleProvider` / `TerParticle`）。已集成 `ItemModelCache` + `ItemTextureAtlas` 实现真实物品纹理渲染，采用双路径纹理解析策略：
  - **方块物品**（stone、dirt 等）：通过 `BlockItemRegistry` 解析为 `BlockState`，复用 `BlockModelCache` 获取方块粒子纹理（与 `DiggingParticle` 路径一致），优先使用 `BlockAppearance::particleTexture`，回退到随机面纹理。
  - **非方块物品**（工具、食物等）：通过 `ItemModelCache::getItemModel()` 获取 `BakedItemModel`，取 `textureLayers[0]`（layer0），再用注入的 `ItemTextureAtlas::getItemTexture()` 解析纹理区域 UV。
  - `ItemTextureAtlas` 通过静态方法 `ItemParticle::setItemTextureAtlas()` 注入（由 `TridentEngine` 在初始化/销毁时管理，参考 `ItemMeshBuilder` 的同类模式），`nullptr` 表示清除引用。
  - Item/ItemSlime/ItemCobweb/ItemSnowball 四种 ParticleTypeId 共享同一个 `ItemParticle` 类，仅类型 ID 不同；数据工厂从 `ItemParticleData` 提取 `ItemStack` 并调用 `createWithItemStack()`。
  - **架构限制（TODO）**：`ItemParticle` 使用 `TERRAIN_SHEET` 渲染类型，但 `ParticleManager` 当前仅绑定单一的 `ParticleTextureAtlas` 纹理（默认生成的占位纹理），不支持按渲染类型切换纹理图集。这意味着非方块物品的 `ItemTextureAtlas` UV 坐标在渲染时采样的是错误纹理。该限制同样影响 `DiggingParticle` 等所有 `TERRAIN_SHEET` 粒子（方块图集 UV 也采样错误纹理）。完整修复需要 `ParticleManager` 支持按 `ParticleRenderType` 绑定不同的纹理图集描述符。详见 `ItemParticle.cpp` 中 `_initializeFromPlainItem` 的 TODO 注释。
- `LeavesParticle.hpp` 包含三个独立的树叶粒子类：
  - `CherryLeavesParticle` - 樱花树叶粒子（粉色花瓣缓慢飘落，正弦摆动，旋转动画，TRANSLUCENT 渲染，使用 cherry 纹理）
  - `PaleOakLeavesParticle` - 苍白橡树叶粒子（灰绿色叶片缓慢飘落，较弱正弦摆动，TRANSLUCENT 渲染，使用 pale_oak 纹理）
  - `TintedLeavesParticle` - 着色树叶粒子（接收生物群系颜色，正弦摆动，旋转动画，OPAQUE 渲染，使用 leaf 纹理。TODO: 待生物群系颜色数据管线支持）

## 上下游外部依赖关系

### 依赖的上游模块

- `mc::client::renderer::trident::particle::Particle` - 粒子基类
- `mc::client::renderer::trident::chunk::ChunkMesher` - 获取全局 `BlockModelCache`
- `mc::client::resource::BlockModelCache` - 获取方块纹理和粒子纹理（方块物品路径）
- `mc::client::resource::ItemModelCache` - 获取物品 BakedModel（非方块物品路径，仅 `ItemParticle` 使用）
- `mc::client::resource::ItemTextureAtlas` - 物品纹理图集 UV 解析（非方块物品路径，仅 `ItemParticle` 使用，通过静态注入）
- `mc::client::resource::ResourceManager` - 通过 `BlockAppearance::particleTexture` 提供模型定义的粒子纹理
- `mc::common::item::items::block::BlockItemRegistry` - 方块物品判断与 Block 解析（仅 `ItemParticle` 使用）
- `mc::world::block::BlockState` - 方块状态
- `mc::math::Random` - 随机数生成

### 被下游模块依赖

- `ParticleRegistry` - 通过 `create()` / `createWithBlock()` / `createWithItemStack()` 工厂方法创建粒子实例
- `ParticleFactories` - 注册 `Item`/`ItemSlime`/`ItemCobweb`/`ItemSnowball` 的数据工厂，从 `ItemParticleData` 提取 `ItemStack`
- `ParticleManager` - 管理粒子生命周期和渲染
- 方块破坏逻辑 - 通过 `DiggingParticle::createWithBlock()` 创建粒子
- 物品破碎逻辑 - 通过 `ItemParticle::createWithItemStack()` 创建粒子（数据管线：`ParticlePacket::createItem` → `NetworkClient::onItemParticle` → `ClientApplicationNetwork` → `ItemParticleData` → 数据工厂）

## 容易踩的坑

1. **纹理获取失败**：`BlockModelCache` 不可用或方块状态没有对应外观时，会使用默认全纹理 UV (0,0)-(1,1)，可能导致粒子显示不正确。应确保在创建粒子前 `ChunkMesher::modelCache()` 已初始化。

2. **粒子纹理选择**：`_initializeBlockTexture()` 优先使用 `BlockAppearance::particleTexture`（模型 JSON 中 `textures.particle` 指定的纹理），若模型未定义粒子纹理则回退到从 `faceTextures` 中随机选取一个面的纹理。这与 MC 行为一致。

3. **UV 偏移计算**：挖掘粒子从 16x16 纹理中随机选取 4x4 区域（模拟 MC 1.16.5 的 `field_217587_G`/`field_217588_H`），UV 偏移范围为 0-3。

4. **默认工厂方法**：`create()` 默认使用石头方块状态，但依赖 `VanillaBlocks::STONE` 已初始化。推荐使用 `createWithBlock()` 并传入正确的 `BlockState`。

5. **渲染类型**：必须使用 `TERRAIN_SHEET` 渲染类型，因为方块粒子使用方块纹理图集而非粒子纹理图集。

6. **生命周期淡出**：生命周期超过 70% 后开始淡出，alpha 值线性递减。

7. **ItemParticle 双路径纹理解析**：`ItemParticle::_initializeItemTexture()` 先通过 `BlockItemRegistry::isBlockItem()` 判断是否为方块物品。方块物品走 `_initializeFromBlockItem()`（BlockModelCache 路径，与 DiggingParticle 一致），非方块物品走 `_initializeFromPlainItem()`（ItemModelCache + ItemTextureAtlas 路径）。两条路径任一成功即返回，都失败则使用占位纹理 `minecraft:particle/generic`。`ItemTextureAtlas` 必须通过 `setItemTextureAtlas()` 注入后才能解析非方块物品纹理，否则会输出 warning 并回退到占位纹理。

8. **ItemParticle 数据管线**：物品粒子通过 `ParticleData` 管线创建。`ParticleFactories` 为 Item/ItemSlime/ItemCobweb/ItemSnowball 注册共享的 `itemDataFactory`，从 `ItemParticleData` 提取 `ItemStack` 调用 `createWithItemStack()`。网络层通过 `ParticlePacket::createItem()` 序列化 `ItemStack`，客户端 `NetworkClient::_handleParticle()` 检测 `isItemParticle()` 后调用 `onItemParticle` 回调，由 `ClientApplicationNetwork` 封装为 `ItemParticleData` 投递到 `ParticleManager::addPendingParticle()`。
