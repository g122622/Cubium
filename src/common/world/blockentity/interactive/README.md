# 交互类方块实体 (Interactive Block Entities)

此目录包含交互类方块实体的实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `EnchantingTableEntity.hpp/cpp` | 附魔台方块实体 |
| `DispenserBlockEntity.hpp/cpp` | 发射器/投掷器方块实体 |
| `PistonBlockEntity.hpp/cpp` | 活塞方块实体 |

## 附魔台方块实体 (EnchantingTableEntity)

### 功能

- 检测周围书架计算附魔力量
- 管理书本动画状态
- 支持自定义名称（铁砧重命名）

### 附魔力量计算

```
附魔台周围2格范围内:
- 水平距离 = 2（曼哈顿距离）
- 垂直距离 = 0 或 1

书架与附魔台之间必须是空气。

每个有效书架增加1点附魔力量，最大15点。
```

### 书架检测逻辑

```
    S . . . S
    . . . . .
    . . T . .
    . . . . .
    S . . . S

S = 书架（距离2格）
T = 附魔台
. = 空气（必须）
```

### 动画系统

附魔台的书本有三种动画：
1. **翻开动画**: 玩家靠近时书本翻开
2. **翻转动画**: 书本在空中轻微旋转
3. **书页动画**: 书页翻动效果

### API

```cpp
// 创建附魔台实体
EnchantingTableEntity table(BlockPos(0, 0, 0));

// 重新计算附魔力量（书架变化后调用）
table.recalculateEnchantPower(world);

// 获取附魔力量
i32 power = table.getEnchantPower(); // 0-15

// 自定义名称
table.setCustomName("My Enchanting Table");
String name = table.getCustomName();

// 动画更新（客户端渲染调用）
table.updateAnimation(world, dt);
f32 rotation = table.getBookRotation();
f32 openAmount = table.getBookOpenAmount();
f32 pageAngle = table.getBookPageAngle();
```

### 与其他组件的关系

- **EnchantingTableBlock**: 创建附魔台方块实体
- **World**: 方块实体注册和查询
- **Player**: 玩家交互和GUI打开

### 实现状态

| 功能 | 状态 |
|------|------|
| 附魔力量计算 | ✅ 完成 |
| 书架检测 | ✅ 完成（待 VanillaBlocks::BOOKSHELF） |
| 自定义名称 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |
| 书本动画 | ✅ 框架完成 |
| 玩家靠近检测 | ⏳ TODO |
| GUI打开 | ⏳ TODO |

## 活塞方块实体 (PistonBlockEntity)

### 功能

- 处理活塞移动动画（伸出/收回）
- 推动碰撞的实体
- 管理移动中的方块状态

### 活塞移动流程

```
1. 红石信号触发 → PistonBlock.neighborChanged()
2. 计算推动链 → PistonBlock.extend() 或 retract()
3. 创建移动活塞方块 → 创建 PistonBlockEntity
4. 每 tick 更新进度 → tick() 中 progress += 0.5
5. 动画完成后 → 替换为最终方块
```

### 动画进度

```
progress: 0.0 → 1.0 （每 tick +0.5，共 2 tick）
伸出：offset = progress - 1.0（-1.0 → 0.0）
收回：offset = 1.0 - progress（1.0 → 0.0）
```

### API

```cpp
// 创建活塞方块实体
auto piston = std::make_unique<PistonBlockEntity>(
    pos, std::move(blockState), Direction::North, true, true);

// 获取动画进度
float progress = piston->getProgress(partialTick);  // 插值进度

// 获取渲染偏移
float offsetX = piston->getOffsetX(partialTick);
float offsetY = piston->getOffsetY(partialTick);
float offsetZ = piston->getOffsetZ(partialTick);

// 检查状态
bool extending = piston->isExtending();
Direction facing = piston->getFacing();
bool complete = piston->isComplete();

// 清除活塞实体（动画完成后）
piston->clearPistonBlockEntity(world);
```

### 与其他组件的关系

- **PistonBlock**: 创建和管理活塞方块实体
- **PistonHeadBlock**: 活塞头方块（伸出时显示）
- **World**: 方块实体生命周期管理
- **Entity**: 实体推动系统

### 实现状态

| 功能 | 状态 |
|------|------|
| 方块实体基础结构 | ✅ 完成 |
| 动画进度管理 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |
| 渲染偏移计算 | ✅ 完成 |
| 实体推动 | ⏳ 框架完成（待实体系统完善） |
| 方块状态保存/恢复 | ⏳ TODO（待 BlockState 序列化） |
| 蜂蜜块特殊处理 | ⏳ TODO |
