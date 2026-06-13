# blockentity/ - 方块实体渲染器

方块实体渲染器模块，负责渲染需要动态效果的方块实体（如箱子、活塞、信标、旗帜等）。

## 目录结构

```text
blockentity/
├── IBlockEntityRenderer.hpp          # 渲染器接口模板，类型安全的渲染契约
├── BlockEntityRenderer.hpp/cpp       # 渲染器辅助类，提供方块模型渲染、光照获取等通用功能
├── BlockEntityRendererDispatcher.hpp/cpp  # 渲染器调度器，管理注册和按类型分派
├── README.md                          # 本文件
├── model/                             # 方块实体模型
│   ├── BlockEntityModel.hpp/cpp       # 方块实体模型基类，部件管理和网格生成
│   ├── ChestModel.hpp/cpp             # 箱子模型，支持单箱和双箱
│   ├── BeaconBeamModel.hpp/cpp        # 信标光束模型，双层渲染和旋转动画
│   ├── BannerModel.hpp/cpp            # 旗帜模型，支持站立和墙壁两种形态
│   └── README.md                      # 模型子模块文档
└── renderers/                         # 具体渲染器实现
    ├── PistonRenderer.hpp/cpp         # 活塞渲染器，移动方块插值动画
    ├── ChestRenderer.hpp/cpp          # 箱子渲染器，盖子开合动画
    ├── BeaconRenderer.hpp/cpp         # 信标渲染器，全局光束渲染
    └── BannerRenderer.hpp/cpp         # 旗帜渲染器，旗帜飘动动画
```

## 内部模块关系

```
BlockEntityRendererDispatcher（调度器）
    └── 管理所有 BlockEntityRendererBase 实例
            │
            ├── ChestRenderer ──────使用────→ ChestModel
            ├── BeaconRenderer ─────使用────→ BeaconBeamModel
            ├── BannerRenderer ─────使用────→ BannerModel
            └── PistonRenderer ─────（直接渲染方块模型，无专用模型类）

BlockEntityRendererHelper（辅助工具）
    └── 被 renderers 使用，提供方块模型渲染和光照获取

IBlockEntityRenderer<TEntity>（模板接口）
    └── BlockEntityRendererBase（类型擦除基类）
            └── BlockEntityRenderer<TEntity>（类型安全模板）
```

## 上下游外部依赖关系

### 依赖（上游）

- `common/world/blockentity/` - 方块实体定义（ChestEntity、BeaconEntity、PistonBlockEntity 等）
- `common/world/block/` - 方块状态
- `client/renderer/trident/core/` - Trident 上下文、纹理图集
- `client/renderer/trident/entity/model/core/ModelRenderer.hpp` - 模型部件系统（被 ChestModel、BannerModel 使用）
- `client/resource/BlockModelCache` - 方块模型缓存

### 被依赖（下游）

- `client/renderer/trident/core/TridentEngine.hpp` - 初始化渲染器调度器
- `client/world/ClientWorld` - 渲染方块实体时调用调度器

## 与 EntityRenderer 的区别

| 特性 | EntityRenderer | BlockEntityRenderer |
|------|----------------|---------------------|
| 数据来源 | `ClientEntity` | `BlockEntity` |
| 渲染位置 | 世界坐标（实体位置） | 方块坐标 + 动画偏移 |
| 光照 | 实体光照计算 | 方块光照（天空光+方块光） |
| 动画 | `AnimationContext` | `partialTick` + `gameTime` 驱动 |
| 全局可见 | 少数实体 | 信标光束等 |

## 容易踩的坑

1. **全局渲染器必须设置 `isGlobalRenderer()`**：信标光束等需要跨区块可见的渲染器必须返回 `true`，否则会被渲染距离裁剪掉

2. **BeaconBeamModel 不继承 BlockEntityModel**：信标光束是程序化生成的几何体，不使用 ModelRenderer 部件系统，调用时需要额外传入 `gameTime` 和 `partialTick`

3. **箱子缓动函数**：箱子开合使用三次缓动，公式为 `eased = 1.0 - (1.0 - angle)³`，而非线性插值

4. **信标光束旋转公式**：`(floorMod(gameTime, 40) + partialTick) * 2.25 - 45` 度，不要直接使用 gameTime

5. **信标光束双层渲染**：内层光束 radius=0.2、alpha=1.0，外层光晕 radius=0.25、alpha=0.125，需分别渲染

6. **圣诞节纹理检测**：ChestRenderer 在 12月24-26日使用 `textures/entity/chest/christmas.png`

7. **旗帜类型差异**：站立旗帜有16个方向旋转，墙壁旗帜只有4个方向，初始化部件不同

8. **纹理尺寸**：ChestModel 和 BannerModel 的纹理尺寸都是 64x64，创建部件时需正确设置

9. **活塞插值**：`getProgress(partialTick)` 使用 `lerp(lastProgress, progress, partialTick)`；`getExtendedProgress(progress)` 返回 `extending ? progress - 1.0 : 1.0 - progress`

10. **gameTime 参数**：`render()` 接口的 `gameTime` 参数由 `BlockEntityRendererDispatcher::render()` 传入，用于驱动需要时间驱动动画的渲染器（如旗帜飘动、信标光束旋转）。调用方需从 `TridentEngine::m_gameTime` 或 `ClientWorld::gameTime()` 获取。当 `gameTime = 0` 时，旗帜不会飘动、信标光束不会旋转。
