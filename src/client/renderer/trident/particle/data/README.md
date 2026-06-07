# 粒子数据 (Particle Data)

## 概述

粒子数据类用于携带粒子参数，支持网络序列化和命令行解析。参考 MC 1.16.5 IParticleData 接口。

## 目录结构

```
data/
├── ParticleData.hpp           # 粒子数据基类，定义 getType()/getTypeName()/getParameters()/clone() 接口
├── BasicParticleData.hpp/cpp  # 无参数粒子数据（火焰、烟雾等）
├── BlockParticleData.hpp/cpp  # 方块粒子数据，携带 BlockState 参数
├── ItemParticleData.hpp/cpp   # 物品粒子数据，携带 ItemStack 参数
├── RedstoneParticleData.hpp/cpp # 红石粒子数据，携带 RGB 颜色参数
└── README.md
```

## 内部模块关系

```
ParticleData (基类)
    ├── BasicParticleData (无参数粒子)
    ├── BlockParticleData (方块粒子，依赖 BlockState)
    ├── ItemParticleData (物品粒子，依赖 ItemStack)
    └── RedstoneParticleData (红石粒子，依赖 glm::vec3 颜色)
```

所有具体粒子数据类都继承自 `ParticleData` 基类，实现 `getType()`、`getTypeName()`、`getParameters()`、`clone()` 接口。

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
| `client/renderer/trident/particle/particles/` | 具体粒子实现（如 DiggingParticle 使用 BlockParticleData） |
| 服务端粒子包序列化 | 网络同步粒子效果 |
| `/particle` 命令解析 | 命令行参数解析 |

## 容易踩的坑

1. **粒子类型与数据类匹配**：创建粒子数据时必须使用正确的类型，否则会触发断言失败
   - `BlockParticleData` 必须使用 `ParticleTypeId::Block`、`Breaking`、`FallingDust`
   - `ItemParticleData` 必须使用 `ParticleTypeId::Item`、`ItemSlime`、`ItemSnowball`
   - 使用 `requiresBlockState()` / `requiresItemData()` / `requiresDustColor()` 辅助函数检查

2. **颜色范围**：`RedstoneParticleData` 的颜色分量会在构造函数中被 clamp 到 [0, 1] 范围，传入时注意不需要手动 clamp

3. **类型名称解析**：通过 `BasicParticleData(const std::string& typeName)` 构造时，如果名称无效会得到 `ParticleTypeId::Invalid`，需要调用方检查 `isValidParticleType()`

4. **BlockState 的 modelKey**：`BlockParticleData::getParameters()` 会输出方块状态的属性参数，格式如 `minecraft:stone[variant=stone]`
