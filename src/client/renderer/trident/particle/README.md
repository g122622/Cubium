# 粒子系统 (Particle System)

## 概述

粒子系统负责管理和渲染游戏中的所有粒子效果，包括火焰、烟雾、爆炸、天气效果等。参考 Minecraft Java 1.16.5 的粒子系统架构实现。

## 目录结构

```
particle/
├── Particle.hpp/cpp              # 粒子基类
├── ParticleManager.hpp/cpp       # 粒子管理器
├── ParticleTypes.hpp             # 粒子类型 ID 枚举
├── ParticleRegistry.hpp/cpp      # 粒子类型注册表
├── ParticleTextureAtlas.hpp/cpp  # 粒子纹理图集
├── ParticleRenderType.hpp        # 渲染类型枚举
│
├── data/                         # 粒子数据定义
│   ├── ParticleData.hpp          # 粒子参数基类
│   ├── BasicParticleData.hpp/cpp # 无参数粒子数据
│   └── BlockParticleData.hpp/cpp # 方块粒子数据
│
├── sprite/                       # 精灵动画系统（待实现）
│   ├── ISprite.hpp               # 精灵接口
│   └── AnimatedSprite.hpp/cpp    # 动画精灵
│
└── particles/                    # 具体粒子实现
    ├── RainParticle.hpp/cpp      # 雨滴粒子
    ├── SnowParticle.hpp/cpp      # 雪花粒子
    │
    ├── ambient/                  # 环境粒子
    │   ├── BubbleParticle.hpp/cpp    # 水下气泡粒子
    │   ├── UnderwaterParticle.hpp/cpp # 水下悬浮粒子
    │   └── CloudParticle.hpp/cpp     # 云朵粒子
    │
    ├── block/                    # 方块粒子
    │   └── DiggingParticle.hpp/cpp   # 挖掘粒子
    │
    ├── effect/                   # 特效粒子
    │   ├── FlameParticle.hpp/cpp     # 火焰粒子
    │   ├── SmokeParticle.hpp/cpp     # 烟雾粒子
    │   ├── LavaParticle.hpp/cpp      # 熔岩滴粒子
    │   ├── PortalParticle.hpp/cpp    # 传送门粒子
    │   ├── CritParticle.hpp/cpp      # 暴击粒子
    │   ├── ExplosionParticle.hpp/cpp # 爆炸粒子（含 LargeExplosion）
    │   ├── PoofParticle.hpp/cpp      # 消散粒子
    │   ├── SpellParticle.hpp/cpp     # 药水效果粒子
    │   ├── DragonBreathParticle.hpp/cpp # 龙息粒子（含 EndRod, SweepAttack）
    │   ├── SoulParticle.hpp/cpp      # 灵魂粒子
    │   ├── RedstoneParticle.hpp/cpp  # 红石粉尘粒子
    │   └── CampfireParticle.hpp/cpp  # 营火烟雾粒子
    │
    ├── liquid/                   # 液体粒子
    │   ├── DripParticle.hpp/cpp      # 液体滴落粒子基类
    │   └── DripWaterParticle.hpp/cpp # 水滴粒子
    │
    ├── mob/                      # 生物粒子
    │   ├── HeartParticle.hpp/cpp     # 爱心粒子
    │   └── VillagerParticle.hpp/cpp  # 村民粒子
    │
    └── weather/                  # 天气粒子
        └── SplashParticle.hpp/cpp    # 水溅粒子
```

## 核心类

### Particle（粒子基类）

所有粒子的基类，定义粒子的基本属性和行为。

```cpp
class Particle {
public:
    // 生命周期
    virtual void tick(ClientWorld* world);
    [[nodiscard]] bool isAlive() const;

    // 渲染
    [[nodiscard]] virtual ParticleRenderType getRenderType() const;
    virtual void buildVertices(...);
    [[nodiscard]] virtual ResourceLocation getTextureLocation() const;
    [[nodiscard]] virtual u32 getLightColor(ClientWorld* world) const;

    // 物理
    void move(ClientWorld* world, const glm::vec3& delta);

    // 属性访问器
    const glm::vec3& position() const;
    const glm::vec3& velocity() const;
    f64 age() const;
    f64 maxAge() const;
    // ...
};
```

### ParticleManager（粒子管理器）

管理所有粒子的生命周期、更新和渲染。

主要职责：
- 粒子的创建和销毁
- 按渲染类型分组管理
- GPU 缓冲区管理
- Vulkan 渲染管线

### ParticleRegistry（粒子类型注册表）

单例模式，管理所有粒子类型的注册和创建。

```cpp
// 注册粒子类型
ParticleRegistry::instance().registerType(
    ParticleTypeId::Flame,
    "minecraft:flame",
    FlameParticle::create,
    ParticleRenderType::PARTICLE_SHEET_LIT
);

// 创建粒子实例
auto particle = ParticleRegistry::instance().createParticle(
    ParticleTypeId::Flame,
    glm::vec3(0.0f),
    glm::vec3(0.0f, 0.1f, 0.0f)
);
```

### ParticleTextureAtlas（粒子纹理图集）

管理粒子纹理的加载、打包和查询。

功能：
- 从资源包加载粒子纹理（textures/particle/*.png）
- 支持动画纹理（垂直帧条）
- 纹理打包优化
- GPU 纹理上传

### ParticleRenderType（渲染类型）

决定粒子的渲染方式：

| 类型 | 说明 | 混合 | 深度写入 |
|------|------|------|----------|
| TERRAIN_SHEET | 方块纹理图集 | 否 | 是 |
| PARTICLE_SHEET_OPAQUE | 不透明粒子纹理 | 否 | 是 |
| PARTICLE_SHEET_LIT | 发光粒子纹理 | 是 | 否 |
| PARTICLE_SHEET_TRANSLUCENT | 半透明粒子纹理 | 是 | 否 |
| CUSTOM | 自定义渲染 | - | - |
| NO_RENDER | 不渲染 | - | - |

## 已实现的粒子类型

### 环境粒子（ambient/）
- **BubbleParticle**: 水下气泡，向上漂浮，离开水面消失
- **UnderwaterParticle**: 水下悬浮粒子
- **CloudParticle**: 云朵粒子

### 方块粒子（block/）
- **DiggingParticle**: 挖掘方块产生的粒子

### 效果粒子（effect/）
- **FlameParticle**: 火焰粒子，向上漂浮并缩小
- **SmokeParticle**: 烟雾粒子，向上漂浮
- **LargeSmokeParticle**: 大烟雾粒子
- **LavaParticle**: 熔岩滴粒子
- **PortalParticle**: 传送门粒子
- **CritParticle**: 暴击粒子
- **ExplosionParticle**: 爆炸粒子
- **LargeExplosionParticle**: 大型爆炸粒子，发光，动画纹理
- **PoofParticle**: 消散粒子
- **SpellParticle**: 药水效果粒子
- **DragonBreathParticle**: 龙息粒子
- **EndRodParticle**: 末地烛粒子
- **SweepAttackParticle**: 横扫攻击粒子，发光，动画纹理
- **SoulParticle**: 灵魂粒子
- **RedstoneParticle**: 红石粉尘粒子
- **CampfireParticle**: 营火烟雾粒子（Cozy/Signal 两种类型）

### 液体粒子（liquid/）
- **DripParticle**: 液体滴落粒子基类
  - 状态机：Hanging → Falling → Landed
  - 支持 Water/Lava/Honey/ObsidianTear 类型
- **DripWaterParticle**: 水滴粒子

### 天气粒子（weather/）
- **RainParticle**: 雨滴粒子
  - 快速下落，终端速度 -3.0 blocks/tick
  - 落地时生成 2 个 SplashParticle（50% 概率消失）
  - 碰撞地面时应用地面摩擦 0.7
- **SnowParticle**: 雪花粒子
  - 缓慢飘落，终端速度 -0.5 blocks/tick
  - 左右摇摆效果（正弦波漂移）
  - 落地时立即消失
- **SplashParticle**: 水溅粒子
  - 由 RainParticle 落地时生成
  - 小型向上喷射效果
- **FishingParticle**: 钓鱼涟漪粒子
  - 无重力，漂浮在水面
  - 向下移动形成涟漪效果
  - 用于钓鱼浮标水面效果和鱼接近浮标时的波纹

### 生物粒子（mob/）
- **HeartParticle**: 爱心粒子
- **VillagerParticle**: 村民粒子

### 特殊粒子（special/）
- **NautilusParticle**: 鹦鹉螺粒子
  - 发光粒子，最大亮度
  - 无重力，无物理碰撞
  - 向目标方向移动，速度逐渐衰减
  - 用于潮涌核心效果：从框架方块飞向中心，攻击目标时在目标位置生成

## 与 MC 1.16.5 的对齐

### 物理参数
- 重力乘数：0.04（`PARTICLE_GRAVITY_MULTIPLIER`）
- 空气摩擦：0.98
- 地面摩擦：0.7

### DripParticle 状态机
```
Hanging (悬挂) → Falling (下落) → Landed (落地)
```
- Hanging: 重力 0.02，缓慢积累进度
- Falling: 重力根据类型（水 0.06，蜂蜜 0.01）
- Landed: 存在 16 tick

### 发光粒子
以下粒子返回固定高亮度 15728880：
- FlameParticle
- LavaParticle
- LargeExplosionParticle
- SweepAttackParticle
- SoulFireFlame
- EndRodParticle
- RedstoneParticle

## 创建自定义粒子

1. 创建粒子类，继承自 `Particle`：

```cpp
class MyParticle : public Particle {
public:
    MyParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;
    ParticleRenderType getRenderType() const override;
    ResourceLocation getTextureLocation() const override;
};
```

2. 注册粒子类型：

```cpp
ParticleRegistry::instance().registerType(
    ParticleTypeId::Custom,
    "minecraft:my_particle",
    MyParticle::create,
    ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT
);
```

3. 添加粒子纹理到资源包：

```
textures/particle/my_particle.png
```

## 纹理格式

粒子纹理存储在资源包的 `textures/particle/` 目录下。

- 支持标准 PNG 格式
- 动画纹理使用垂直帧条（帧按垂直方向排列）
- 动画元数据使用 `.png.mcmeta` 文件

动画元数据示例（`flame.png.mcmeta`）：
```json
{
    "animation": {
        "width": 8,
        "height": 8,
        "frametime": 2
    }
}
```

## 性能优化

当前实现：
- 最大粒子数限制：16384
- 按渲染类型分组渲染
- 顶点缓冲区动态更新
- 双缓冲 Uniform 更新

未来优化方向（参考 MadParticle）：
- GPU 实例化渲染
- 多线程并行更新
- 持久化映射缓冲区
- 光照缓存系统

## 参考资料

- Minecraft Java 1.16.5 `net.minecraft.client.particle.*`
- MadParticle 优化模组 `cn.ussshenzhou.madparticle.particle.*`

## 容易踩的坑

1. **纹理坐标**：粒子纹理使用图集，UV 坐标需要从 `ParticleTextureAtlas` 获取
2. **光照采样**：需要从 `ClientWorld` 获取光照值，发光粒子返回固定高亮度
3. **碰撞检测**：粒子碰撞检测需要 `ClientWorld` 参数，传 `nullptr` 则跳过碰撞
4. **生命周期**：`tick()` 方法中需要手动增加 `m_age`，父类不会自动增加
5. **线程安全**：`ParticleRegistry` 是单例，粒子工厂函数不能阻塞
6. **流体检测**：使用 `FluidTags::WATER()` 和 `FluidTags::LAVA()` 而非 `isWaterAt()`
7. **发光粒子亮度**：MC 1.16.5 使用 15728880（blockLight=15, skyLight=15）

## 发射器粒子（Emitter Particles）

发射器粒子是一类特殊的元粒子，它们不直接渲染，而是在生命周期内生成其他粒子。

### EmitterParticle 基类

```cpp
class EmitterParticle : public Particle {
public:
    EmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime);
    EmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount);

    void tick(ClientWorld* world) override;
    ParticleRenderType getRenderType() const override { return ParticleRenderType::NO_RENDER; }

    void emit(ClientWorld* world, ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity);
    void emitWithOffset(...);
    bool shouldEmit() const;

protected:
    u32 m_emitCount = 0;          // 剩余发射次数（0 = 无限）
    u32 m_emitInterval = 1;       // 发射间隔（ticks）
    u32 m_ticksSinceLastEmit = 0; // 上次发射后的 tick 数
};
```

### 已实现的发射器粒子

- **HugeExplosionEmitterParticle**: 在短暂延迟后生成大型爆炸粒子
- **FlameEmitterParticle**: 持续发射火焰粒子
- **SmokeEmitterParticle**: 持续发射烟雾粒子

### 发射回调机制

粒子通过 `ParticleEmitCallback` 回调发射新粒子：

```cpp
using ParticleEmitCallback = std::function<void(ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity)>;

// ParticleManager 在 tick 时设置回调
particle->setEmitCallback([this](ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity) {
    addPendingParticle(type, pos, velocity, nullptr);
});
```

## 粒子同步系统

粒子系统支持客户端-服务端同步，实现多人游戏中的粒子效果同步。

### 网络数据包

`ParticlePacket` 用于服务端向客户端广播粒子生成事件：

```cpp
// 服务端广播粒子
MinecraftServer::broadcastParticleInRange(
    ParticleTypeId::Flame,
    Vector3(100, 64, 200),  // 位置
    Vector3(0, 0.02f, 0),   // 速度
    Vector3(0.5f, 0.5f, 0.5f), // 偏移范围
    10,                     // 数量
    256.0f                  // 广播范围
);

// 客户端回调处理
callbacks.onParticle = [](ParticleTypeId type, f64 x, f64 y, f64 z,
                          f32 vx, f32 vy, f32 vz,
                          f32 ox, f32 oy, f32 oz, u32 count) {
    // 生成粒子
};
```

### 待处理粒子队列

`ParticleManager` 使用待处理队列延迟生成粒子，避免在 tick 中途修改粒子列表：

```cpp
// 添加到待处理队列（线程安全）
void addPendingParticle(ParticleTypeId type,
                       const glm::vec3& pos,
                       const glm::vec3& velocity,
                       ClientWorld* world = nullptr);

// 在 tick 开始时处理待处理队列
void processPendingParticles();
```

### 距离裁剪

粒子支持基于相机位置的距离裁剪，默认最大距离 256 格（与 MC 1.16.5 一致）：

```cpp
// 设置相机位置
particleManager.setCameraPosition(cameraPos);

// 设置最大粒子距离
particleManager.setMaxParticleDistance(256.0f);
```

## 测试覆盖

粒子系统包含完整的单元测试和集成测试：

- `ParticleTest`: 粒子基类测试
- `ParticleRenderTypeTest`: 渲染类型测试
- `ParticleTypesTest`: 粒子类型验证测试
- `ParticleRegistryTest`: 注册表测试
- `ParticleManagerPendingTest`: 待处理队列测试
- `ParticleManagerDistanceCullingTest`: 距离裁剪测试
- `ParticleManagerAliveCountTest`: 存活计数测试
- `ParticlePacketTest`: 粒子数据包测试
- `ParticleSyncIntegrationTest`: 粒子同步集成测试
