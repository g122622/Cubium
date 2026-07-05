# 物品使用上下文模块

本目录包含物品使用时的上下文类型，提供物品/方块放置所需的玩家、世界、位置、朝向等信息。

## 目录结构

```
context/
├── ItemUseContext.hpp/cpp        # 物品使用上下文基类（玩家、世界、位置、朝向等）
├── BlockItemUseContext.hpp/cpp   # 方块物品放置上下文（继承 ItemUseContext，新增放置位置计算、可替换判断、方向排序）
└── README.md
```

## 内部模块关系

```
ItemUseContext (基类)
  │
  └── BlockItemUseContext (方块放置上下文)
        ├── placementPos()           实际放置位置（点击位置或相邻位置）
        ├── adjacentPos()            相邻位置（击中面的另一侧）
        ├── canPlace()               是否可在放置位置放置
        ├── replacingClickedBlock()  是否替换点击的方块（如水、草）
        ├── horizontalDirection()    玩家水平朝向（NORTH/SOUTH/EAST/WEST）
        ├── placementDirection()     方块应面向的方向
        └── getNearestLookingDirections()  按玩家视线排序的 6 方向列表
```

## ItemUseContext 构造签名

```cpp
ItemUseContext(IWorld& world,
    Player* player,
    const ItemStack& stack,
    const Vector3& hitPos,
    const BlockPos& blockPos,
    Direction face,
    Hand hand,
    f32 playerYaw,
    f32 playerPitch);
```

参数说明：
- `world` 世界引用（非 const，支持放置时修改世界状态）
- `player` 玩家指针（可为 nullptr，此时依赖 playerYaw/playerPitch）
- `stack` 物品堆
- `hitPos` 击中点（世界坐标）
- `blockPos` 击中的方块位置
- `face` 击中的面
- `hand` 使用的手（主手或副手）
- `playerYaw` 玩家 yaw 角度（度数；MC 约定 0=南, 90=西, 180=北, 270=东）
- `playerPitch` 玩家 pitch 角度（度数；MC 约定正值=俯视，负值=仰视）

**注意**：构造参数不允许默认值（遵循 `docs/PROJECT_CONVENTIONS.md` 「函数参数和配置结构体不允许使用、设置默认值」）。所有调用方必须显式传入 `hand`、`playerYaw`、`playerPitch`。

## BlockItemUseContext 构造签名

```cpp
BlockItemUseContext(IWorld& world,
    Player* player,
    const ItemStack& stack,
    const Vector3& hitPos,
    const BlockPos& blockPos,
    Direction face,
    f32 playerYaw,
    f32 playerPitch);
```

`BlockItemUseContext` 默认 `hand = Hand::MainHand`（方块放置不区分主副手），并将 `playerYaw`/`playerPitch` 透传给 `ItemUseContext` 基类。

## getNearestLookingDirections 与 pitch

`getNearestLookingDirections()` 与 MC 1.21.11 `BlockPlaceContext.getNearestLookingDirections` 对齐：

1. 调用 `Direction.orderedByNearest(yaw, pitch)` 得到 6 方向数组
2. 若 `replacingClickedBlock == true`：直接返回该数组
3. 否则：将"点击面的反向"提到数组首位（保证优先尝试附着面）

`orderedByNearest` 与 MC 1.21.11 `Direction.orderedByNearest(Entity)` 一致：
- `pitch > 0`（俯视）→ Down 排在 Up 之前
- `pitch < 0`（仰视）→ Up 排在 Down 之前
- `pitch == 0`（水平）→ Y 轴方向不会排在首位
- 第 i 个与第 5-i 个互为相反方向

**pitch 来源**：优先使用 `player->pitch()`；当 `player == nullptr` 时回退到构造参数 `m_playerPitch`。生产代码中 `BlockInteractionManager` 通过 `ServerPlayerData->pitch` 显式传入，确保 player 为 nullptr 时仍能正确排序。

## 上下游外部依赖关系

**本目录依赖：**
- `common/world/IWorld.hpp` - 世界接口
- `common/entity/entities/player/Player.hpp` - 玩家实体（前向声明）
- `common/item/core/ItemStack.hpp` - 物品堆（前向声明）
- `common/util/Direction.hpp` - 方向枚举与工具
- `common/util/math/Vector3.hpp` - 向量类型
- `common/util/math/ray/Raycast.hpp` - 射线检测结果类型
- `common/world/block/BlockPos.hpp` - 方块位置

**依赖本目录的模块：**
- `item/items/block/BlockItem.cpp` - 在 `onItemUse` 中将 `ItemUseContext` 转换为 `BlockItemUseContext`
- `server/interaction/BlockInteractionManager.cpp` - 服务端构造 `BlockItemUseContext` 处理玩家放置
- `world/block/blocks/...` - 多个方块（`WallTorchBlock`、`LanternBlock`、`BannerBlock`、`CocoaBlock`、`SignBlock`、`HangingSignBlock`、`LightningRodBlock`、`WeatheringLightningRodBlock`）的 `getStateForPlacement` 调用 `getNearestLookingDirections()`
- `item/items/block/WallOrFloorItem.cpp` - 墙/地物品根据 `getNearestLookingDirections()` 选择放置变体

## 容易踩的坑

### 1. pitch 必须显式传入

`BlockItemUseContext` 构造函数没有默认参数。所有调用方必须显式传入 `playerYaw` 与 `playerPitch`。测试代码中常见的 `player == nullptr` 场景应传入 `0.0f` 表示水平视线。

### 2. player==nullptr 时的回退顺序

`getNearestLookingDirections()` 中：
- 若 `player != nullptr`：使用 `player->yaw()` 与 `player->pitch()`（实时值）
- 若 `player == nullptr`：回退到 `m_playerYaw` / `m_playerPitch`（构造时传入的快照）

这意味着测试代码可以通过构造参数模拟任意 yaw/pitch 组合，无需构造完整的 Player 实体。

### 3. BlockItemUseContext 的复制语义

`BlockItem::getBlockItemUseContext(BlockItemUseContext&)` 默认返回传入的 context 副本。由于 `BlockItemUseContext` 持有 `m_world` 引用与 `m_stack` 指针，复制时这些成员直接复制（浅拷贝），生命周期由原始 context 保证。

### 4. orderedByNearest 是文件内静态函数

`orderedByNearest(yaw, pitch)` 定义在 `BlockItemUseContext.cpp` 的匿名命名空间中，不对外暴露。如需在其他位置复用该算法，应提取到 `Direction.hpp` 的工具函数中，而非重复实现。
