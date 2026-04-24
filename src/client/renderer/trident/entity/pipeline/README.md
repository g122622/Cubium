# 渲染管线

本目录包含实体渲染的 Vulkan 管线和纹理图集管理。

## 文件列表

| 文件 | 描述 |
|------|------|
| `EntityPipeline.hpp/cpp` | Vulkan 实体渲染管线 |
| `EntityTextureAtlas.hpp/cpp` | 实体纹理图集管理 |

## EntityPipeline

Vulkan 实体渲染管线，管理实体渲染的 GPU 资源。

### 主要功能

- 管线状态管理
- 顶点/索引缓冲区管理
- 描述符集管理
- 纹理绑定

### 使用方法

```cpp
// 初始化管线
EntityPipeline pipeline;
pipeline.initialize(
    device, physicalDevice, graphicsQueue,
    renderPass, cameraDescriptorLayout, descriptorPool,
    commandPool, sampleCount
);

// 创建实体网格
auto mesh = pipeline.createMesh(vertices, indices);

// 渲染
pipeline.bind(cmd);
pipeline.bindTextureDescriptor(cmd);
pipeline.drawMesh(cmd, mesh, modelMatrix, position, scale);

// 清理
pipeline.destroyMesh(mesh);
```

### 推送常量布局

```glsl
// 顶点着色器推送常量
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 position;
    float scale;
};
```

## EntityTextureAtlas

实体纹理图集，将多个实体纹理合并到一张大纹理中。

### 主要功能

- 纹理图集构建
- UV 坐标重映射
- 支持多种纹理路径格式

### 使用方法

```cpp
// 初始化
EntityTextureAtlas atlas;
atlas.initialize(device, physicalDevice, commandPool, graphicsQueue);

// 添加纹理
atlas.addTexture(pack, ResourceLocation("minecraft:textures/entity/pig/pig.png"));

// 构建图集
auto result = atlas.build();

// 获取纹理区域
const TextureRegion* region = atlas.getRegion(location);

// 渲染时绑定
pipeline.setTextureAtlas(atlas.imageView(), atlas.sampler());
```

### 纹理路径格式

支持多种纹理路径格式：
- `minecraft:textures/entity/pig/pig.png`
- `minecraft:entity/pig/pig`
- `minecraft:pig`（简化格式）

## 数据结构

### EntityMesh

```cpp
struct EntityMesh {
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    u32 indexCount;
    u32 vertexCount;
    f64 posX, posY, posZ;
};
```

### TextureRegion

```cpp
struct TextureRegion {
    f64 u0, v0;  // 左上角 UV
    f64 u1, v1;  // 右下角 UV
};
```

## 性能优化

1. **顶点缓冲区复用**：使用设备本地内存
2. **纹理图集**：减少纹理绑定次数
3. **描述符集缓存**：避免频繁分配

## 命名空间

```cpp
namespace mc::client::renderer::entity::pipeline {
    class EntityPipeline;
    class EntityTextureAtlas;
    struct EntityMesh;
}
```
