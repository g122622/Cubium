# 实体渲染系统

实体渲染系统负责渲染游戏中的所有实体，包括玩家、生物、怪物、物品实体等。

## 目录结构

```
entity/
├── core/                           # 核心渲染器
│   ├── AnimatedMeshCache.hpp/cpp   # 动画网格缓存（避免每帧重建网格）
│   ├── AnimationContext.hpp/cpp    # 动画上下文（limbSwing、headYaw等参数）
│   ├── EntityRenderer.hpp/cpp      # 实体渲染器基类
│   ├── EntityRendererManager.hpp/cpp # 渲染器管理器（渲染器注册、网格缓存、渲染分发）
│   ├── IEntityRenderer.hpp         # 实体渲染器接口（模板：将渲染器与模型类型解耦）
│   ├── LivingRenderer.hpp          # 生物渲染器模板（支持层渲染器系统）
│   ├── README.md                   # 模块文档
│   ├── RendererFactory.hpp/cpp     # 渲染器工厂（注册表模式，替代巨型 if-else）
├── pipeline/                       # 渲染管线
│   ├── EntityPipeline.hpp/cpp      # Vulkan 渲染管线（管理管线状态、缓冲区、描述符集）
│   └── EntityTextureAtlas.hpp/cpp  # 实体纹理图集（UV 映射、多路径格式支持）
├── util/                           # 工具类
│   ├── NameTagRenderer.hpp/cpp     # 名称标签渲染器
│   ├── ShadowRenderer.hpp/cpp      # 阴影渲染器
│   └── WorldTextRenderer.hpp/cpp   # 世界空间文本渲染器
├── renderer/                       # 具体渲染器（按实体类型分类）
│   ├── animal/                     # 动物渲染器（猪、牛、羊、鸡、狼、猫等）
│   ├── aquatic/                    # 水生生物渲染器（河豚、鱿鱼等）
│   ├── monster/                    # 怪物渲染器（僵尸、骷髅、苦力怕等）
│   ├── nether/                     # 下界生物渲染器
│   ├── player/                     # 玩家渲染器
│   ├── projectile/                 # 投掷物渲染器（雪球、鸡蛋、末影珍珠、药水等）
│   ├── special/                    # 特殊实体渲染器
│   ├── vehicle/                    # 载具渲染器（船、矿车等）
│   ├── RendererRegistration.hpp/cpp # 渲染器注册入口
├── model/                          # 模型系统
│   ├── core/                       # 模型核心
│   │   ├── AgeableModel.hpp/cpp    # 可成长模型基类（幼体/成年状态切换）
│   │   ├── EntityModel.hpp/cpp     # 模型基类（定义动画和渲染接口）
│   │   ├── ModelFactory.hpp/cpp    # 模型工厂（注册表模式）
│   │   ├── ModelRenderer.hpp/cpp   # 模型部件渲染（头、身体、腿等）
│   │   └── SegmentedModel.hpp/cpp  # 分段模型基类
│   ├── base/                       # 基础模型
│   │   ├── BipedModel.hpp/cpp      # 双足模型（玩家、僵尸、骷髅）
│   │   └── QuadrupedModel.hpp/cpp  # 四足模型（猪、牛、羊）
│   ├── player/                     # 玩家模型
│   ├── animal/                     # 动物模型
│   ├── aquatic/                    # 水生生物模型
│   ├── monster/                    # 怪物模型
│   ├── nether/                     # 下界生物模型
│   ├── projectile/                 # 投掷物模型
│   ├── ModelRegistration.hpp/cpp   # 模型注册入口
├── layer/                          # 层渲染器系统
│   ├── core/                       # 层渲染器核心
│   │   └── LayerRenderer.hpp       # 层渲染器基类模板
│   ├── equipment/                  # 装备层（手持物品、头盔、盔甲）
│   ├── cosmetic/                   # 外观层（披风、鞘翅）
│   ├── entity/                     # 实体特性层（鞍、羊毛、狼项圈、箭）
│   └── effect/                     # 效果层（附魔光效、眼睛发光）
└── effect/                         # 特效系统
    ├── glow/                       # 发光效果
    ├── fire/                       # 着火效果
    └── hurt/                       # 受伤闪烁效果
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────┐
│                        EntityRendererManager                         │
│  ┌────────────────┐  ┌──────────────────┐  ┌───────────────────┐   │
│  │ RendererFactory│  │ AnimatedMeshCache│  │ EntityPipeline    │   │
│  │ (创建渲染器)    │  │ (动画网格缓存)    │  │ (Vulkan管线)      │   │
│  └────────┬───────┘  └──────────────────┘  └───────────────────┘   │
│           │                                                          │
│           ▼                                                          │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                    EntityRenderer (基类)                    │    │
│  │  ┌──────────────────────────────────────────────────────┐  │    │
│  │  │ LivingRenderer<TEntity, TModel> (生物渲染器模板)      │  │    │
│  │  │   ├── IEntityRenderer 接口                            │  │    │
│  │  │   ├── TModel 模型实例                                 │  │    │
│  │  │   └── LayerRenderer[] 层渲染器列表                    │  │    │
│  │  └──────────────────────────────────────────────────────┘  │    │
│  │  ┌──────────────────────────────────────────────────────┐  │    │
│  │  │ PipelineMeshProvider (接口，投掷物等自定义网格)       │  │    │
│  │  └──────────────────────────────────────────────────────┘  │    │
│  └────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
           │                              │
           ▼                              ▼
┌────────────────────┐      ┌────────────────────────────────┐
│    ModelFactory    │      │      LayerRenderer[]           │
│  ┌──────────────┐  │      │  ├── HeldItemLayer (手持物品)  │
│  │ EntityModel  │◄─┼──────┼──├── HeadLayer (头盔/南瓜)     │
│  │  ├── Ageable │  │      │  ├── ArmorLayer (盔甲)         │
│  │  ├── Biped   │  │      │  ├── CapeLayer (披风)          │
│  │  └── Quadru. │  │      │  └── EnergyGlintLayer (附魔光) │
│  └──────────────┘  │      └────────────────────────────────┘
└────────────────────┘
```

**渲染决策流程：**

```
EntityRendererManager.renderWithPipeline(entity)
    │
    ├── ItemEntity? ─────────────────────────────────► Billboard 渲染
    │
    ├── ModelFactory.hasModel(entityType)?
    │   ├── Yes ──► 创建模型 → 设置动画 → 生成网格 → AnimatedMeshCache 缓存
    │   └── No
    │
    ├── renderer.getPipelineMeshProvider()?
    │   ├── Yes ──► PipelineMeshProvider.generateMesh() 自定义网格
    │   └── No ──► 跳过/错误标记
    │
    └── EntityPipeline.drawMesh() + renderLayersPipeline()
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/entity` | 实体数据结构（Entity、LivingEntity、AgeableEntity 等） |
| `common/resource` | ResourceLocation 纹理路径 |
| `common/core` | 基础类型（Vector3、Result 等） |
| `common/util/math` | 数学工具（矩阵、视锥体剔除） |
| `client/renderer/trident/core` | Vulkan 上下文、管线、纹理 |
| `client/renderer/trident/firstperson` | 第一人称渲染（共享 PlayerModel） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `client/world/ClientWorld` | 渲染世界中的所有实体 |
| `client/ClientApplication` | 初始化实体渲染系统 |

## 容易踩的坑

### 1. 幼体模型的头身分离渲染

AgeableModel 在幼体状态下会将头身分离渲染（头部和身体使用不同的变换矩阵）。如果新增可成长实体，必须正确实现 `getHeadParts()` 和 `getBodyParts()` 方法，否则幼体渲染会出错。

### 2. 动画网格缓存失效

AnimatedMeshCache 使用 AnimationContext 的哈希值判断是否需要重建网格。如果实体的动画状态改变但哈希未更新，会导致渲染不正确。修改动画参数时确保调用 `context.computeHash()`。

### 3. 层渲染器注册顺序

层渲染器的渲染顺序就是 `addLayer()` 的调用顺序。某些层（如 EnergyGlintLayer）需要在其他层之后渲染才能正确显示。

### 4. BlendMode::Lines 的特殊处理

`BlendMode::Lines` 使用 `VK_PRIMITIVE_TOPOLOGY_LINE_LIST` 拓扑，适用于钓鱼线等线段渲染。绑定管线时必须指定正确的 blendMode。

### 5. 河豚模型动态切换

PufferfishRenderer 根据膨胀状态（puffState 0/1/2）动态切换模型。类似需求不要在 `getModel()` 中创建新模型，应该预先创建所有模型实例。

### 6. PipelineMeshProvider 与 ModelFactory 的选择

- 使用 ModelFactory：需要完整骨骼动画的实体（大多数生物）
- 使用 PipelineMeshProvider：自定义几何体（箭、船、矿车、钓鱼浮漂等）

### 7. 纹理图集 UV 重映射

EntityTextureAtlas 支持 MC 1.12 和 1.13+ 两种路径格式。新增实体纹理时确保路径格式正确，否则 UV 重映射会失败。

### 8. 阴影渲染条件

`EntityRenderer::shouldRenderShadow()` 会检查实体的地面接触状态。如果自定义实体没有正确设置地面接触状态，阴影可能不显示。
