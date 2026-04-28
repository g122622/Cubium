# texture/ - 纹理模块

纹理模块负责纹理图集、动画精灵和纹理上传管理。

## 目录结构

```text
texture/
├── TridentTexture.hpp/cpp       # Vulkan 纹理封装
├── TridentTextureAtlas.hpp/cpp  # 纹理图集
├── AnimatedSprite.hpp/cpp       # 动画精灵
├── TextureAtlasTicker.hpp/cpp   # 动画更新管理器
└── README.md                     # 本文件
```

## 动画系统架构

### AnimatedSprite

管理单张动画纹理的帧数据和播放状态。

**MC 1.16.5 对齐要点**：

1. **帧切换逻辑**（参考 `TextureAtlasSprite.updateAnimation()`）：
   - `++tickCounter`，检查是否 `>= frameTime`
   - 切换帧：`frameCounter = (frameCounter + 1) % frameCount`
   - 重置 `tickCounter = 0`
   - 只有帧索引变化时才标记 `needsUpload = true`

2. **插值模式**：
   - 如果 `interpolate = true`，每个 tick 都需要上传插值帧
   - 插值公式：`mix(ratio, currentColor, nextColor)`

### TextureAtlasTicker

管理所有动画精灵，在游戏 tick 中更新动画状态。

**使用方式**：
```cpp
// 每游戏 tick 调用
textureAtlasTicker.tick();

// 渲染前上传待更新帧
textureAtlasTicker.uploadPendingFrames(context, atlas);
```

### AnimationMetadata

解析 `.mcmeta` 文件中的动画配置。

**字段映射**：
| mcmeta 字段 | C++ 字段 | 说明 |
|------------|----------|------|
| `frametime` | `frametime` | 默认帧时间（tick） |
| `width` | `width` | 帧宽度（-1 = 自动检测） |
| `height` | `height` | 帧高度（-1 = 自动检测） |
| `interpolate` | `interpolate` | 是否启用帧间插值 |
| `frames[]` | `frames` | 自定义帧序列 |

**自动帧尺寸计算**：
- 如果 width 和 height 都为 -1，使用 `min(imageWidth, imageHeight)`
- 参考 MC 1.16.5 `AnimationMetadataSection.getFrameSize()`

## 与 MC 1.16.5 的对齐验证

| 功能 | MC 1.16.5 实现 | 当前实现 | 状态 |
|------|----------------|----------|------|
| 帧计数器递增 | `frameCounter = (frameCounter + 1) % j` | ✅ 一致 | ✅ |
| 帧时间获取 | `getFrameTimeSingle(frameCounter)` | ✅ 一致 | ✅ |
| 插值帧上传 | `InterpolationData.uploadInterpolated()` | ⚠️ TODO | 待实现 |
| 纹理子区域上传 | `uploadTextureSub()` | ⚠️ TODO | 待实现 |

## 测试覆盖

动画元数据解析和帧切换逻辑需要单元测试覆盖：
- `AnimationMetadata.fromJson()` 解析测试
- `AnimatedSprite.tick()` 帧切换测试
- `AnimatedSprite.getInterpolatedFrame()` 插值测试
