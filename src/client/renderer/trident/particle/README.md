# 粒子系统 (Particle System)

## 目录结构

```
particle/
├── Particle.hpp/cpp              # 粒子基类，定义生命周期、物理、渲染接口
├── ParticleManager.hpp/cpp       # 粒子管理器，负责生命周期、GPU 缓冲区、渲染
├── ParticleRegistry.hpp/cpp      # 粒子类型注册表（单例）
├── ParticleRenderType.hpp        # 渲染类型枚举（TERRAIN_SHEET, PARTICLE_SHEET_LIT 等）
├── ParticleTextureAtlas.hpp/cpp  # 粒子纹理图集，支持动画纹理
├── ParticleTypes.hpp             # 粒子类型 ID 枚举
├── ParticleFactories.hpp/cpp     # 粒子工厂函数集合
│
├── data/                         # 粒子参数数据
│   ├── ParticleData.hpp          # 粒子参数基类
│   ├── BasicParticleData.hpp/cpp # 无参数粒子数据
│   ├── BlockParticleData.hpp/cpp # 方块粒子数据（携带 BlockState）
│   ├── ItemParticleData.hpp/cpp  # 物品粒子数据（携带 ItemStack）
│   └── RedstoneParticleData.hpp/cpp # 红石粒子数据（携带信号强度）
│
├── sprite/                       # 精灵动画系统
│   ├── ISprite.hpp               # 精灵接口
│   ├── SimpleSprite.hpp/cpp      # 简单静态精灵
│   └── AnimatedSprite.hpp/cpp    # 动画精灵（帧动画）
│
└── particles/                    # 具体粒子实现
    ├── RainParticle.hpp/cpp      # 雨滴粒子
    ├── SnowParticle.hpp/cpp      # 雪花粒子
    │
    ├── ambient/                  # 环境粒子
    │   ├── BubbleParticle.hpp/cpp     # 水下气泡
    │   ├── BubblePopParticle.hpp/cpp  # 气泡破裂（BubbleParticle 离开水面时生成）
    │   ├── UnderwaterParticle.hpp/cpp # 水下悬浮
    │   ├── CloudParticle.hpp/cpp      # 云朵粒子
    │   └── SporeBlossomParticle.hpp/cpp # 孢子花粒子
    │
    ├── block/                    # 方块粒子
    │   ├── DiggingParticle.hpp/cpp    # 挖掘粒子（使用方块纹理）
    │   ├── DustPillarParticle.hpp/cpp # 尘柱粒子（重锤砸地攻击，继承DiggingParticle）
    │   └── ItemParticle.hpp/cpp       # 物品粒子（Item/ItemSlime/ItemCobweb/ItemSnowball 共用，双路径纹理解析）
    │
    ├── effect/                   # 特效粒子
    │   ├── FlameParticle.hpp/cpp      # 火焰粒子
    │   ├── SmokeParticle.hpp/cpp      # 烟雾粒子
    │   ├── LavaParticle.hpp/cpp       # 熔岩滴粒子
    │   ├── PortalParticle.hpp/cpp     # 传送门粒子
    │   ├── CritParticle.hpp/cpp       # 暴击粒子 + 附魔暴击粒子（EnchantedHitParticle）
    │   ├── ExplosionParticle.hpp/cpp  # 爆炸粒子
    │   ├── PoofParticle.hpp/cpp       # 消散粒子
    │   ├── SpellParticle.hpp/cpp      # 药水效果粒子
    │   ├── DragonBreathParticle.hpp/cpp # 龙息粒子
    │   ├── EmitterParticle.hpp/cpp    # 发射器粒子基类
    │   ├── SoulParticle.hpp/cpp       # 灵魂粒子
    │   ├── WhiteSmokeParticle.hpp/cpp # 白色烟雾粒子（方向性发射、DISPENSER_SMOKE/SHOOT_WHITE_SMOKE）
    │   ├── RedstoneParticle.hpp/cpp   # 红石粉尘粒子
    │   └── CampfireParticle.hpp/cpp   # 营火烟雾粒子
    │
    ├── liquid/                   # 液体粒子
    │   ├── DripParticle.hpp/cpp       # 液体滴落基类（状态机：Hanging → Falling → Landed）
    │   └── DripWaterParticle.hpp/cpp  # 水滴粒子
    │
    ├── mob/                      # 生物粒子
    │   ├── HeartParticle.hpp/cpp      # 爱心粒子
    │   └── VillagerParticle.hpp/cpp   # 村民粒子
    │
    ├── special/                  # 特殊粒子
    │   └── NautilusParticle.hpp/cpp   # 鹦鹉螺粒子（发光、无重力）
    │
    └── weather/                  # 天气粒子
        ├── SplashParticle.hpp/cpp     # 水溅粒子
        └── FishingParticle.hpp/cpp    # 钓鱼涟漪粒子
```

## 内部模块关系

```
ParticleManager
    ├── ParticleRegistry          # 查询粒子类型元数据
    ├── ParticleTextureAtlas      # 纹理图集（UV 坐标）
    ├── Particle（基类）
    │       ├── 粒子数据：data/ParticleData 子类
    │       └── 精灵动画：sprite/ISprite 接口
    └── particles/（具体粒子实现）
            └── 继承 Particle，实现 tick()、getRenderType()、getTextureLocation()
```

粒子管理器（`ParticleManager`）持有所有粒子实例，按渲染类型分组渲染。粒子注册表（`ParticleRegistry`）提供粒子类型的工厂方法。纹理图集（`ParticleTextureAtlas`）从资源包加载纹理并提供 UV 坐标。

## 上下游外部依赖关系

### 上游依赖（粒子系统依赖的模块）

| 依赖模块 | 用途 |
|---------|------|
| `common/core` | 基础类型、Result |
| `common/util/math` | 向量、随机数 |
| `common/util/AxisAlignedBB` | 碰撞检测 |
| `common/resource` | ResourceLocation、IResourcePack、BlockModelCache、ItemModelCache、ItemTextureAtlas |
| `common/world` | BlockState、FluidTags |
| `common/item` | ItemStack（ItemParticle）、BlockItemRegistry（方块物品判断与 Block 解析） |
| `client/world/ClientWorld` | 光照采样、碰撞检测、流体状态 |
| `client/renderer/trident/core` | Vulkan 设备、命令池、渲染通道 |
| `client/renderer/trident/chunk` | ChunkMesher::modelCache() 获取 BlockModelCache（方块/方块物品粒子纹理） |
| `client/network` | ParticlePacket 粒子同步（含 createItem / createBlock / createEntityEffect） |

### 下游依赖（使用粒子系统的模块）

| 使用模块 | 用途 |
|---------|------|
| `TridentEngine` | 持有 ParticleManager，每帧调用 tick/render；注入 `ItemTextureAtlas` 给 `ItemParticle` |
| `WeatherRenderer` | 生成雨滴/雪花粒子 |
| `ClientWorld` | 世界事件触发粒子（方块破坏、实体效果、物品破碎等） |
| `ClientApplicationNetwork` | 处理服务端粒子同步包（Block/Item/EntityEffect/Vibration 等），通过 `ParticleData` 走数据管线 |

## 容易踩的坑

1. **纹理坐标**：粒子纹理使用图集，UV 坐标必须从 `ParticleTextureAtlas` 获取，不能硬编码。

2. **光照采样**：需要从 `ClientWorld` 获取光照值。发光粒子（Flame、Lava、EndRod、Redstone 等）返回固定亮度 15728880（blockLight=15, skyLight=15）。

3. **碰撞检测**：`move()` 方法需要 `ClientWorld` 参数，传 `nullptr` 则跳过碰撞检测。

4. **生命周期**：子类 `tick()` 方法中需要手动增加 `m_age`，父类不会自动增加。

5. **线程安全**：`ParticleRegistry` 是单例，粒子工厂函数不能阻塞。

6. **流体检测**：使用 `FluidTags::WATER()` 和 `FluidTags::LAVA()` 而非 `isWaterAt()`。

7. **距离裁剪**：默认最大粒子距离 256 格，通过 `setMaxParticleDistance()` 调整。

8. **待处理队列**：使用 `addPendingParticle()` 添加粒子，粒子会在下一帧开始时处理，避免在 tick 中途修改粒子列表。

9. **粒子质量过滤**：`ParticleManager` 根据 `ParticleMode` 设置过滤粒子生成和渲染：
   - `All`：显示所有粒子
   - `Decreased`：约 2/3 的普通粒子通过（每帧 1/3 概率降级为 Minimal 行为），重要粒子始终显示
   - `Minimal`：仅显示 `overrideLimiter=true` 的重要粒子，ambient 粒子被完全跳过
   - 过滤在 `addPendingParticle()`、`ClientWorld::addParticle()` 和 `ClientWorld::addBlockParticle()` 入口处执行
   - 通过 `shouldShowParticle(type)` 方法统一判断，使用 `ParticleTypeInfo::overrideLimiter` 标志区分重要/普通粒子

9. **物理参数**：重力乘数 0.04、空气摩擦 0.98、地面摩擦 0.7，与 MC 1.16.5 一致。

10. **DripParticle 状态机**：Hanging → Falling → Landed 三态，各态物理参数不同。

11. **getScale() 返回值是乘数**：`getScale()` 返回的值会与 `m_size` 相乘（渲染管线：`halfSize = m_size * scale * 0.5`），因此 getScale() 应仅返回缩放乘数（如 0~1 范围），**不要**乘以 `m_initialSize` 或 `size()`，否则渲染尺寸会被平方放大。如需基于初始大小做动画，应在 `tick()` 中用 `setSize(m_initialSize * factor)` 更新 m_size，让 getScale() 返回 1.0；或让 getScale() 仅返回乘数。

12. **粒子随机源**：`Particle` 基类提供 `m_random` 成员（`mc::math::Random` 类型），构造时使用位置哈希和时间戳作为种子。子类应直接使用 `m_random` 获取随机数，**不要**在 tick() 或构造函数中创建局部 `mc::math::Random` 对象，否则每个粒子实例的随机序列会完全相同（默认种子为 0），且每 tick 创建对象有性能开销。

13. **TERRAIN_SHEET 粒子的纹理图集限制（TODO）**：`ParticleManager` 当前在渲染所有粒子时仅绑定单一的 `ParticleTextureAtlas` 纹理（描述符集 binding 1），不支持按 `ParticleRenderType` 切换纹理图集。`DiggingParticle`、`DustPillarParticle`、`ItemParticle` 等 `TERRAIN_SHEET` 粒子预计算的方块图集 / 物品图集 UV 坐标在渲染时会采样错误纹理。`ItemParticle` 的非方块物品路径通过 `setItemTextureAtlas()` 静态注入 `ItemTextureAtlas`，但该图集的 UV 仅用于 `buildVertices()` 预计算，渲染采样器仍指向 `ParticleTextureAtlas`。完整修复需要 `ParticleManager` 按 `ParticleRenderType` 维护多套纹理图集描述符并按渲染批次绑定。
