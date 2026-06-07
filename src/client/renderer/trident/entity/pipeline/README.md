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
| `common/resource/IResourcePack.hpp` | 资源包接口 |
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

### 1. 网格更新时的容量复用

`updateMesh()` 在容量足够时仅上传新数据，不再销毁重建 GPU 缓冲区。如果顶点/索引数量超过容量，会自动扩容。但频繁扩容会影响性能，建议在创建网格时预估合适的容量。

### 2. 可复用暂存缓冲区

管线内部使用长期存在的暂存缓冲区（`m_vertexStagingBuffer`, `m_indexStagingBuffer`）进行顶点/索引上传，避免每次更新都调用 `vkAllocateMemory`。但这意味着管线销毁时必须正确清理这些缓冲区。

### 3. 纹理图集路径格式

`EntityTextureAtlas` 支持多种纹理路径格式：
- `minecraft:textures/entity/pig/pig.png` - 完整路径
- `minecraft:entity/pig/pig` - 省略 textures 前缀和扩展名
- `minecraft:pig` - 简化格式

内部会尝试多种路径回退，但建议使用完整路径以避免歧义。

### 4. 纹理图集运行时重建

`EntityTextureAtlas` 支持运行时添加新纹理并重建图集（`rebuild()`），但这是昂贵操作。应批量添加纹理后一次性重建，避免频繁调用 `rebuild()`。

### 5. 描述符集绑定顺序

渲染时必须先调用 `bind(cmd)` 绑定管线，再调用 `bindTextureDescriptor(cmd)` 绑定纹理描述符集。顺序错误会导致渲染异常或验证层错误。

### 6. BlendMode 选择

`EntityPipeline::bind()` 支持多种混合模式：
- `BlendMode::Alpha` - 默认，用于大多数实体
- `BlendMode::Additive` - 用于眼睛发光、能量光效等
- `BlendMode::Lines` - 使用 `LINE_LIST` 拓扑，用于钓鱼线等线段渲染

选择错误的混合模式会导致渲染效果不正确。

### 7. EntityMesh 生命周期

`EntityPipeline` 不管理 `EntityMesh` 的生命周期，调用者需要：
- 使用 `createMesh()` 创建网格后持有返回的 `EntityMesh`
- 在不需要时调用 `destroyMesh()` 释放 GPU 资源
- 不要在管线销毁后访问 `EntityMesh`

### 8. Vulkan 资源销毁顺序

`EntityPipeline` 和 `EntityTextureAtlas` 都持有 Vulkan 资源，必须在设备销毁前调用 `destroy()` 方法。典型的销毁顺序：
```cpp
pipeline.destroy();      // 先销毁管线
atlas.destroy();         // 再销毁图集
// 最后销毁 Vulkan 设备
```
