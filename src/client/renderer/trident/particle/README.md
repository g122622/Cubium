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
    ├── ambient/                  # 环境粒子（待实现）
    ├── block/                    # 方块粒子（待实现）
    ├── effect/                   # 特效粒子
    │   ├── FlameParticle.hpp/cpp # 火焰粒子
    │   ├── SmokeParticle.hpp/cpp # 烟雾粒子
    │   ├── LavaParticle.hpp/cpp  # 熔岩滴粒子
    │   └── PortalParticle.hpp/cpp# 传送门粒子
    ├── liquid/                   # 液体粒子
    │   └── DripParticle.hpp/cpp  # 液体滴落粒子基类
    ├── mob/                      # 生物粒子
    │   └── HeartParticle.hpp/cpp # 爱心粒子
    └── weather/                  # 天气粒子（待实现）
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

## 粒子类型

参考 MC 1.16.5 ParticleTypes，支持以下分类：

- **环境类**：气泡、水下悬浮、传送门等
- **方块/物品类**：破坏粒子、挖掘粒子、下落灰尘等
- **效果类**：火焰、烟雾、熔岩、爆炸、暴击、药水效果等
- **液体滴落类**：水滴、熔岩滴、蜂蜜滴等
- **天气类**：雨滴、雪花、溅射等
- **生物相关**：爱心、愤怒村民、开心村民等

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
