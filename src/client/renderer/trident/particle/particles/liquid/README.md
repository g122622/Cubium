# 液体粒子 (Liquid Particles)

## 概述

液体粒子用于实现水滴、熔岩滴、蜂蜜滴等液体滴落效果。

## 文件

| 文件 | 描述 |
|------|------|
| DripParticle.hpp/cpp | 液体滴落粒子基类 - 实现悬挂、积累、下落机制 |

## DripParticle 基类

液体滴落粒子的基类，实现了一个三阶段生命周期：

### 生命周期阶段

1. **Hanging（悬挂）**
   - 粒子从方块下方悬挂
   - 缓慢积累变大（dripProgress 从 0 到 1）
   - 位置基本不变，有微小摆动

2. **Falling（下落）**
   - 积累满后开始下落
   - 受重力影响加速
   - 速度限制在终端速度

3. **Landed（落地）**
   - 检测与方块的碰撞
   - 触发落地效果（可被子类重写）
   - 粒子消失

### 子类化

子类可以重写以下方法：

```cpp
class WaterDripParticle : public DripParticle {
protected:
    void tickHanging(ClientWorld* world) override;
    void tickFalling(ClientWorld* world) override;
    void onLand(ClientWorld* world) override;
};
```

## 计划中的子类

| 类名 | 描述 |
|------|------|
| DripWaterParticle | 水滴 - 悬挂于含水方块下方 |
| DripLavaParticle | 熔岩滴 - 悬挂于熔岩方块下方，发光 |
| DripHoneyParticle | 蜂蜜滴 - 悬挂于蜂蜜方块下方，粘稠下落 |
| FallingWaterParticle | 下落的水滴 |
| FallingLavaParticle | 下落的熔岩滴 |
| LandingLavaParticle | 落地的熔岩滴 |

## 参考

- Minecraft Java 1.16.5 `net.minecraft.client.particle.DripParticle`
