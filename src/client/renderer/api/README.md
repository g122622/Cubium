# Renderer API 模块

## 概述

`api` 目录是渲染系统的平台无关抽象层，为不同渲染后端（Vulkan、OpenGL、DirectX、Metal）提供统一的接口定义。该模块定义了渲染引擎的核心接口、数据结构和状态管理，是客户端渲染系统的基础。

## 目录结构树

```
api/
├── BlendMode.hpp                # 混合因子、混合操作、混合状态定义
├── CompareOp.hpp                # 深度比较操作、深度状态定义
├── CullMode.hpp                 # 面剔除模式、正面朝向、光栅化状态定义
├── IRenderEngine.hpp            # 渲染引擎主接口（核心入口点）
├── TridentApi.hpp               # 统一头文件（包含所有 API）
├── Types.hpp                    # 缓冲区类型枚举（BufferUsage/MemoryType/IndexType）
├── buffer/
│   └── IBuffer.hpp              # 缓冲区接口（顶点、索引、Uniform、暂存）
├── camera/
│   ├── CameraConfig.hpp         # 相机配置结构（FOV、宽高比、裁剪面等）
│   └── ICamera.hpp              # 相机接口（视图/投影矩阵控制）
├── pipeline/
│   ├── IPipeline.hpp            # 渲染管线、管线布局、描述符集接口
│   ├── RenderState.hpp          # 渲染状态组合（Blend+Depth+Rasterizer）
│   └── RenderType.hpp           # 命名渲染类型（MC 1.16.5 风格）
└── texture/
    ├── ITexture.hpp             # 纹理接口、纹理格式、采样器设置
    ├── ITextureAtlas.hpp        # 纹理图集构建器接口
    └── TextureRegion.hpp        # 纹理区域（UV 坐标封装）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        TridentApi.hpp                           │
│                      （统一头文件入口）                           │
└─────────────────────────────────────────────────────────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        ▼                       ▼                       ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│    Types      │     │  RenderState  │     │ IRenderEngine │
│  （基础类型）   │     │ （状态组合）   │     │  （核心接口）   │
└───────────────┘     └───────────────┘     └───────────────┘
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│   IBuffer     │     │  BlendMode    │     │   ICamera     │
│  （缓冲区）    │     │  CompareOp    │     │   IPipeline   │
│               │     │   CullMode    │     │   ITexture    │
└───────────────┘     └───────────────┘     └───────────────┘
```

**依赖链**：
- `RenderState` 组合了 `BlendState`、`DepthState`、`RasterizerState`
- `RenderType` 封装了 `RenderState` + 名称 + 排序索引
- `IRenderEngine` 依赖所有子模块（Buffer、Camera、Pipeline、Texture、Mesh）
- `TridentApi.hpp` 作为统一头文件包含所有接口

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖 | 用途 |
|------|------|
| `glm` | 数学库（vec3, mat4, quaternion） |
| `common/core/Types.hpp` | 基础类型定义（u8, u32, f64 等） |
| `common/core/Result.hpp` | 错误处理 |
| `common/resource/ResourceLocation.hpp` | 资源定位符 |
| `common/util/math/MathUtils.hpp` | 数学工具 |

### 依赖本模块的外部模块

| 模块 | 说明 |
|------|------|
| `client/renderer/trident/` | Vulkan 后端实现 |
| `client/renderer/` | 上层渲染器（区块渲染、实体渲染、GUI 渲染等） |

## 容易踩的坑

### 1. 帧同步问题 - Uniform 缓冲区数据竞争

在多帧在飞（frames in flight）时，直接更新 Uniform 缓冲区可能导致数据竞争。

**解决方案**：使用 `IUniformBuffer` 的多帧轮换功能：
```cpp
// 创建时指定帧数
auto ubo = engine->createUniformBuffer(sizeof(UBO), 2);  // 双缓冲

// 每帧切换
engine->beginFrame();
ubo->advanceFrame();  // 切换到当前帧的缓冲区
ubo->upload(&data, sizeof(data));
```

### 2. 纹理格式不匹配

上传的像素数据格式与纹理描述中的格式不匹配会导致渲染异常或崩溃。
- `R8G8B8A8_UNORM` 需要 4 字节/像素
- `R8G8B8A8_SRGB` 需要 4 字节/像素，且会进行 gamma 校正
- 压缩格式（BC1-BC7）需要预压缩的数据

### 3. 渲染状态遗漏

忘记设置某些渲染状态会导致渲染异常。

**推荐做法**：使用 `RenderType` 预设，而不是手动设置每个状态：
```cpp
// 推荐
engine->setRenderType(RenderType::translucent());

// 不推荐 - 容易遗漏
// engine->setBlendState(BlendState::alpha());
// engine->setDepthState(DepthState::readOnly());
```

### 4. 面剔除方向错误

不同图形 API 的坐标系约定不同，可能导致面剔除方向错误。项目使用 Vulkan 约定（顺时针为正面 `FrontFace::Clockwise`）。如果模型数据使用逆时针，需要在 `RasterizerState` 中调整 `frontFace`。

### 5. 纹理图集 UV 坐标精度问题

直接使用像素坐标计算 UV 可能导致精度问题。

**解决方案**：使用 `ITextureAtlas::getRegion()` 获取精确的 UV 坐标：
```cpp
// 正确
TextureRegion region = atlas->getRegion(tileX, tileY);
vertex.u = region.u0;
vertex.v = region.v0;
```

### 6. 顶点格式与着色器布局不匹配

CPU 端的 `Vertex` 结构体（定义于 `client/renderer/MeshTypes.hpp`，由区块管线直接消费）必须与着色器中的顶点输入布局严格一致（位置、UV、颜色、光照的顺序和类型）。

### 7. 深度缓冲精度问题（z-fighting）

远距离物体出现深度冲突通常是近平面设置过小导致的。推荐配置：
```cpp
config.nearPlane = 0.1f;   // 不要设得太小
config.farPlane = 1000.0f; // 根据实际需要设置
```
