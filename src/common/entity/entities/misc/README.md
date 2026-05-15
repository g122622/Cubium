# 杂项实体

本目录包含其他类别的实体。

## 目录结构

```
misc/
├── MiscEntities.hpp/cpp     # 杂项实体定义
└── README.md                # 本文档
```

## 实体列表

| 实体 | 说明 | 特性 |
|------|------|------|
| FallingBlockEntity | 下落方块 | 沙子、砾石下落，可造成伤害 |
| TNTEntity | TNT实体 | 倒计时爆炸 |
| EyeOfEnderEntity | 末影之眼 | 飞向要塞 |
| EvokerFangsEntity | 唤魔者尖牙 | 地刺攻击 |

**注意**：潮涌核心 (Conduit) 不是实体，而是方块实体，实现位于 `src/common/world/blockentity/processing/ConduitEntity.hpp/cpp`。

## 下落方块

FallingBlockEntity 是沙子、砾石等重力方块下落时创建的实体。参考 MC 1.16.5 `net.minecraft.block.FallingBlock`。

### 属性

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| m_blockId | u32 | 0 | 下落的方块ID |
| m_hurtEntities | bool | false | 是否伤害实体（铁砧=true） |
| m_placeBlock | bool | true | 是否应该放置方块 |
| m_shouldDropItem | bool | true | 是否应该掉落物品 |
| m_dontSetBlock | bool | false | 是否不放置方块（铁砧损坏时） |
| m_fallStartY | f64 | 0.0 | 下落起始Y坐标（用于计算伤害） |
| m_fallTime | i32 | 0 | 下落时间（tick） |

### 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| HURT_AMOUNT | 2.0f | 每格下落伤害系数 |
| MAX_HURT_AMOUNT | 40 | 最大伤害值（20颗心） |
| MAX_FALL_TIME | 600 | 最大下落时间（30秒） |

### 公共方法

```cpp
// 方块ID
void setBlockId(u32 blockId);
u32 getBlockId() const;

// 伤害实体
void setHurtEntities(bool hurt);
bool shouldHurtEntities() const;

// 下落起始位置
void setFallStartPos(f64 y);

// 放置/掉落控制
bool shouldPlaceBlock() const;
void setShouldDropItem(bool drop);
bool shouldDropItem() const;
void setDontSetBlock(bool dontSet);
bool dontSetBlock() const;
```

### 行为流程

1. **下落阶段**：
   - 受重力影响（每tick Y速度-0.04）
   - 空气阻力（每tick速度×0.98）
   - 地面检测触发落地处理

2. **落地处理** (`handleLanding()`)：
   - 如果 `m_hurtEntities=true`：计算伤害并伤害碰撞箱内实体
   - 如果 `m_dontSetBlock=true`：调用 `FallingBlock::onBroken()` 回调
   - 否则尝试放置方块：
     - 成功：调用 `FallingBlock::onEndFalling()` 回调
     - 失败：掉落物品（如果 `shouldDropItem=true` 且游戏规则允许）

3. **放置条件** (`tryPlaceBlock()`)：
   - 下方方块不可穿透（非空气、非液体、非火焰）
   - 目标位置可替换（空气或可替换材质）
   - `setBlockState()` 成功

4. **伤害计算** (`hurtEntities()`)：
   - 伤害 = min((下落距离-1) × 2.0, 40)
   - 铁砧使用 `DamageType::Anvil`，其他使用 `DamageType::FallingBlock`

### 物品掉落

当方块无法放置时：
1. 检查 `shouldDropItem()` 标志
2. 检查游戏规则 `doEntityDrops`
3. 使用 `BlockItemRegistry` 获取方块对应物品
4. 使用 `ItemDropHelper::spawnItemEntity()` 生成物品实体

### 与 FallingBlock 方块的交互

`FallingBlock` 基类定义了以下回调：

```cpp
virtual void onStartFalling(IWorld& world, const BlockPos& pos, FallingBlockEntity& entity);
virtual void onEndFalling(IWorld& world, const BlockPos& pos, 
                         const BlockState& fallingState, const BlockState& hitState, 
                         FallingBlockEntity& entity);
virtual void onBroken(IWorld& world, const BlockPos& pos, FallingBlockEntity& entity);
```

- **onStartFalling**：方块开始下落时调用（如铁砧设置伤害标志）
- **onEndFalling**：方块成功放置时调用（如混凝土粉末遇水固化）
- **onBroken**：方块无法放置时调用（如铁砧损坏音效）

### 参考

- MC 1.16.5 `net.minecraft.entity.item.FallingBlockEntity`
- FallingBlock 基类：`src/common/world/block/blocks/FallingBlock.hpp`
- 物品掉落：`src/common/entity/utils/ItemDropHelper.hpp`

## TNT实体

TNT实体是被激活的TNT方块，倒计时后爆炸。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| fuse | 0 | 引信倒计时（tick） |
| explosionRadius | 4.0f | 爆炸半径 |
| exploded | false | 是否已爆炸 |
| owner | nullptr | 点燃者（用于伤害归属） |

### 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| DEFAULT_FUSE | 80 | 默认引信时间（4秒） |

### 行为

1. **点燃**：调用 `ignite()` 设置引信时间为80 tick
2. **倒计时**：每tick引信减1
3. **物理**：重力加速度0.04/tick，空气阻力0.98/tick，地面弹跳系数0.7/0.5
4. **爆炸**：引信归零时调用 `createExplosion()`，爆炸模式为 `Break`（破坏方块但不掉落物品）
5. **粒子**：客户端模式下，每tick有1/3概率在TNT上方生成Smoke粒子，粒子轻微向上飘动

### 粒子效果

客户端模式下（`isClientSide() == true`）：
- 每tick有1/3概率生成烟雾粒子
- 粒子类型：`ParticleTypeId::Smoke`
- 粒子位置：TNT上方，带随机偏移（±0.3格）
- 粒子速度：轻微向上飘动（0.02 + random*0.02）

### 工厂方法

```cpp
static std::unique_ptr<Entity> create(IWorld* world);
```

### 注册

TNTEntity 已在 `VanillaEntities::doRegisterAll()` 中注册，实体类型为 `minecraft:tnt`。

### 参考

- MC 1.16.5 `net.minecraft.entity.item.TNTEntity`
- 爆炸系统：`src/common/world/explosion/Explosion.hpp`

## 末影之眼

- 飞向最近的要塞
- 有几率碎裂掉落
- 最大飞行时间80tick

## 唤魔者尖牙

### 行为

1. 延迟出现
2. 尖牙冒出动画
3. 攻击范围内实体
4. 消失

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| delay | 0 | 出现延迟 |
| damage | 6.0 | 伤害值 |
| warmup | 14 | 攻击准备时间 |
| lifetime | 22 | 总生命周期 |

## 实现状态

| 组件 | 状态 |
|------|------|
| FallingBlockEntity | ✅ 完成 - 下落、落地放置、物品掉落、伤害实体 |
| TNTEntity | ✅ 完成 - 点燃、爆炸、物理、实体注册 |
| EyeOfEnderEntity | ⚠️ 框架完成，TODO需填充 |
| ConduitEntity | ✅ 已移除 - 功能已在 `blockentity/processing/ConduitEntity` 中完整实现 |
| EvokerFangsEntity | ⚠️ 框架完成，TODO需填充 |
| WardenWarningEffect | ⚠️ 框架完成，TODO需填充 |

**注意**：潮涌核心的功能不在本文件中，完整实现位于：
- 方块实体：`src/common/world/blockentity/processing/ConduitEntity.hpp/cpp`
- 方块：`src/common/world/block/blocks/ocean/ConduitBlock.hpp/cpp`

## 测试覆盖

FallingBlockEntity 测试位于 `tests/common/entity/entities/misc/FallingBlockEntityTest.cpp`，包含 19 个测试用例：
- 默认构造、实体尺寸、不可推动、不可碰撞
- 方块ID设置、伤害标志、下落起始位置
- 掉落物品标志、不放置方块标志
- 落地放置方块、无法放置时掉落物品
- 游戏规则影响、重力应用、空气阻力、最大下落时间
