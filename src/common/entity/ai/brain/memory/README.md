# Memory 模块 - Brain 记忆系统

## 目录结构

```
memory/
├── Memory.hpp                  # 带TTL的记忆值包装器
├── MemoryModuleType.hpp        # 记忆类型标识符模板
├── MemoryModuleType.cpp        # 记忆类型注册实现（85+种类型）
├── MemoryModuleStatus.hpp      # 记忆状态枚举
├── MemoryModules.hpp           # 村民记忆模块便捷别名
├── IPositionTarget.hpp         # 位置目标抽象接口（用于LOOK_TARGET）
├── BlockPosTarget.hpp          # 方块位置目标实现
├── WalkTarget.hpp              # 行走目标封装（位置+速度+距离）
└── README.md
```

## 内部模块关系

```
Memory.hpp (基础)
    └── MemoryModuleType.hpp (依赖 Memory<T>)
            ├── MemoryModuleType.cpp (实现)
            └── MemoryModules.hpp (别名)

IPositionTarget.hpp (接口)
    └── BlockPosTarget.hpp (实现)
            └── WalkTarget.hpp (依赖 IPositionTarget, BlockPosTarget)
```

## 上下游外部依赖关系

**被依赖（下游）**：
- `brain/Brain.hpp` - 使用 Memory、MemoryModuleType、MemoryModuleStatus
- `brain/sensor/*.hpp` - 传感器设置/读取记忆值
- `brain/task/*.hpp` - 任务检查记忆状态

**依赖（上游）**：
- `common/core/Types.hpp` - 基础类型定义
- `common/util/math/Vector3.hpp` - 位置目标依赖
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/util/assert/AssertAll.hpp` - 断言宏

## 容易踩的坑

### 1. 忘记初始化

必须在使用任何记忆类型前调用 `MemoryModuleTypes::initialize()`，否则所有指针都是 nullptr。

### 2. 类型不匹配

获取记忆时类型必须与注册时完全匹配。HOME 是 `GlobalPos` 类型，不能用 `BlockPos` 获取。

### 3. 指针比较

记忆类型使用指针作为键，必须使用全局注册的类型指针，不能创建局部实例。

### 4. hasRequiredMemories 行为反直觉

当活动没有配置记忆要求时，`hasRequiredMemories()` 返回 `false`（与 MC 1.16.5 一致）。所有需要条件启动的活动都必须显式配置记忆要求。

### 5. OPENED_DOORS 类型

类型是 `std::unordered_set<GlobalPos>`，不是 `std::vector<GlobalPos>`。

### 6. WalkTarget 构造断言

WalkTarget 构造时会断言 target 非空，确保传入有效的 PositionTargetPtr。
