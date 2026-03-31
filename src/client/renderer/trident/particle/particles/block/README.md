# 方块粒子 (Block Particles)

## 概述

方块粒子用于方块相关的视觉效果，如挖掘、破坏、下落灰尘等。

## 文件

| 文件 | 描述 |
|------|------|
| DiggingParticle.hpp/cpp | 挖掘粒子 - 挖掘方块时产生 |

## 特性

### DiggingParticle（挖掘粒子）

- **渲染类型**：TERRAIN_SHEET（使用方块纹理图集）
- **生命周期**：约 1 秒
- **行为**：
  - 受重力影响
  - 从方块位置向四周散射
  - 有物理碰撞
  - 落地后摩擦减速
- **颜色**：使用方块纹理原色
- **用途**：
  - 挖掘方块时
  - 破坏方块时
  - 方块下落时

## 用法

```cpp
// 创建挖掘粒子（需要方块状态）
BlockState state = BlockRegistry::get(BlockId::Stone).getDefaultState();
auto digging = DiggingParticle::createWithBlock(
    position,
    velocity,
    state
);
particleManager.addParticle(std::move(digging));
```

## 扩展粒子

后续可添加的其他方块粒子：
- **BreakingParticle**：方块破碎效果
- **FallingDustParticle**：下落灰尘
- **BlockCrackParticle**：方块裂纹

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.DiggingParticle`
