# 玩家渲染器

本目录包含玩家实体的渲染器实现。

## 目录结构

```
player/
├── PlayerRenderer.hpp   # 玩家渲染器头文件
├── PlayerRenderer.cpp   # 玩家渲染器实现
└── README.md            # 本文档
```

## 内部模块关系

```
PlayerRenderer
├── 继承体系
│   ├── EntityRenderer (基类：阴影渲染、基础实体渲染)
│   └── IEntityRenderer<Player, PlayerModel> (接口：模型访问、纹理获取)
├── 核心组件
│   ├── PlayerModel m_model (玩家模型，支持标准/纤细手臂)
│   └── LayerRenderer[] m_layers (层渲染器列表)
└── 层渲染器（按顺序添加）
    ├── HeldItemLayer (手持物品层)
    ├── HeadLayer (头部物品层：头盔、南瓜等)
    ├── CapeLayer (披风层)
    └── ElytraLayer (鞘翅层)
```

**渲染流程：**
1. `render()` → 设置模型可见性 → 计算动画参数 → 渲染模型 → 渲染阴影
2. `renderLayersPipeline()` → 遍历层渲染器 → 传递纹理（披风、鞘翅）→ 渲染各层
3. `renderRightArm()` / `renderLeftArm()` → 第一人称手臂渲染

## 上下游外部依赖关系

**依赖了谁（上游）：**

| 模块 | 用途 |
|------|------|
| `entity/core/EntityRenderer` | 实体渲染器基类 |
| `entity/core/IEntityRenderer` | 实体渲染器接口 |
| `entity/model/player/PlayerModel` | 玩家模型 |
| `entity/layer/equipment/HeldItemLayer` | 手持物品层 |
| `entity/layer/equipment/HeadLayer` | 头部物品层 |
| `entity/layer/cosmetic/CapeLayer` | 披风层 |
| `entity/layer/cosmetic/ElytraLayer` | 鞘翅层 |
| `common/entity/entities/player/Player` | 玩家实体类 |
| `common/resource/ResourceLocation` | 纹理资源路径 |

**被谁依赖（下游）：**

| 模块 | 用途 |
|------|------|
| `renderer/RendererRegistration.cpp` | 注册玩家渲染器到工厂 |
| `client/world/ClientWorld` | 通过 EntityRendererManager 渲染玩家 |
| `client/skin/ClientSkinManager` | 设置玩家皮肤纹理 |

## 命名空间

```cpp
namespace mc::client::renderer::entity::renderer::player {
    class PlayerRenderer;
}
```

## 容易踩的坑

### 1. 纤细手臂模型差异

`PlayerRenderer(bool slimArms)` 构造参数决定手臂类型。纤细手臂的模型尺寸和纹理坐标与标准手臂不同，创建时必须传入正确参数。

### 2. 层渲染器纹理传递

披风和鞘翅纹理通过 `dynamic_cast` 在 `renderLayersPipeline()` 中传递给对应的层渲染器。如果层渲染器类型不匹配，纹理不会被设置。

### 3. 模型可见性设置顺序

`setModelVisibilities()` 必须在渲染前调用，它会：
1. 设置所有部件可见
2. 根据 `playerModelParts()` 设置外层皮肤可见性
3. 设置蹲伏/游泳状态

**注意**：披风可见性由 `CapeLayer` 单独控制，不在 `setModelVisibilitiesFromFlags` 中。

### 4. 第一人称手臂渲染

`renderRightArm()` / `renderLeftArm()` 会重置动画状态（setAngles 参数全为 0、swingProgress 为 0、crouching/swimming 为 false），仅渲染手臂部件，用于第一人称视角。

### 5. determineArmPose 未完成

当前 `determineArmPose()` 返回固定值 `ArmPose::Empty`，等待物品系统完善后实现手持物品姿态判定（弓、弩、盾牌等）。
