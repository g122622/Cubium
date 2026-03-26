# Dimension 模块

维度渲染设置模块，定义各维度特有的渲染参数。

## 目录结构

```
dimension/
└── DimensionRenderSettings.hpp    # 维度渲染设置
```

## 文件详解

### DimensionRenderSettings.hpp

**职责**: 定义维度的渲染相关参数，包括云高度、天空、天花板、雾类型等设置。

**主要内容**:

#### FogType 枚举
定义雾类型，参考 MC 1.16.5 DimensionRenderInfo.FogType：
- `None` (0): 无雾
- `Normal` (1): 普通雾
- `End` (2): 末地雾

#### DimensionRenderSettings 结构体
定义维度渲染参数：

| 字段 | 类型 | 说明 |
|------|------|------|
| `cloudHeight` | `f32` | 云高度（NaN 表示无云） |
| `hasSky` | `bool` | 是否有天空 |
| `hasCeiling` | `bool` | 是否有天花板（下界为 true） |
| `fogType` | `FogType` | 雾类型 |
| `hasNaturalLight` | `bool` | 是否有自然光照 |
| `name` | `const char*` | 维度名称（调试用） |

**静态工厂方法**:
- `overworld()` - 主世界设置（云高度 192，有天空，普通雾）
- `nether()` - 下界设置（无云，有天花板，无自然光）
- `end()` - 末地设置（无云，末地雾）
- `getDefault()` - 默认设置（返回主世界）

**成员方法**:
- `hasClouds()` - 检查该维度是否有云（通过判断 cloudHeight 是否为 NaN）

## 文件关系图

```
                    ┌──────────────────────────────────┐
                    │     DimensionRenderSettings.hpp  │
                    │         (维度渲染设置)            │
                    └──────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
                    ▼               ▼               ▼
           ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐
           │ CloudRenderer│  │  其他渲染器  │  │  测试用例        │
           │ (客户端渲染)  │  │ (天空/雾等)  │  │ test_cloud_     │
           └─────────────┘  └─────────────┘  │ renderer.cpp    │
                                               └─────────────────┘
```

## 与相关模块的关系

### 与 DimensionSettings.hpp 的区别

| 模块 | 位置 | 用途 |
|------|------|------|
| **DimensionRenderSettings** | `world/dimension/` | 渲染参数（云高度、雾类型等） |
| **DimensionSettings** | `world/gen/settings/` | 生成参数（噪声设置、默认方块、海平面等） |

- `DimensionSettings` 用于世界生成阶段
- `DimensionRenderSettings` 用于客户端渲染阶段

## 模块概述

### 整体职责

定义各维度（主世界、下界、末地）的渲染相关参数，供客户端渲染器使用。

### 输入

- 无运行时输入，所有设置通过静态工厂方法预定义

### 输出

- `DimensionRenderSettings` 结构体实例，包含维度的渲染参数

### 依赖项

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（`f32`, `u8`, `bool` 等） |
| `<cmath>` | `std::isnan()` 用于检测 NaN |
| `<limits>` | `std::numeric_limits<f32>::quiet_NaN()` |

### 使用方法

```cpp
#include "common/world/dimension/DimensionRenderSettings.hpp"

// 获取主世界渲染设置
auto settings = DimensionRenderSettings::overworld();

// 检查是否有云
if (settings.hasClouds()) {
    // 在 settings.cloudHeight 高度渲染云
    renderClouds(settings.cloudHeight);
}

// 检查是否有天空
if (settings.hasSky) {
    renderSky();
}

// 根据雾类型设置渲染
switch (settings.fogType) {
    case FogType::None:
        disableFog();
        break;
    case FogType::Normal:
        enableNormalFog();
        break;
    case FogType::End:
        enableEndFog();
        break;
}

// 检查自然光照
if (settings.hasNaturalLight) {
    updateSkyLight();
}
```

### 容易踩的坑

1. **NaN 检查**: `cloudHeight` 使用 NaN 表示无云，必须使用 `std::isnan()` 检查，不能直接比较
   ```cpp
   // 错误！NaN 不等于自身
   if (settings.cloudHeight == settings.cloudHeight) { ... }

   // 正确
   if (!std::isnan(settings.cloudHeight)) { ... }
   // 或使用 hasClouds() 方法
   if (settings.hasClouds()) { ... }
   ```

2. **与 DimensionSettings 混淆**: 注意区分 `DimensionRenderSettings`（渲染）和 `DimensionSettings`（生成）

3. **指针类型**: `name` 字段是 `const char*`，指向静态字符串常量，不要尝试修改或释放

4. **预设值不可变**: 工厂方法返回的是临时对象，如果需要持久化请复制
   ```cpp
   // 每次调用都创建新对象
   auto settings1 = DimensionRenderSettings::overworld();
   auto settings2 = DimensionRenderSettings::overworld();
   // settings1 和 settings2 是独立的副本
   ```

## 涉及的测试用例

测试文件位置: `tests/client/renderer/test_cloud_renderer.cpp`

| 测试用例 | 说明 |
|----------|------|
| `DimensionRenderSettingsTest.OverworldSettings` | 测试主世界设置（云高度、天空、雾类型等） |
| `DimensionRenderSettingsTest.NetherSettings` | 测试下界设置（无云、有天花板、无自然光） |
| `DimensionRenderSettingsTest.EndSettings` | 测试末地设置（无云、末地雾） |
| `DimensionRenderSettingsTest.DefaultSettings` | 测试 `getDefault()` 返回主世界设置 |
| `DimensionRenderSettingsTest.HasCloudsMethod` | 测试 `hasClouds()` 方法正确性 |
| `DimensionRenderSettingsTest.FogTypeEnumValues` | 测试雾类型枚举值正确 |

## 未来扩展

当前模块仅包含渲染设置，完整的维度系统可能还需要：

- `Dimension.hpp` - 维度基类
- `DimensionType.hpp` - 维度类型枚举
- `DimensionManager.hpp` - 维度管理器

这些功能目前在 `world/gen/settings/DimensionSettings.hpp` 中有生成相关的设置。
