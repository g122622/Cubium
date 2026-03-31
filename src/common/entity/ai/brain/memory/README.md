# Memory 模块 - Brain 记忆系统

## 概述

Memory 模块实现了 Brain AI 系统的记忆存储机制。记忆模块允许实体存储和检索关于世界的各种信息，用于驱动 AI 行为决策。

## 目录结构

```
memory/
├── Memory.hpp                  # Memory 类模板 - 带TTL的记忆值
├── MemoryModuleType.hpp        # MemoryModuleType 类 - 类型安全的记忆类型标识
├── MemoryModuleType.cpp        # MemoryModuleTypes 初始化实现
├── MemoryModuleStatus.hpp      # MemoryModuleStatus 枚举 - 记忆状态
├── MemoryModules.hpp           # 村民记忆模块便捷别名
└── README.md
```

## 核心类

### Memory<T>

带生存时间(TTL)的记忆值包装器：

```cpp
// 创建永久记忆
auto mem = Memory<BlockPos>::permanent(bedPos);

// 创建带TTL的记忆（100 tick后过期）
auto mem = Memory<EntityId>::timed(targetId, 100);

// 每tick更新
mem.tick();

// 检查是否过期
if (mem.isExpired()) {
    // 忘记这个记忆
}
```

### MemoryModuleType<T>

类型安全的记忆类型标识符：

```cpp
// 定义记忆类型
const MemoryModuleType<BlockPos>* NEAREST_BED;

// 注册到Brain
brain.registerMemory(MemoryModuleTypes::NEAREST_BED);

// 设置记忆值
brain.setMemory(MemoryModuleTypes::NEAREST_BED, bedPos);

// 获取记忆值
auto bedPos = brain.getMemory<BlockPos>(MemoryModuleTypes::NEAREST_BED);
```

### MemoryModuleTypes

全局记忆类型注册表，包含所有预定义的记忆类型：

- **位置相关**: HOME, JOB_SITE, MEETING_POINT, NEAREST_BED 等
- **实体相关**: ATTACK_TARGET, NEAREST_HOSTILE, BREED_TARGET 等
- **移动相关**: PATH, WALK_TARGET, LOOK_TARGET
- **状态相关**: ADMIRING_ITEM, IS_IN_WATER, AGGRESSIVE 等
- **时间相关**: LAST_SLEPT, LAST_WORKED_AT_POI 等

### MemoryModules

村民记忆模块便捷别名命名空间，提供更易读的名称：

```cpp
// 使用别名（等同于 MemoryModuleTypes::HOME）
brain.registerMemory(MemoryModules::HOME);
```

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

## 参考 MC 1.16.5

本模块对应 Minecraft Java Edition 1.16.5 的 `net.minecraft.world.server.ServerWorld` 和 `net.minecraft.entity.ai.brain.memory` 包。
