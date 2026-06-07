# 天气渲染模块 (Weather Renderer)

## 目录结构

```
weather/
├── README.md              # 本文件
├── WeatherRenderer.hpp    # 天气渲染器头文件，定义常量和类接口
└── WeatherRenderer.cpp    # 天气渲染器实现，雨/雪层生成和渲染
```

## 模块职责

天气渲染器负责渲染雨雪效果，参考 MC 1.16.5 `WorldRenderer.renderRainSnow()`。使用纹理层而非单独粒子渲染大量雨雪，根据生物群系温度决定降水类型（雨/雪），支持海拔温度修正和地形高度感知。

## 内部模块关系

单文件模块，无内部子模块。`WeatherRenderer` 类封装了所有天气渲染逻辑。

## 上下游外部依赖

### 上游依赖（本模块依赖的）

- `ClientWorld`：世界查询接口（生物群系、高度、光照）
- `Biome`：生物群系温度和降水类型判断
- `VulkanUtils`：Vulkan 资源创建工具
- `Random`：随机数生成（位置种子）
- `mc::math::lerp`：线性插值
- `Frustum`：视锥剔除

### 下游依赖（依赖本模块的）

- `TridentRenderer`：主渲染器在帧渲染流程中调用天气渲染

## 容易踩的坑

### 光照采样位置

MC 在地面高度 `l2` 处采样光照，而非相机高度。`l2 = max(groundY, cameraY)`。

### 温度阈值判断

MC 使用 `biome.getTemperature(pos) >= 0.15f` 判断雨/雪。温度基于位置计算，高海拔更冷。

### 生物群系降水类型

即使温度足够，某些生物群系（沙漠、下界）也不降水，需检查 `biome->climate().precipitation`。

### 随机偏移数组

MC 使用归一化的方向向量（从中心向外辐射），而非随机值：`offsetX = -f1 / f2`, `offsetZ = f / f2`，其中 `f = j - 16`, `f1 = i - 16`, `f2 = sqrt(f*f + f1*f1)`。

### 高度计算

MC 的 j2/k2/l2 计算：
- j2 = camY - radius（下边界初始值），不低于 groundY
- k2 = camY + radius（上边界初始值），不低于 groundY
- l2 = max(groundY, camY)（光照采样高度）

### 雨和雪的 Alpha 公式不同

- 雨：`(1 - dist²/radius²) * 0.5 + 0.5`
- 雪：`(1 - dist²/radius²) * 0.3 + 0.5`

### 雪的光照增强公式

雪需要更亮的光照效果：`blockLight = (raw * 9 + 240) / 4`, `skyLight = (raw * 3 + 240) / 4`。
