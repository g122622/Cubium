# 生物粒子 (Mob Particles)

## 目录结构树

```
mob/
├── HeartParticle.hpp/cpp         # 爱心粒子 - 繁殖/驯服时显示
├── VillagerParticle.hpp/cpp      # 村民粒子 - 愤怒/开心/喷嚏
├── SpitParticle.hpp/cpp          # 羊驼吐沫粒子 - 羊驼攻击时发射，弹道轨迹
├── SquidInkParticle.hpp/cpp      # 鱿鱼墨汁粒子（SquidInk + GlowSquidInk）
├── TotemParticle.hpp/cpp         # 不死图腾粒子 - 金色粒子效果，膨胀淡出
└── README.md
```

## 内部模块关系

```
HeartParticle              → 独立粒子类，无子模块
VillagerParticle.hpp       → 包含 AngryVillagerParticle、HappyVillagerParticle、SneezeParticle
SpitParticle               → 独立粒子类，弹道轨迹（重力 0.03）
SquidInkParticle.hpp       → 包含 SquidInkParticle、GlowSquidInkParticle
TotemParticle              → 独立粒子类，金色粒子效果
```

所有粒子类继承自 `Particle` 基类 (`client/renderer/trident/particle/Particle.hpp`)。

**SpitParticle 特性：**
- 羊驼攻击时发射的吐沫粒子
- 具有弹道轨迹，受重力影响（0.03），碰到地面后减速
- TRANSLUCENT 渲染类型，使用 `spit` 纹理
- 带旋转效果，生命后期淡出

**SquidInkParticle.hpp 包含的粒子类：**
- `SquidInkParticle` - 鱿鱼墨汁粒子（深色墨汁，随机漂移上浮，膨胀淡出，TRANSLUCENT 渲染）
- `GlowSquidInkParticle` - 发光鱿鱼墨汁粒子（明亮青蓝色，行为与 SquidInk 相同但自发光 0xF0，TRANSLUCENT 渲染）

**TotemParticle 特性：**
- 玩家使用不死图腾复活时产生的金色粒子效果
- 随机漂移、向上微浮，随生命周期膨胀并淡出
- TRANSLUCENT 渲染类型，使用 `totem_of_undying` 纹理

## 上下游外部依赖关系

**依赖方（上游）**：
- `Particle` 基类 - 提供生命周期、渲染接口
- `ParticleRegistry` - 粒子类型注册
- `ParticleTextureAtlas` - 纹理图集
- `mc::math::Random` - 随机数生成

**被依赖方（下游）**：
- 实体系统 - 在生物繁殖、驯服、交易等事件中创建粒子
- 鱿鱼/发光鱿鱼 - 受攻击时创建 SquidInkParticle/GlowSquidInkParticle
- 羊驼 - 攻击时创建 SpitParticle
- 不死图腾 - 玩家复活时创建 TotemParticle
- 网络同步 - 通过 `ir::play::LevelParticles` 接收服务端粒子事件

## 容易踩的坑

1. **生命周期管理**：`tick()` 方法中需要手动增加 `m_age`，父类不会自动增加
2. **颜色设置**：使用 `setColor(glm::vec4)` 设置 RGBA，alpha 用于淡出效果
3. **摩擦力系数**：`setFriction()` 影响速度衰减，不同粒子类型系数不同
4. **物理碰撞**：村民粒子通常 `setHasPhysics(false)`，不受方块碰撞影响
5. **getScale() 返回值是乘数而非绝对尺寸**：`getScale()` 返回的值会与 `m_size` 相乘（渲染管线：`halfSize = m_size * scale * 0.5`），因此 getScale() 应仅返回乘数（如 0~1 范围），不要乘以 m_initialSize 或 size()，否则渲染尺寸会被平方放大
