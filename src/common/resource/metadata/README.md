# 资源元数据模块

此模块提供资源元数据的解析和管理，用于解析资源包中的`.mcmeta`文件。

## 目录结构

```text
src/common/resource/metadata/
└── AnimationMetadata.hpp/cpp   # 动画纹理元数据
```

## 文件介绍

### AnimationMetadata

解析`.png.mcmeta`文件中的动画配置。

**mcmeta文件格式示例**：
```json
{
  "animation": {
    "frametime": 5,
    "width": 16,
    "height": 16,
    "interpolate": true,
    "frames": [
      0,
      {"index": 1, "time": 10},
      2,
      3
    ]
  }
}
```

**主要字段**：
- `frametime`：每帧默认持续时间（游戏tick），默认1
- `width`/`height`：单帧尺寸，-1自动检测
- `interpolate`：是否启用帧间颜色插值，默认false
- `frames`：自定义帧序列，可指定每帧的索引和时间

## 使用方法

```cpp
using namespace mc::resource::metadata;

// 从mcmeta数据解析
AnimationMetadata metadata = AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

// 获取帧信息
i32 frameCount = metadata.getFrameCount();
i32 frameIndex = metadata.getFrameIndex(0);  // 获取第一帧索引
i32 frameTime = metadata.getFrameTime(0);     // 获取第一帧时间

// 检查是否为有效动画
if (metadata.isValidAnimation()) {
    // 启用动画精灵
}
```

## 与MC 1.16.5的对应关系

| 本模块类 | MC 1.16.5 对应类 |
|---------|-----------------|
| `AnimationFrame` | `net.minecraft.client.resources.data.AnimationFrame` |
| `AnimationMetadata` | `net.minecraft.client.resources.data.AnimationMetadataSection` |

## 依赖项

- `nlohmann-json`：JSON解析
- `common/core/Types.hpp`：基础类型定义
