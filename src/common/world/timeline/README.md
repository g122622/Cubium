# Timeline 时间线系统

时间线按时间（dayTime）驱动环境属性：用关键帧轨道描述属性值随时间的演化，支持周期回绕与缓动插值。

## 目录结构

```
timeline/
├── Keyframe.hpp                 # 关键帧（ticks + 值）
├── EasingType.hpp               # 缓动类型（CONSTANT / LINEAR）
├── KeyframeTrack.hpp            # 关键帧轨道与构建器
├── KeyframeTrackSampler.hpp     # 关键帧采样器（烘焙分段 + 周期回绕插值）
├── AttributeTrack.hpp           # 属性轨道（属性修改器 + 关键帧轨道）
├── AttributeTrackSampler.hpp    # 属性轨道采样器（按 tick 缓存采样结果并应用修改器）
├── Timeline.hpp                 # 时间线与构建器（类型擦除存储异构轨道）
└── Timelines.hpp/cpp            # 预定义时间线（VILLAGER_SCHEDULE 等）
```

## 内部模块关系

```
Timelines ──构建──> Timeline::Builder ──添加──> AttributeTrack
                                                     │
Timeline ──createTrackSampler──> AttributeTrackSampler
                                        │
                                        ├──烘焙──> KeyframeTrackSampler <── KeyframeTrack <── Keyframe
                                        └──使用──> attribute::AttributeModifier / LerpFunction
```

## 上下游外部依赖关系

**依赖（上游）**：
- `world/attribute/` — 属性类型、属性修改器、插值函数、环境属性层接口

**被依赖（下游）**：
- `entity/ai/brain/Brain.hpp` — `setTimeline` 创建活动轨道采样器，`_updateActivity` 按 dayTime 采样当前活动
- `entity/entities/villager/VillagerEntity.cpp` — 按成年/幼年注册对应的活动属性
- `server/registry/RegistryBootstrap.cpp` — `Timelines::bootstrap()` 预热预定义时间线

## 容易踩的坑

### 1. 周期回绕段

`bakeSegments` 在带周期时会额外生成两段：**回绕段**（末关键帧 → 首关键帧，跨越周期边界）与**延伸段**（末关键帧 → 下一周期的首关键帧）。因此查询早于首个关键帧的 dayTime（例如 0）得到的是末关键帧的值，而非首关键帧的值——这正是村民在 0 tick 处于 REST 的原因。

### 2. 采样边界左闭右开

`getSegmentAt` 取满足 `tick < segment.toTicks` 的首个分段，故 `tick == fromTicks` 落在当前段返回 `fromValue`，而 `tick == toTicks` 落到下一段返回其 `fromValue`。对阶跃插值而言，效果即"抵达关键帧时切换值"。

### 3. 轨道以属性地址为键

`Timeline` 用 `std::any` 类型擦除存储异构轨道，以 `EnvironmentAttribute` 的地址作键。属性必须是静态对象（地址稳定），否则 `createTrackSampler` 断言失败。

### 4. Value 与 Argument 目前必须相同

`createTrackSampler` 带 `static_assert` 约束 `Value == Argument`。MC 通过 `LerpFunction.cast` 支持异构，C++ 侧的类型安全方案待设计（见源码 TODO）。

### 5. 预定义时间线的列举顺序不可随意调整

`Timelines::bootstrap()` 的列举顺序与 `RegistryDataBuilder` 中 `minecraft:timeline` 注册表条目顺序一致（DAY, MOON, VILLAGER_SCHEDULE, EARLY_GAME），网络层标签索引据此计算。
