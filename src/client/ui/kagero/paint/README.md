# paint - 绘制抽象层

本目录是 Kagero UI 框架的平台无关绘制抽象层，封装画布、画笔、路径、图像等绘制原语，屏蔽底层渲染后端（Trident/Vulkan）的差异。

## 目录结构

```
paint/
├── Color.hpp                # 颜色定义与工具函数（ARGB/u32 转换等）
├── Geometry.hpp/cpp         # 几何类型（Rect, RRect, Point, Size, Matrix）及变换
├── PaintContext.hpp/cpp     # 绘图上下文，封装 ICanvas 操作，Widget 通过此上下文绘制
├── TextureImage.hpp/cpp     # 纹理图像实现（持有 Vulkan 图像视图/采样器引用）
└── contracts/               # 接口定义（供渲染后端实现）
    ├── ICanvas.hpp          # 画布接口（drawRect/drawImage/drawText 等绘图操作）
    ├── IPaint.hpp           # 画笔接口（颜色、样式、混合模式等配置）
    ├── IPath.hpp            # 路径接口（矢量图形）
    ├── IImage.hpp           # 图像接口（width/height/format/debugName）
    ├── ISurface.hpp         # 绘图表面接口
    ├── ITypeface.hpp        # 字体接口
    └── ITextBlob.hpp        # 文本块接口
```

## 内部模块关系

```
┌──────────────────────────────────────────────────┐
│                   PaintContext                    │
│            （Widget 层的唯一绘制入口）              │
└──────────────────────┬───────────────────────────┘
                       │ 委托
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   ICanvas    │ │    IPaint    │ │   IImage     │
│  （画布）     │ │  （画笔）     │ │  （图像）     │
└──────┬───────┘ └──────────────┘ └──────┬───────┘
       │ 实现由渲染后端提供                │ 实现
       ▼                                  ▼
┌──────────────────┐               ┌──────────────────┐
│  TridentCanvas   │               │  TextureImage    │
│ （Vulkan 实现）   │               │ （纹理引用封装）   │
└──────────────────┘               └──────────────────┘
```

## TextureImage

`TextureImage` 是 `IImage` 的具体实现，持有 Vulkan 图像视图（`VkImageView`）和采样器（`VkSampler`）的**非拥有引用**，纹理资源的生命周期由外部（如 `GuiSpriteAtlas`）管理。

### 构造函数

```cpp
TextureImage(VkImageView imageView,
    VkSampler sampler,
    i32 width,
    i32 height,
    f32 u0 = 0.0f,
    f32 v0 = 0.0f,
    f32 u1 = 1.0f,
    f32 v1 = 1.0f,
    u8 atlasSlot = 1,
    std::string debugName = std::string(),
    ImageFormat format = ImageFormat::RGBA8);
```

| 参数 | 说明 |
|------|------|
| `imageView` | Vulkan 图像视图，`VK_NULL_HANDLE` 表示无效纹理 |
| `sampler` | Vulkan 采样器 |
| `width`/`height` | 图像尺寸（像素） |
| `u0`/`v0`/`u1`/`v1` | 纹理坐标，默认覆盖整张纹理 `[0,1]` |
| `atlasSlot` | 图集槽位 ID（0=字体、1=物品图集、2+=GUI 图集） |
| `debugName` | 调试名称，用于日志与诊断 |
| `format` | 纹理像素格式，默认 `RGBA8` |

### ImageFormat 像素格式

`ImageFormat` 枚举定义在 `contracts/IImage.hpp`，用于描述纹理的像素通道布局：

| 枚举值 | 通道布局 | 对应 Vulkan 格式 |
|--------|----------|------------------|
| `R8` | 单通道红 | `VK_FORMAT_R8_UNORM` |
| `RG8` | 红绿双通道 | `VK_FORMAT_R8G8_UNORM` |
| `RGB8` | 红绿蓝三通道 | `VK_FORMAT_R8G8B8_UNORM` |
| `RGBA8` | 红绿蓝-alpha 四通道 | `VK_FORMAT_R8G8B8A8_UNORM` |
| `BGRA8` | 蓝绿红-alpha 四通道 | `VK_FORMAT_B8G8R8A8_UNORM` |

`format` 参数允许调用方按实际纹理格式传入，使 `IImage::format()` 返回值与底层 Vulkan 纹理格式保持一致。当前唯一调用方 `GuiSpriteAtlas` 使用 `RGBA8`（对应图集的 `VK_FORMAT_R8G8B8A8_UNORM`）。

### 多图集支持

通过 `atlasSlot` 字段支持多图集纹理选择，渲染器据此选择对应的着色器描述符集：
- 槽位 0：字体纹理
- 槽位 1：物品纹理图集
- 槽位 2+：GUI 纹理图集（icons、widgets 等）

## 上下游外部依赖关系

### 被依赖方（谁使用了 paint）
- `widget/PaintContext` — Widget 层通过 PaintContext 间接使用 ICanvas/IPaint
- `TridentCanvas` — ICanvas 的 Vulkan 实现
- `GuiSpriteAtlas` — 创建 TextureImage 供 PaintContext 绘制 GUI 精灵

### 依赖方（paint 使用了谁）
- `contracts/IImage.hpp` — ImageFormat 枚举与 IImage 接口
- `common/core/Types.hpp` — 基础类型（i32, u32, f32, u8 等）
- Vulkan — VkImageView/VkSampler 句柄类型

## 容易踩的坑

### 1. TextureImage 不拥有纹理资源

`TextureImage` 只是 `VkImageView`/`VkSampler` 的引用持有者，析构时不会销毁 Vulkan 资源。资源生命周期由外部（如 `GuiSpriteAtlas`）管理，调用方须确保 TextureImage 的使用期不超过底层纹理的生命周期。

### 2. ImageFormat 须与实际 Vulkan 格式一致

构造 TextureImage 时传入的 `format` 参数应与创建 `VkImageView` 时使用的 `VkFormat` 一致，否则 `IImage::format()` 返回值会误导依赖该元数据的调用方。

### 3. PaintContext 状态栈

`PaintContext` 的 `save()`/`restore()` 必须配对使用，否则会导致绘图状态（裁剪、变换）错乱。建议使用 RAII 包装器管理状态栈。
