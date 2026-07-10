# 渲染器

本目录包含具体实体类型的渲染器实现，所有渲染器通过 `RendererRegistration.cpp` 统一注册到 `RendererFactory`。

## 目录结构

```
renderer/
├── RendererRegistration.hpp/cpp  # 渲染器注册入口（统一注册所有实体渲染器到工厂）
├── animal/                        # 动物渲染器
│   ├── AnimalRenderers.hpp/cpp    # 猪、牛、羊、哞菇、鸡、兔子、蝙蝠、鱿鱼（简单渲染器）
│   ├── CatRenderer.hpp/cpp        # 猫渲染器（11种皮肤变体）
│   ├── HorseRenderer.hpp/cpp      # 马渲染器（马、驴、骡、骷髅马、僵尸马）
│   ├── LlamaRenderer.hpp/cpp      # 羊驼渲染器（4种颜色变体）
│   ├── OcelotRenderer.hpp/cpp     # 豹猫渲染器
│   ├── VillagerRenderer.hpp/cpp   # 村民渲染器（多层纹理：类型+职业+等级）
│   └── WolfRenderer.hpp/cpp       # 狼渲染器
├── aquatic/                       # 水生生物渲染器
│   └── AquaticRenderers.hpp/cpp   # 河豚、热带鱼、鳕鱼、鲑鱼、海豚、海龟等
├── monster/                       # 怪物渲染器
│   ├── MonsterRenderers.hpp/cpp   # 基础怪物（僵尸、骷髅、苦力怕、蜘蛛、末影人、烈焰人）
│   ├── MonsterVariantRenderers.hpp/cpp  # 变体怪物（僵尸村民、溺尸、尸壳、流浪者、洞穴蜘蛛、巨人）
│   └── SpecialMonsterRenderers.hpp/cpp  # 特殊怪物（凋灵、史莱姆、守卫者、潜影贝、蠹虫、末影螨、灾厄村民等）
├── nether/                        # 下界生物渲染器
│   └── NetherRenderers.hpp/cpp    # 恶魂、岩浆怪、炽足兽、猪灵、猪灵蛮兵、疣猪兽等
├── player/                        # 玩家渲染器
│   └── PlayerRenderer.hpp/cpp     # 玩家渲染器（支持标准/纤细手臂、层渲染器）
├── projectile/                    # 投掷物渲染器
│   ├── BillboardRenderers.hpp/cpp # Billboard渲染器（雪球、鸡蛋、末影珍珠、药水等）
│   ├── ExperienceOrbRenderer.hpp/cpp   # 经验球渲染器
│   ├── FireballRenderers.hpp/cpp  # 火球渲染器（小火球、恶魂火球、龙息）
│   ├── FishingBobberRenderer.hpp/cpp   # 钓鱼浮漂渲染器
│   ├── ItemEntityRenderer.hpp/cpp # 物品实体渲染器
│   └── ProjectileRenderers.hpp/cpp # 箭、三叉戟等投射物
├── special/                       # 特殊实体渲染器
│   ├── SpecialEntityRenderers.hpp/cpp  # 末影水晶、潜影贝子弹、闪电、下落方块、TNT、物品展示框、画、盔甲架、烟花等
│   └── README.md                 # 特殊渲染器实现说明（FallingBlock/TNT 变换链、闪烁公式等）
└── vehicle/                       # 载具渲染器
    └── VehicleRenderers.hpp/cpp   # 船、矿车渲染器
```

## 内部模块关系

```
                        ┌────────────────────┐
                        │ RendererFactory    │ （core/）
                        │ (注册表模式)        │
                        └─────────┬──────────┘
                                  │ 注册
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           RendererRegistration                               │
│  initializeRendererRegistration()                                           │
│    ├── animal/*   (动物渲染器)                                               │
│    ├── aquatic/*  (水生生物渲染器)                                           │
│    ├── monster/*  (怪物渲染器)                                               │
│    ├── nether/*   (下界生物渲染器)                                           │
│    ├── player/*   (玩家渲染器)                                               │
│    ├── projectile/* (投掷物渲染器)                                           │
│    ├── special/*  (特殊实体渲染器)                                           │
│    └── vehicle/*  (载具渲染器)                                               │
└─────────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼ 继承
        ┌─────────────────────────┴─────────────────────────┐
        │                                                   │
        ▼                                                   ▼
┌───────────────────┐                            ┌───────────────────┐
│ LivingRenderer    │ (core/)                     │ EntityRenderer    │ (core/)
│ <TEntity, TModel> │                             │ (基类)            │
└─────────┬─────────┘                            └─────────┬─────────┘
          │ 继承                                           │ 继承
          ▼                                                ▼
  animal/, monster/,                             projectile/*,
  nether/, aquatic/                              vehicle/*, player/
  (大多数生物)                                    (投掷物、载具、玩家)
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `core/EntityRenderer.hpp` | 实体渲染器基类 |
| `core/LivingRenderer.hpp` | 生物渲染器基类（动画参数计算、层渲染器管理） |
| `core/RendererFactory.hpp` | 渲染器工厂（注册表模式） |
| `core/AnimationContext.hpp` | 动画上下文（limbSwing、headYaw等参数） |
| `model/animal/*` | 动物模型 |
| `model/monster/*` | 怪物模型 |
| `model/player/*` | 玩家模型 |
| `model/projectile/*` | 投掷物模型 |
| `layer/core/LayerRenderer.hpp` | 层渲染器基类 |
| `layer/equipment/*` | 装备层渲染器（手持物品、头盔、盔甲） |
| `layer/cosmetic/*` | 外观层渲染器（披风、鞘翅） |
| `layer/entity/*` | 实体特性层渲染器（羊毛、狼项圈等） |
| `common/entity/core/EntityRegistry.hpp` | 实体类型常量（ET::PIG 等） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `core/EntityRendererManager.cpp` | 通过 RendererFactory 创建渲染器实例 |
| `ClientApplication` | 启动时调用 `initializeRendererRegistration()` |

## 容易踩的坑

### 1. 渲染器注册必须在启动时完成

`initializeRendererRegistration()` 必须在 `RendererFactory` 初始化后、任何实体渲染前调用。如果渲染器未注册，`RendererFactory::createRenderer()` 会返回 `nullptr`，导致实体不渲染。

### 2. LivingRenderer 模板参数约束

`LivingRenderer<TEntity, TModel>` 的 `TEntity` 必须继承自 `LivingEntity`，`TModel` 必须继承自 `EntityModel`。如果实体不继承 `LivingEntity`（如 `Player`），应直接继承 `EntityRenderer` 并实现 `IEntityRenderer` 接口。

### 3. 阴影大小设置

不同实体的阴影大小不同，需要在渲染器构造函数中设置 `m_shadowSize`。例如：鸡/兔子/蝙蝠为 0.3，猪/牛/羊/马/鱿鱼为 0.7，蜘蛛 0.7，史莱姆 0.25，潜影贝/蠹虫/末影螨为 0。

### 4. 层渲染器添加顺序

层渲染器的渲染顺序就是 `addLayer()` 的调用顺序。某些层（如 `EnergyGlintLayer`）需要在其他层之后渲染才能正确显示。

### 5. PlayerRenderer 不继承 LivingRenderer

`Player` 类不继承 `LivingEntity`，因此 `PlayerRenderer` 直接继承 `EntityRenderer` 并手动实现 `IEntityRenderer` 接口和层渲染器支持。

### 6. 渲染器复用同一模型但不同纹理

部分渲染器可复用同一模型类但通过不同的纹理路径区分。例如 `WitherSkeletonRenderer` 复用 `SkeletonModel`，`MooshroomRenderer` 复用 `CowModel`。

### 7. AnimalRenderers.hpp 中的简单渲染器是内联实现

`AnimalRenderers.hpp` 中的简单渲染器（猪、牛、羊等）的 `getEntityTexture()` 直接在类定义中实现，不需要额外 cpp 文件。
