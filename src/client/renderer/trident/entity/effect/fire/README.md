# 着火效果

本目录包含实体着火效果实现，支持基于 `.mcmeta` 的火焰纹理动画。

## 目录结构

```
fire/
├── FireEffect.hpp              # 着火效果渲染器头文件（静态类）
├── FireEffect.cpp              # 着火效果渲染器实现（纹理加载、tick 推进、billboard 渲染）
├── FireAnimationState.cpp      # 火焰动画播放状态机（双计数器帧推进）
├── FireTextureLoader.hpp       # 纹理解码接口（纯 CPU，可单元测试）
└── FireTextureLoader.cpp       # 纹理解码实现（PNG + mcmeta 解析、帧提取）
```

## 内部模块关系

`FireEffect` 是纯静态工具类，持有独立的 Vulkan 纹理资源。

核心数据流：
1. `loadFireTextureData()` 从资源包解码 fire_0.png / fire_1.png 及其 .mcmeta
   - 纯 CPU 操作，不依赖 Vulkan，便于单元测试
   - 读取同名 `.png.mcmeta` 动画元数据（frametime、frames 序列、interpolate）
   - 从动画条带提取**所有**帧（而非仅首帧），纵向拼接为单条像素缓冲区
   - 输出 `FireTextureData`：`[fire_0 全部帧][fire_1 全部帧]` 拼接 + 两组 `AnimationMetadata`
2. `FireEffect::loadTexture()` 将解码结果上传为单张 Vulkan VkImage（所有帧预上传）
3. `FireEffect::tick()` 每 tick 推进 fire_0 / fire_1 的帧计数器（双计数器模式）
4. `_renderFireLayers()` 渲染时通过 UV 偏移选择当前动画帧（无需逐帧上传）

`FireAnimationState` 采用与 `AnimatedSprite` 一致的双计数器模式：
- `tickCounter` 累加 tick，达到 `currentFrameTime` 后切换帧
- 支持每帧独立时长（mcmeta `frames[].time`）
- 支持自定义帧序列（mcmeta `frames` 数组）和模运算循环

## 上下游外部依赖关系

**本目录依赖的上游模块**：
- `client/renderer/trident/entity/model/core/ModelRenderer.hpp` - 模型顶点类型
- `client/renderer/trident/entity/pipeline/EntityPipeline.hpp` - 实体渲染管线
- `client/world/entity/ClientEntity.hpp` - 客户端实体
- `common/entity/core/Entity.hpp` - 实体基类
- `common/resource/metadata/AnimationMetadata.hpp` - 动画元数据解析
- `common/resource/pack/IResourcePack.hpp` - 资源包接口
- `common/util/assert/AssertAll.hpp` - 运行时断言
- `common/util/math/MathConstants.hpp` - 数学常量
- `common/util/math/Vector3.hpp` - 向量类型

**依赖本目录的下游模块**：
- `client/renderer/trident/core/TridentEngine.cpp` - 初始化/清理/热重载/tick 推进调用
- `client/renderer/trident/entity/core/EntityRendererManager.cpp` - 渲染时调用
- `client/application/features/ClientApplicationResource.cpp` - 资源热重载时调用 `TridentEngine::reloadFireTexture`

## 容易踩的坑

### 1. 火焰纹理路径

火焰纹理从资源包加载，路径为 `minecraft/textures/block/fire_0.png` 和 `minecraft/textures/block/fire_1.png`（`readResource` 仅自动补 `assets/` 前缀，命名空间 `minecraft/` 必须显式给出）。如果资源包中不存在，会自动生成程序化纹理作为后备。

### 1.1 动画帧提取与 mcmeta 解析

原版 `fire_0.png` / `fire_1.png` 为 16x16 单帧，但资源包可能提供 16x512 动画条带（32 帧）。`loadFireTextureData` 会：

1. 读取同名 `.png.mcmeta` 文件，用 `AnimationMetadata::fromMcmeta()` 解析动画配置
2. 从 mcmeta 获取帧尺寸（`width`/`height`），无 mcmeta 时按"帧高=帧宽"启发式
3. 从条带提取**所有**帧（而非仅首帧），纵向拼接为单条像素缓冲区
4. 保留 mcmeta 中的自定义帧序列（如原版 `fire_0.png.mcmeta` 的 `[16..31, 0..15]` 相位偏移）

帧尺寸必须能整除图像尺寸，否则降级为单帧处理。

### 1.2 纹理布局与 UV 计算

纹理布局为 `[fire_0 全部帧][fire_1 全部帧]` 纵向拼接。渲染时：
- 单帧 V 范围 = `1.0 / totalFrames`（totalFrames = fire0FrameCount + fire1FrameCount）
- fire_0 帧的 V 偏移 = `fire0FrameIndex / totalFrames`
- fire_1 帧的 V 偏移 = `(fire0FrameCount + fire1FrameIndex) / totalFrames`
- 偶数层用 fire_0，奇数层用 fire_1（MC 原版 `layer % 2` 行为）
- 每两层翻转 UV（MC 原版 `(layer/2) % 2` 行为）

所有帧在 `loadTexture()` 时一次性上传到 GPU，动画推进仅改变 UV 偏移，无需逐帧上传。

### 1.3 初始化与纹理注入分离

`FireEffect::initialize()` 仅建立 Vulkan 句柄并生成程序化占位纹理，不访问资源包。真实火焰纹理在 `TridentEngine::initializeEntityTextureAtlas()` 中通过 `FireEffect::loadTexture(packs)` 注入；资源热重载时由 `TridentEngine::reloadFireTexture()` 重新注入。`loadTexture` 内部会 `vkDeviceWaitIdle` 并销毁旧纹理，可安全重复调用，同时重置动画播放状态。

### 1.4 动画 tick 集成

`FireEffect::tick()` 由 `TridentEngine::tickTextureAnimations()` 调用，与方块/物品图集动画共享同一 tick 推进时机。由于火焰纹理所有帧已预上传，tick 仅推进帧计数器，无需在 `uploadAnimationFrames()` 中做任何上传操作。

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

`FireEffect` 持有 Vulkan 资源（Image、ImageView、Sampler、Memory），必须在 `cleanup()` 中正确销毁。初始化和清理必须配对调用，否则会泄漏 GPU 资源。`loadTexture()` 热重载前通过内部 `_destroyFireTexture()` 销毁旧资源，再重新创建，不会泄漏。
