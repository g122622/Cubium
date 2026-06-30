# 粒子数据(Particle Data)

## 概述

粒子数据类用于携带粒子参数，支持网络序列化和命令行解析。

## 目录结构

```
data/
├── ParticleData.hpp              # 粒子数据基类，定义 getType() / getTypeName() / getParameters() / clone() 接口
├── BasicParticleData.hpp / cpp   # 无参数粒子数据（火焰、烟雾等）
├── BlockParticleData.hpp / cpp   # 方块粒子数据，携带 BlockState 参数
├── ItemParticleData.hpp / cpp    # 物品粒子数据，携带 ItemStack 参数
├── RedstoneParticleData.hpp / cpp # 红石粒子数据，携带 RGB 颜色参数
├── VibrationParticleData.hpp / cpp # 振动粒子数据，携带目标位置 Vector3d + 到达时间 i32
├── TrailParticleData.hpp / cpp   # 轨迹粒子数据，携带目标位置 Vector3d + 颜色 u32(ARGB) + 持续时间 i32
└── README.md
```

## 内部模块关系

```
ParticleData (基类)
    ├── BasicParticleData (无参数粒子)
    ├── BlockParticleData (方块粒子，依赖 BlockState)
    ├── ItemParticleData (物品粒子，依赖 ItemStack)
    ├── RedstoneParticleData (红石粒子，携带 glm::vec3 颜色)
    ├── VibrationParticleData (振动粒子，携带目标位置 Vector3d + 到达时间 i32)
    └── TrailParticleData (轨迹粒子，携带目标位置 Vector3d + 颜色 u32 + 持续时间 i32)
```

所有具体粒子数据类都继承自 `ParticleData` 基类，实现 `getType()`、`getTypeName()`、`getParameters()`、`clone()` 接口。

## 数据管线

粒子数据通过 `ParticleDataFactory` 传递到粒子创建流程：

1. **网络/命令层**：解析粒子数据，创建 `ParticleData` 子类实例
2. **注册层**：`ParticleRegistry::createParticleWithData()` 检查是否有数据工厂，有则调用数据工厂
3. **工厂层**：数据工厂从 `ParticleData*` 提取参数，调用 `createWithTarget()` 等方法创建粒子
4. **无数据回退**：当 `ParticleData` 为空或类型不匹配时，回退到 `create()` 使用默认值

已注册数据工厂的粒子类型：
- `Vibration`：从 `VibrationParticleData` 提取目标位置和到达时间
- `Trail`：从 `TrailParticleData` 提取目标位置、颜色和持续时间

## 上下游外部依赖关系

### 依赖的上游模块

| 模块 | 用途 |
|------|------|
| `common/world/block/Block.hpp` | BlockState 类型（BlockParticleData） |
| `common/item/core/ItemStack.hpp` | ItemStack 类型（ItemParticleData） |
| `client/renderer/trident/particle/ParticleTypes.hpp` | ParticleTypeId 枚举 |
| `client/renderer/trident/particle/ParticleRegistry.hpp` | 类型名称查找 |
| `common/util/assert/AssertAll.hpp` | 断言检查 |
| `glm/glm.hpp` | glm::vec3 颜色类型（RedstoneParticleData） |

### 被哪些下游模块依赖

| 模块 | 用途 |
|------|------|
| `client/renderer/trident/particle/ParticleRegistry.hpp` | 根据类型创建粒子数据 |
| `client/renderer/trident/particle/ParticleManager.hpp` | 携带粒子数据创建粒子 |
| `client/renderer/trident/particle/ParticleFactories.cpp` | 数据工厂注册 |
| `client/renderer/trident/particle/particles/` | 具体粒子实现（如 DiggingParticle 使用 BlockParticleData） |
| 服务端粒子包序列化 | 网络同步粒子效果 |
| `/particle` 命令解析 | 命令行参数解析 |

## 容易踩的坑

1. **粒子类型与数据类匹配**：创建粒子数据时必须使用正确的类型，否则会触发断言失败
   - `BlockParticleData` 必须使用 `ParticleTypeId::Block`、`Breaking`、`FallingDust`
   - `ItemParticleData` 必须使用 `ParticleTypeId::Item`、`ItemSlime`、`ItemSnowball`
   - 使用 `requiresBlockState()` / `requiresItemData()` / `requiresDustColor()` 辅助函数检查

2. **颜色范围**：`RedstoneParticleData` 的颜色分量会在构造函数中被 clamp 到[0, 1] 范围，传入时注意不需要手动 clamp

3. **类型名称解析**：通过 `BasicParticleData(const std::string& typeName)` 构造时，如果名称无效会得到 `ParticleTypeId::Invalid`，需要调用方检查 `isValidParticleType()`

4. **BlockState 的 modelKey**：`BlockParticleData::getParameters()` 会输出方块状态的属性参数，格式如 `minecraft:stone[variant=stone]`

5. **数据工厂回退**：当 `createParticleWithData()` 传入的 `ParticleData` 为 nullptr 或类型不匹配（dynamic_cast 失败）时，自动回退到普通 `create()` 工厂，使用默认值创建粒子
