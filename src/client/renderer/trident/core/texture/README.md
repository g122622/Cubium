# texture/ - 纹理模块

纹理模块负责 Vulkan 纹理创建、纹理图集和动画精灵管理。

## 目录结构

```
texture/
├── TridentTexture.hpp/cpp       # Vulkan 纹理和纹理图集实现
├── AnimatedSprite.hpp/cpp       # 动画精灵（帧动画管理）
├── TextureAtlasTicker.hpp/cpp   # 动画精灵更新管理器
└── README.md                     # 本文件
```

## 内部模块关系

```
TridentTexture（Vulkan 纹理封装，实现 ITexture 接口）
├── 图像创建/销毁（VkImage/VkImageView/VkSampler）
├── 数据上传（upload/uploadRegion）
└── 布局转换和 mipmap 生成

TridentTextureAtlas（纹理图集）
├── 持有一个 TridentTexture
├── 提供瓦片级别的区域查询（getRegion）
└── 支持子区域上传（uploadRegion，用于动画帧更新）

AnimatedSprite（动画精灵）
├── 依赖 AnimationMetadata（来自 resource/metadata）
├── 管理帧数据和播放状态
└── 上传帧数据到图集（uploadCurrentFrame）

TextureAtlasTicker（动画管理器）
├── 持有多个 AnimatedSprite 共享指针
├── 每 tick 更新动画状态
└── 批量上传待更新帧到图集
```

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `api/ITexture`、`api/ITextureAtlas` - 平台无关的渲染抽象接口
- `common/resource/metadata/AnimationMetadata` - 动画元数据解析
- `common/core/Result` - 错误处理
- Vulkan SDK - 图形 API

**下游依赖（被谁使用）：**
- `TridentEngine` - 持有 `TextureAtlasTicker` 实例管理方块/物品图集动画
- `TextureAtlasBuilder` - 构建图集后生成 `AnimationDescriptor`，用于创建 `AnimatedSprite`
- `BlockEntityRenderer` - 使用纹理相关功能

## 容易踩的坑

1. **动画纹理生命周期**：动画纹理需要在主线程每 tick 调用 `TextureAtlasTicker::tick()` 更新帧状态，在渲染前调用 `uploadPendingFrames()` 上传到 GPU。如果忘记调用会导致动画卡住或闪烁。

2. **纹理子区域上传**：`uploadRegion()` 的 `rowLength` 参数用于精灵表场景，0 表示紧密排列。如果源数据行长度与目标区域宽度不同，必须正确设置，否则会出现纹理错位。

3. **插值模式性能开销**：启用 `interpolate = true` 时，每个 tick 都需要上传插值帧，GPU 计算开销也会增加。对性能敏感场景慎用。

4. **图集尺寸限制**：`TridentTextureAtlas` 创建时指定的 `tileSize` 必须与实际纹理瓦片尺寸匹配，否则 `getRegion()` 返回的 UV 坐标会出错。

5. **线程安全**：`AnimatedSprite` 和 `TextureAtlasTicker` 不是线程安全的。`tick()` 应在主线程调用，`uploadCurrentFrame()` 和 `uploadPendingFrames()` 应在渲染线程调用。
