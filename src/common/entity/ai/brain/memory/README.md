# Memory 模块 - Brain 记忆系统

## 概述

Memory 模块实现了 Brain AI 系统的记忆存储机制。记忆模块允许实体存储和检索关于世界的各种信息，用于驱动 AI 行为决策。

**对齐状态**: 与 MC 1.16.5 完全对齐

## 目录结构

```
memory/
├── Memory.hpp                  # Memory 类模板 - 带TTL的记忆值
├── MemoryModuleType.hpp        # MemoryModuleType 类 - 类型安全的记忆类型标识
├── MemoryModuleType.cpp        # MemoryModuleTypes 初始化实现
├── MemoryModuleStatus.hpp      # MemoryModuleStatus 枚举 - 记忆状态
├── MemoryModules.hpp           # 村民记忆模块便捷别名
├── IPositionTarget.hpp         # 位置目标接口
├── BlockPosTarget.hpp          # 方块位置目标实现
├── WalkTarget.hpp              # 行走目标封装
└── README.md
```

## 核心类

### Memory<T>

带生存时间(TTL)的记忆值包装器。完全对齐 MC 1.16.5 的 `net.minecraft.entity.ai.brain.Memory`。

```cpp
// 创建永久记忆 (MC: Memory.func_234068_a_() / permanent())
auto mem = Memory<BlockPos>::permanent(bedPos);

// 创建带TTL的记忆 (MC: Memory.func_234069_a_() / timed())
auto mem = Memory<EntityId>::timed(targetId, 100);

// 每tick更新 (MC: Memory.func_234064_a_() / tick())
mem.tick();

// 检查是否过期 (MC: Memory.func_234073_d_() / isExpired())
if (mem.isExpired()) {
    // 忘记这个记忆
}

// 检查是否有TTL限制 (MC: Memory.func_234074_e_() / hasTTL())
if (mem.hasTTL()) {
    // 这是一个临时记忆
}
```

### MemoryModuleType<T>

类型安全的记忆类型标识符。完全对齐 MC 1.16.5 的记忆类型注册表。

### MemoryModuleTypes - 全局记忆类型

MC 1.16.5 标准记忆类型：

| 分类 | 记忆类型 | 说明 |
|------|----------|------|
| **位置** | HOME, JOB_SITE, POTENTIAL_JOB_SITE, MEETING_POINT | GlobalPos 类型 |
| | NEAREST_BED, HIDING_PLACE, CELEBRATE_LOCATION, NEAREST_REPELLENT | BlockPos 类型 |
| | SECONDARY_JOB_SITE | List<GlobalPos> 类型 |
| **实体** | MOBS, VISIBLE_MOBS, VISIBLE_VILLAGER_BABIES | 实体列表 |
| | NEAREST_PLAYERS, NEAREST_VISIBLE_PLAYER | 玩家相关 |
| | ATTACK_TARGET, INTERACTION_TARGET, HURT_BY_ENTITY, AVOID_TARGET | 目标实体 |
| | BREED_TARGET, NEAREST_VISIBLE_ADULT, NEAREST_VISIBLE_WANTED_ITEM | 其他实体 |
| **猪灵** | NEAREST_VISIBLE_HUNTABLE_HOGLIN, NEAREST_VISIBLE_BABY_HOGLIN | 猪灵兽相关 |
| | NEAREST_ADULT_PIGLINS, NEAREST_VISIBLE_ADULT_PIGLINS | 猪灵相关 |
| | VISIBLE_ADULT_PIGLIN_COUNT, VISIBLE_ADULT_HOGLIN_COUNT | 计数 |
| **移动** | PATH, WALK_TARGET, LOOK_TARGET | 导航相关 |
| **门** | INTERACTABLE_DOORS, OPENED_DOORS | 门相关 |
| **战斗** | ATTACK_COOLING_DOWN, HURT_BY | 战斗状态 |
| **时间** | HEARD_BELL_TIME, CANT_REACH_WALK_TARGET_SINCE | 时间戳 |
| | LAST_SLEPT, LAST_WOKEN, LAST_WORKED_AT_POI | 日常活动时间 |
| **状态** | ADMIRING_ITEM, ADMIRING_DISABLED, HUNTED_RECENTLY | 猪灵状态 |
| | DANCING, ATE_RECENTLY, PACIFIED, UNIVERSAL_ANGER | 其他状态 |
| | GOLEM_DETECTED_RECENTLY | 铁傀儡检测 |
| **玩家** | TEMPTING_PLAYER, NEAREST_PLAYER_HOLDING_WANTED_ITEM | 玩家相关 |

扩展类型（非 MC 1.16.5 标准）：
- IS_IN_WATER, IS_PREGNANT, PLAY_DEAD, AGGRESSIVE 等扩展状态
- LIKED_NOTEBLOCK, LISTENING_NOTEBLOCK 等 1.17+ Allay 类型
- TONGUE_TARGET, RAM_TARGET 等 1.17+ 青蛙/山羊类型
- SNIFFER_SNIFFING_TARGET, SNIFFER_DIGGING 等 1.19+ Sniffer 类型

### MemoryModuleStatus

完全对齐 MC 1.16.5 的 `net.minecraft.entity.ai.brain.memory.MemoryModuleStatus`：

```cpp
enum class MemoryModuleStatus {
    VALUE_PRESENT,  // 记忆有值 (MC: VALUE_PRESENT)
    VALUE_ABSENT,   // 记忆无值 (MC: VALUE_ABSENT)
    REGISTERED      // 已注册   (MC: REGISTERED)
};
```

## 与 MC 1.16.5 的对齐状态

### 完全对齐的组件

| 组件 | 状态 | 说明 |
|------|------|------|
| Memory<T> | ✅ | TTL 逻辑、过期检测完全一致 |
| MemoryModuleStatus | ✅ | 枚举值完全一致 |
| MemoryModuleType | ✅ | 包含所有 MC 1.16.5 标准类型 |
| WalkTarget | ✅ | getSpeed(), getDistance() 方法命名一致 |
| IPositionTarget | ✅ | 对应 MC 的 IPosWrapper |
| BlockPosTarget | ✅ | 对应 MC 的 BlockPosWrapper |

### 关键修复

1. **Brain::hasRequiredMemories** - 修复了逻辑错误，现在当活动没有记忆要求时返回 `false`（与 MC 1.16.5 一致）

2. **MemoryModuleTypes** - 添加了缺失的猪灵相关记忆类型：
   - INTERACTABLE_DOORS
   - NEAREST_VISIBLE_HUNTABLE_HOGLIN
   - NEAREST_VISIBLE_BABY_HOGLIN
   - NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD
   - NEAREST_ADULT_PIGLINS
   - NEAREST_VISIBLE_ADULT_PIGLINS
   - NEAREST_VISIBLE_ADULT_HOGLINS
   - NEAREST_VISIBLE_ADULT_PIGLIN
   - VISIBLE_ADULT_PIGLIN_COUNT
   - VISIBLE_ADULT_HOGLIN_COUNT
   - NEAREST_PLAYER_HOLDING_WANTED_ITEM

3. **OPENED_DOORS** - 类型从 `std::vector<GlobalPos>` 修正为 `std::unordered_set<GlobalPos>`，与 MC 1.16.5 的 `Set<GlobalPos>` 一致

4. **WalkTarget** - 方法重命名：`getSpeedModifier()` → `getSpeed()`, `getCloseEnoughDist()` → `getDistance()`

## 使用方式

### 初始化

程序启动时必须调用初始化：

```cpp
#include "entity/ai/brain/memory/MemoryModuleType.hpp"

// 在服务器初始化时调用
entity::ai::brain::memory::MemoryModuleTypes::initialize();
```

### Brain 集成

```cpp
// 注册记忆模块
brain.registerMemory(MemoryModuleTypes::HOME);
brain.registerMemory(MemoryModuleTypes::JOB_SITE);
brain.registerMemory(MemoryModuleTypes::NEAREST_HOSTILE);

// 设置永久记忆
brain.setMemory(MemoryModuleTypes::HOME, GlobalPos(dimensionId, bedPos));

// 设置带TTL的记忆
brain.setMemoryWithTTL(MemoryModuleTypes::NEAREST_HOSTILE, hostileEntity, 200);

// 检查记忆是否存在
if (brain.hasMemory(MemoryModuleTypes::HOME)) {
    auto home = brain.getMemory<GlobalPos>(MemoryModuleTypes::HOME);
}

// 移除记忆
brain.removeMemory(MemoryModuleTypes::NEAREST_HOSTILE);
```

### 记忆状态检查

```cpp
// 检查记忆存在性
if (brain.hasMemory(type, MemoryModuleStatus::VALUE_PRESENT)) {
    // 记忆有值
}

if (brain.hasMemory(type, MemoryModuleStatus::VALUE_ABSENT)) {
    // 记忆已注册但无值
}

if (brain.hasMemory(type, MemoryModuleStatus::REGISTERED)) {
    // 记忆已注册
}
```

## 依赖关系

```
Memory.hpp
    └── Types.hpp (基础类型)

MemoryModuleType.hpp
    ├── Memory.hpp
    ├── string
    └── unordered_map

Brain.hpp
    ├── Memory.hpp
    ├── MemoryModuleType.hpp
    ├── MemoryModuleStatus.hpp
    └── Schedule.hpp
```

## 容易踩的坑

### 1. 忘记初始化

必须在使用任何记忆类型前调用 `MemoryModuleTypes::initialize()`，否则所有指针都是 nullptr。

### 2. 类型不匹配

获取记忆时类型必须与注册时完全匹配：

```cpp
// 错误：类型不匹配
brain.setMemory(MemoryModuleTypes::HOME, BlockPos(1, 2, 3));  // HOME 是 GlobalPos 类型
brain.getMemory<BlockPos>(MemoryModuleTypes::HOME);  // 返回 nullopt

// 正确：
brain.setMemory(MemoryModuleTypes::HOME, GlobalPos(dimensionId, BlockPos(1, 2, 3)));
auto home = brain.getMemory<GlobalPos>(MemoryModuleTypes::HOME);
```

### 3. 指针比较

记忆类型使用指针作为键，必须使用相同的指针：

```cpp
// 错误：创建了新的类型实例
MemoryModuleType<BlockPos> localType("local");
brain.registerMemory(&localType);  // 使用局部变量地址

// 正确：使用全局注册的类型
brain.registerMemory(MemoryModuleTypes::NEAREST_BED);
```

### 4. hasRequiredMemories 行为

注意：当活动没有配置记忆要求时，`hasRequiredMemories()` 返回 `false`，这与直觉相反但与 MC 1.16.5 一致。这意味着所有需要有条件启动的活动都必须显式配置记忆要求。

## 参考 MC 1.16.5

本模块对应 Minecraft Java Edition 1.16.5 的以下类：

- `net.minecraft.entity.ai.brain.Memory`
- `net.minecraft.entity.ai.brain.memory.MemoryModuleType`
- `net.minecraft.entity.ai.brain.memory.MemoryModuleStatus`
- `net.minecraft.entity.ai.brain.memory.WalkTarget`
- `net.minecraft.util.math.IPosWrapper`
- `net.minecraft.util.math.BlockPosWrapper`
