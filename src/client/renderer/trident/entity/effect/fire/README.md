# 着火效果

本目录包含实体着火效果实现。

## 目录结构

```
fire/
├── FireEffect.hpp      # 着火效果渲染器头文件
└── FireEffect.cpp      # 着火效果渲染器实现（纹理加载、billboard 渲染）
```

## 内部模块关系

`FireEffect` 是纯静态工具类，无内部模块划分。

核心渲染流程：
1. `_loadFireTexture()` 从资源包加载 fire_0.png / fire_1.png 纹理
2. `_renderFireLayers()` 循环绘制多层火焰 billboard
3. `_generateFireQuad()` 生成火焰四边形网格

## 上下游外部依赖关系

**本目录依赖的上游模块**：
- `client/renderer/trident/entity/model/core/ModelRenderer.hpp` - 模型顶点类型
- `client/renderer/trident/entity/pipeline/EntityPipeline.hpp` - 实体渲染管线
- `client/world/entity/ClientEntity.hpp` - 客户端实体
- `common/entity/core/Entity.hpp` - 实体基类
- `common/resource/pack/IResourcePack.hpp` - 资源包接口
- `common/util/math/MathConstants.hpp` - 数学常量
- `common/util/math/Vector3.hpp` - 向量类型

**依赖本目录的下游模块**：
- `client/renderer/trident/core/TridentEngine.cpp` - 初始化/清理调用
- `client/renderer/trident/entity/core/EntityRendererManager.cpp` - 渲染时调用

## 容易踩的坑

### 1. 火焰纹理路径

火焰纹理从资源包加载，路径为 `textures/block/fire_0.png` 和 `textures/block/fire_1.png`。如果资源包中不存在，会自动生成程序化纹理作为后备。因此初始化不会失败，但纹理质量可能不符合预期。

### 2. Billboard 朝向

火焰使用两个互相垂直的 billboard 实现"始终面向相机"效果。在 `_renderFireLayers()` 中，使用实体 yaw 角作为相机 yaw 角，这是一个简化假设。如果相机朝向计算不准确，火焰可能不会正确朝向玩家。

### 3. 多层火焰渲染参数

MC 1.16.5 的火焰渲染使用迭代参数：
- `f1`：火焰半宽，每层乘以 0.9 递减
- `f3`：高度迭代次数，每层减 0.45
- `f4`：Y 偏移累计，每层加 0.45

修改这些参数会影响火焰的视觉高度和层数，需参考原版调整。

### 4. 全亮光照值

火焰使用固定全亮光照值 `FULL_LIGHT = 15728640 (0xF00000)`，确保火焰在任何环境下都可见。

### 5. Vulkan 资源生命周期

`FireEffect` 持有 Vulkan 资源（Image、ImageView、Sampler、Memory），必须在 `cleanup()` 中正确销毁。初始化和清理必须配对调用，否则会泄漏 GPU 资源。
