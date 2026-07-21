# 渲染管线

本目录包含实体渲染的 Vulkan 管线和纹理图集管理。

## 目录结构

```
pipeline/
├── EntityPipeline.hpp/cpp       # Vulkan 实体渲染管线（管理GPU资源、网格创建/更新/渲染）
└── EntityTextureAtlas.hpp/cpp   # 实体纹理图集（合并多个实体纹理到一张大纹理，支持运行时重建）
```

## 内部模块关系

```
EntityPipeline
    ↑ 使用纹理图集
    |
EntityTextureAtlas ──→ 提供纹理区域(TextureRegion)用于UV映射
```

`EntityPipeline` 依赖 `EntityTextureAtlas` 提供的纹理图集进行渲染。纹理图集将多个实体纹理合并到一张大纹理中，减少纹理绑定次数。

## 上下游外部依赖关系

### 上游依赖（本目录依赖的模块）

| 模块 | 依赖内容 |
|------|----------|
| `client/renderer/trident/util/VulkanUtils.hpp` | Vulkan 辅助函数 |
| `client/renderer/trident/entity/model/core/ModelRenderer.hpp` | ModelVertex 顶点类型 |
| `client/renderer/MeshTypes.hpp` | TextureRegion 类型 |
| `client/renderer/util/ShaderPath.hpp` | 着色器路径解析 |
| `common/core/Result.hpp` | 错误处理 |
| `common/resource/pack/IResourcePack.hpp` | 资源包接口 |
| `common/resource/ResourceLocation.hpp` | 资源位置标识 |
| `common/util/math/Vector3.hpp, Vector4.hpp` | 数学向量 |

### 下游依赖（依赖本目录的模块）

| 模块 | 使用方式 |
|------|----------|
| `client/renderer/trident/core/TridentEngine.hpp` | 创建和管理 EntityPipeline、EntityTextureAtlas 实例 |
| `client/renderer/trident/entity/core/EntityRenderer.hpp` | 使用 EntityPipeline 进行实体渲染 |
| `client/renderer/trident/entity/core/EntityRendererManager.hpp` | 管理 EntityPipeline 实例 |
| `client/renderer/trident/entity/core/AnimatedMeshCache.hpp` | 使用 EntityPipeline 缓存网格 |
| `client/renderer/trident/entity/core/LivingRenderer.hpp` | 生物渲染使用 EntityPipeline |
| `client/renderer/trident/entity/layer/*` | 层渲染器使用 EntityPipeline |
| `client/renderer/trident/entity/effect/*` | 特效渲染使用 EntityPipeline |
| `client/renderer/trident/blockentity/BlockEntityRenderer.cpp` | 方块实体渲染使用 EntityPipeline |
| `client/application/features/ClientApplicationBootstrap.cpp` | 初始化时创建图集和管线 |

## 命名空间

```cpp
namespace mc::client::renderer::entity::pipeline {
    class EntityPipeline;
    class EntityTextureAtlas;
    struct EntityMesh;
    struct EntityAtlasBuildResult;
}
```

## 容易踩的坑

### 1. mega-buffer 子分配

`EntityPipeline` 不再为每个 mesh 独占 `VkBuffer`+`VkDeviceMemory`，而是在统一的 vertex（32MB/段）/index（8MB/段）mega-buffer 段内用 OffsetAllocator 子分配一段连续区间。`EntityMesh` 只保存所属段的 `VkBuffer` + 段内 `vertexOffset`/`indexOffset` + `Allocation`/`segmentIndex`/`alignedSize`（供延迟回收）。`createMesh`/`updateMesh` 每次都重新子分配新区间，旧区间入 `m_pendingDestroys` 延迟归还。段 `VkBuffer`/`VkDeviceMemory` 仅在 `destroy()` 释放，容量不足追加新段（OffsetAllocator 不可 resize，多段规避数据迁移）。

### 2. 统一暂存上传

`createMesh`/`updateMesh` 经 `m_context->stagingPool()` 同步上传（`stage`→`memcpy`→`copyToBuffer`→`release`），不再自建 staging buffer。`initialize` 首参为 `TridentContext*`，由 `TridentEngine` 注入 `context()`。池未就绪时报 `staging pool not available` 错误，无 fallback。旧的 `_ensureReusableStagingBuffer`/`_uploadToDeviceBuffer`/`_copyBuffer`/`_beginSingleTimeCommands`/`_endSingleTimeCommands` 及 `m_vertexStagingBuffer`/`m_indexStagingBuffer` 已全部删除。

### 3. 延迟归还与守恒断言

mesh 替换/销毁时旧子分配区间不立即 free，而是入队等过 `maxFramesInFlight+1` 帧后由 `processPendingDestroys` 归还（保证仍被在飞命令缓冲区引用的区间不被提前复用，device-lost 根因）。`_freeAllocation` 内有守恒断言 `storageReport().totalFreeSpace == localFreeBytes`。`destroy()` 先回收延迟队列再销毁段；存在所有者未 `destroyMesh` 的 live mesh（如 `SpecialEntityRenderers` 的 `blockMeshCache`）时，关闭期记泄漏告警但不致命（设备已 idle、整段 VkBuffer 即将销毁）。

### 4. 纹理图集路径格式

`EntityTextureAtlas` 支持多种纹理路径格式：
- `minecraft:textures/entity/pig/pig.png` - 完整路径
- `minecraft:entity/pig/pig` - 省略 textures 前缀和扩展名
- `minecraft:pig` - 简化格式

内部会尝试多种路径回退，但建议使用完整路径以避免歧义。

### 5. 纹理图集运行时重建

`EntityTextureAtlas` 支持运行时添加新纹理并重建图集（`rebuild()`），但这是昂贵操作。应批量添加纹理后一次性重建，避免频繁调用 `rebuild()`。

### 6. 描述符集绑定顺序

渲染时必须先调用 `bind(cmd)` 绑定管线，再调用 `bindTextureDescriptor(cmd)` 绑定纹理描述符集。顺序错误会导致渲染异常或验证层错误。

### 7. BlendMode 选择

`EntityPipeline::bind()` 支持多种混合模式：
- `BlendMode::None` - 无混合（blendEnable=VK_FALSE），用于不透明/剪切实体渲染，对应 MC Java 的 withoutBlend()
- `BlendMode::Alpha` - 默认，用于大多数实体
- `BlendMode::Additive` - 用于眼睛发光、能量光效等（src*srcAlpha + dst*1）
- `BlendMode::Multiply` - 用于颜色调制/着色叠加（out = 2*src*dst），对应 MC 1.21.11 RenderPipelines.CRUMBLING
- `BlendMode::Lines` - 使用 `LINE_LIST` 拓扑，用于钓鱼线等线段渲染

选择错误的混合模式会导致渲染效果不正确。所有管线共享相同的着色器、管线布局、顶点输入和光栅化状态，仅颜色混合状态（和 Lines 的输入装配）不同。管线创建失败时仅记录警告并回退到 Alpha 混合，不影响其他管线初始化。

### 8. EntityMesh 生命周期

`EntityPipeline` 不管理 `EntityMesh` 的生命周期，调用者需要：
- 使用 `createMesh()` 创建网格后持有返回的 `EntityMesh`
- 在不需要时调用 `destroyMesh()` 释放 GPU 资源（将子分配区间入延迟归还队列）
- 不要在管线销毁后访问 `EntityMesh`

### 9. Vulkan 资源销毁顺序

`EntityPipeline` 和 `EntityTextureAtlas` 都持有 Vulkan 资源，必须在设备销毁前调用 `destroy()` 方法。典型的销毁顺序：
```cpp
pipeline.destroy();      // 先销毁管线（mega-buffer 段随之销毁）
atlas.destroy();         // 再销毁图集
// 最后销毁 Vulkan 设备
```
