# World Module

世界模块是 Cubium 世界模拟的核心，提供地形生成、方块/流体管理、光照系统、区块处理和世界状态管理。实现兼容 MC 1.16.5 的世界机制。

## 目录结构

```
world/
├── IWorld.hpp/cpp              # 世界访问接口
├── IWorldWriter.hpp            # 世界写入接口（生成用）
├── WorldConstants.hpp          # 世界常量和坐标转换工具
├── WorldConfig.hpp             # 世界配置
├── WorldEvents.hpp             # 世界事件ID常量
├── GlobalPos.hpp               # 全局位置类型
├── biome/                      # 生物群系系统
│   ├── Biome.hpp/cpp           # 生物群系定义（170个群系）
│   ├── BiomeSource.hpp/cpp     # 生物群系源基类
│   ├── BiomeGenerationSettings.hpp/cpp  # 群系生成配置
│   ├── BiomeRegistry.hpp/cpp   # 群系注册表
│   ├── Biomes.hpp              # 群系ID常量
│   ├── BiomeEffects.hpp/cpp    # 群系视觉效果
│   ├── BiomeAmbientSounds.hpp  # 群系环境音效
│   ├── climate/                # 气候系统
│   └── source/                 # 群系源实现
├── block/                      # 方块系统
│   ├── Block.hpp/cpp           # 方块基类
│   ├── BlockPos.hpp            # 方块位置类型
│   ├── BlockRegistry.hpp/cpp   # 方块注册表
│   ├── Material.hpp/cpp        # 方块材质
│   ├── HarvestTool.hpp         # 工具类型定义
│   ├── ILiquidContainer.hpp    # 液体容器接口
│   ├── VanillaBlocks.hpp/cpp   # 原版方块定义
│   ├── blocks/                 # 具体方块类型（按功能分类）
│   │   ├── agricultural/       # 农作物类
│   │   ├── building/           # 建筑方块
│   │   ├── cave/               # 洞穴相关
│   │   ├── copper/             # 铜相关
│   │   ├── coral/              # 珊瑚类
│   │   ├── decorative/         # 装饰方块
│   │   ├── dirt/               # 泥土类
│   │   ├── end/                # 末地相关
│   │   ├── functional/         # 功能方块
│   │   ├── ice/                # 冰类
│   │   ├── mangrove/           # 红树林
│   │   ├── mob/                # 生物相关
│   │   ├── nether/             # 下界相关
│   │   ├── ocean/              # 海洋相关
│   │   ├── pale_garden/        # 苍白花园
│   │   ├── redstone/           # 红石方块
│   │   ├── sculk/              # 幽匿块
│   │   ├── special/            # 特殊方块
│   │   ├── trial/              # 试炼相关
│   │   └── vegetation/         # 植被类
│   ├── dispense/               # 发射器行为
│   └── registry/               # 方块注册辅助
├── blockentity/                # 方块实体
│   ├── core/                   # 核心方块实体
│   ├── interactive/            # 交互类（告示牌、床等）
│   ├── processing/             # 处理类（高炉、烟熏炉等）
│   ├── redstone/               # 红石类
│   ├── storage/                # 存储类（箱子、漏斗等）
│   ├── transport/              # 运输类
│   └── trial/                  # 试炼类
├── border/                     # 世界边界系统
│   └── WorldBorder.hpp/cpp     # 边界管理（伤害、渐变动画）
├── chunk/                      # 区块管理
│   ├── ChunkData.hpp/cpp       # 区块数据存储
│   ├── ChunkPos.hpp            # 区块位置类型
│   ├── ChunkPrimer.hpp/cpp     # 生成中间态区块
│   ├── ChunkStatus.hpp/cpp     # 区块生成阶段
│   ├── IChunk.hpp/cpp          # 区块接口
│   ├── ChunkDistanceGraph.hpp/cpp    # BFS距离计算
│   ├── ChunkLoadTicket.hpp     # 加载票据类型
│   ├── ChunkLoadTicketManager.hpp/cpp # 多源票据聚合
│   └── SingleChunkLifecycleManager.hpp/cpp # 单区块生命周期
├── dimension/                  # 维度系统
│   ├── DimensionRenderSettings.hpp
│   └── teleport/               # 维度传送
├── entity/                     # 世界实体管理
│   └── EntityManager.hpp/cpp   # 实体生命周期和查询
├── explosion/                  # 爆炸系统
│   ├── Explosion.hpp/cpp       # 爆炸核心逻辑
│   ├── ExplosionContext.hpp/cpp # 爆炸上下文
│   └── ExplosionMode.hpp       # 爆炸模式枚举
├── fluid/                      # 流体系统
│   ├── Fluid.hpp/cpp           # 流体基类
│   ├── FlowingFluid.hpp/cpp    # 流动流体力学
│   ├── FluidRegistry.hpp/cpp   # 流体注册表
│   ├── FluidTags.hpp/cpp       # 流体标签
│   ├── FLUID_TODO.md           # 流体系统TODO
│   └── fluids/                 # 具体流体（水、岩浆等）
├── gamerule/                   # 游戏规则
│   ├── GameRule.hpp/cpp        # 单个规则定义
│   └── GameRules.hpp/cpp       # 规则集合管理
├── gen/                        # 世界生成
│   ├── aquifer/                # 含水层生成
│   ├── carver/                 # 洞穴/峡谷雕刻
│   ├── chunk/                  # 区块生成器
│   ├── density/                # 密度函数
│   ├── feature/                # 世界特性（树、矿等）
│   │   ├── cave/               # 洞穴特性
│   │   ├── fungus/             # 菌类
│   │   ├── gateway/            # 末地折跃门
│   │   ├── lake/               # 湖
│   │   ├── nether/             # 下界特性
│   │   ├── ocean/              # 海洋特性
│   │   ├── ore/                # 矿石
│   │   ├── predicate/          # 放置条件
│   │   ├── spike/              # 尖刺（冰刺等）
│   │   ├── state/              # 状态特性
│   │   ├── template/           # 模板特性
│   │   ├── tree/               # 树生成
│   │   └── vegetation/         # 植被
│   ├── jigsaw/                 # 拼图结构组装
│   ├── noise/                  # 噪声生成器
│   ├── placement/              # 特性放置
│   ├── settings/               # 生成设置
│   ├── spawn/                  # 世界出生点
│   ├── structure/              # 结构生成
│   │   ├── pools/              # 结构池
│   │   └── structures/         # 具体结构
│   ├── surface/                # 地表构建器
│   └── valueprovider/          # 值提供器
├── lighting/                   # 光照系统
│   ├── IChunkLightProvider.hpp # 区块光照提供接口
│   ├── LightType.hpp           # 光照类型枚举
│   ├── InternalLightUtils.hpp/cpp # 内部光照工具
│   ├── engine/                 # 光照引擎
│   ├── manager/                # 光照管理器
│   └── storage/                # 光照存储
├── map/                        # 地图系统
│   ├── MapData.hpp/cpp         # 地图数据
│   ├── MapDecoration.hpp/cpp   # 地图装饰
│   ├── MapBanner.hpp/cpp       # 地图横幅标记
│   ├── MapFrame.hpp/cpp        # 地图物品展示框
│   ├── MapIdTracker.hpp/cpp    # 地图ID追踪
│   ├── MapDataManager.hpp/cpp  # 地图数据管理
│   └── MaterialColor.hpp/cpp   # 材质颜色
├── redstone/                   # 红石系统
│   ├── RedstoneSystem.hpp/cpp  # 红石系统管理
│   ├── RedstonePower.hpp/cpp   # 信号强度计算
│   ├── RedstoneContext.hpp/cpp # 递归保护
│   └── RedstoneHelper.hpp/cpp  # 辅助函数
├── spawn/                      # 生物生成信息
│   └── MobSpawnInfo.hpp/cpp
├── storage/                    # 世界持久化
│   ├── backend/                # 存储后端
│   ├── blockentity/            # 方块实体存储
│   ├── core/                   # 核心存储接口
│   ├── db/                     # 数据库
│   ├── entity/                 # 实体存储
│   ├── list/                   # 世界列表
│   ├── player/                 # 玩家数据存储
│   ├── reader/                 # 存档读取器
│   │   ├── bedrock/            # 基岩版格式
│   │   └── java/               # Java版格式
│   ├── request/                # 世界操作请求
│   ├── save/                   # 存档保存
│   ├── section/                # Section存储
│   ├── snapshot/               # 世界快照
│   └── task/                   # 存储任务
├── tick/                       # Tick调度
│   ├── base/                   # 基础类型
│   ├── list/                   # Tick列表
│   └── manager/                # Tick管理器
├── time/                       # 游戏时间
│   └── GameTime.hpp/cpp        # 昼夜循环
├── village/                    # 村庄系统
│   ├── Village.hpp/cpp         # 村庄核心
│   ├── VillageManager.hpp/cpp  # 村庄管理
│   ├── VillageGossip.hpp/cpp   # 村庄流言
│   ├── VillageGossipType.hpp/cpp
│   ├── poi/                    # 兴趣点
│   ├── raid/                   # 袭击
│   ├── trade/                  # 交易
│   └── README.md
└── weather/                    # 天气系统
    ├── WeatherConstants.hpp    # 天气常量
    ├── WeatherState.hpp        # 天气状态
    └── WeatherUtils.hpp/cpp    # 天气工具
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │     IWorld      │
                    │   (接口层)       │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   ChunkData     │ │   Block/Fluid   │ │   Lighting      │
│   ChunkManager  │ │   Registry      │ │   System        │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
         └───────────────────┼───────────────────┘
                             │
                    ┌────────┴────────┐
                    │   Generation    │
                    │ (Noise, Biome,  │
                    │  Feature, etc)  │
                    └─────────────────┘
```

**核心数据流**：
- `IWorld` 作为统一访问接口，被 `ServerWorld`/`ClientWorld` 实现
- 区块通过 `ChunkLoadTicketManager` → `SingleChunkLifecycleManager` → `ChunkPrimer` → `ChunkData` 流转
- 生成器链：`NoiseChunkGenerator` 调用 `BiomeSource` → 噪声 → 地表 → 雕刻器 → 特性
- 光照由 `WorldLightManager` 协调 `SkyLightEngine`/`BlockLightEngine`，使用 `SWMRNibbleArray` 存储

## 上下游外部依赖关系

### 被依赖方（谁依赖了这个模块）
- `src/server/world/` - 服务端世界实现（ServerWorld、区块加载、玩家管理）
- `src/client/world/` - 客户端世界实现（ClientWorld、渲染数据）
- `src/server/network/` - 数据包序列化（区块数据、方块更新）
- `src/client/renderer/` - 渲染数据消费（区块网格、光照）

### 依赖方（这个模块依赖谁）
- `common/core/` - 基础类型（i8/u8/i32/u64/f32/f64、Result、Constants）
- `common/util/math/` - 数学工具（向量、噪声、坐标转换）
- `common/util/nbt/` - NBT序列化
- `common/util/property/` - 方块状态属性系统
- `common/physics/collision/` - 碰撞形状
- `common/entity/` - 实体基类
- `common/resource/` - 资源定位
- `glm` - 数学库
- `spdlog` - 日志

## 容易踩的坑

### 1. 区块生成顺序
**问题**：在区块达到 FULL 状态前访问方块数据会崩溃。
**解决**：检查区块状态或使用 future 等待。

### 2. 坐标转换负数处理
**问题**：`x / 16` 对负数会向下取整，导致错误。
**解决**：使用 `WorldConstants.hpp` 中的 `toChunkCoord()`、`toLocalCoord()`。

### 3. 生物群系采样分辨率
**问题**：生物群系以 4x4x4 块存储，逐块查询会出错。
**解决**：使用 `BiomeContainer` 进行正确的 4x4x4 采样。

### 4. 流体级别方向
**问题**：流体级别向源头增加而非减少。
**解决**：级别 8 = 源头，级别 1 = 最远离源头。

### 5. 光照线程安全
**问题**：光照引擎更新时读取光照数据会竞争。
**解决**：使用 `SWMRNibbleArray` 的线程安全访问器或同步访问。

### 6. BlockState 指针有效性
**问题**：存储 BlockState 指针超出注册表生命周期。
**解决**：BlockState 指针在程序整个生命周期有效（注册表永不卸载）。

### 7. IWorld BlockPos 重载
**问题**：非 `IWorld` 接口强制添加 `BlockPos` 重载会导致接口膨胀。
**解决**：`IWorld` 已提供 `BlockPos` 重载。`ISpawnWorldReader`、`ClientWorld`、光照/生成辅助类保持原生 xyz 签名，不要强制添加 `BlockPos` 重载。

### 8. ServerWorld setBlockState 测试
**问题**：测试 `ServerWorld::setBlockState()` 时，未初始化的世界会触发光照更新断言。
**解决**：测试前先初始化世界，否则 `m_lightManager` 为 null 会触发断言。

### 9. BlockUpdatePacket 发送
**问题**：直接从服务器代码发送 `BlockUpdatePacket` 会绕过去重和批处理。
**解决**：通过 `ServerWorld::setOnBlockChanged()` 供给 `BlockUpdateSyncManager`，保持去重和 tick 结束刷新的集中化。

### 10. Tick 优先级溢出
**问题**：过多调度的 tick 导致性能问题。
**解决**：`TickManager` 每个 tick 有处理上限（65536），使用优先级处理关键更新。

### 11. 高度常量使用
**问题**：硬编码 0、256、16 等高度/尺寸数字。
**解决**：使用 `mc::world` 命名空间下的常量（`MIN_BUILD_HEIGHT`、`MAX_BUILD_HEIGHT`、`CHUNK_WIDTH` 等）。`CHUNK_HEIGHT` 与 `MAX_BUILD_HEIGHT` 目前值相同但语义不同，未来可能变化。

### 12. canSeeSky 实现
**问题**：错误假设 `canSeeSky()` 只检查顶部是否有方块。
**解决**：实际基于天空光照 >= 15 判断，正确处理透明方块（玻璃、水）和部分方块（台阶、楼梯）。
