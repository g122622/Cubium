# Timeline 时间线系统

时间线按时间（dayTime / gameTime）驱动环境属性：用关键帧轨道描述属性值随时间的演化，支持周期回绕与缓动插值。

## 目录结构

```
timeline/
├── Keyframe.hpp                 # 关键帧（ticks + 值）
├── EasingType.hpp               # 缓动类型（CONSTANT / LINEAR / 具名缓动 / 三次贝塞尔）
├── KeyframeTrack.hpp            # 关键帧轨道与构建器
├── KeyframeTrackSampler.hpp     # 关键帧采样器（烘焙分段 + 周期回绕插值）
├── AttributeTrack.hpp           # 属性轨道（属性修改器 + 关键帧轨道，可带轨道级缓动）
├── AttributeTrackSampler.hpp    # 属性轨道采样器（按 tick 缓存采样结果并应用修改器）
├── Timeline.hpp                 # 时间线与构建器（类型擦除存储异构轨道）
├── TimelineRegistry.hpp/cpp     # 时间线注册表（按 `minecraft:xxx` 名解析/列举）
└── Timelines.hpp/cpp            # 预定义时间线（DAY / MOON / VILLAGER_SCHEDULE / EARLY_GAME）
```

## 内部模块关系

```
Timelines ──构建──> Timeline::Builder ──添加──> AttributeTrack
                          │                          │
                          │                          └──> AttributeModifier / KeyframeTrack<Argument>
                          ▼
Timeline ──createTrackSampler──> AttributeTrackSampler <── EnvironmentAttributeLayer<Value>::TimeBased
                                        │
                                        ├──烘焙──> KeyframeTrackSampler <── KeyframeTrack <── Keyframe
                                        └──使用──> attribute::AttributeModifier / LerpFunction

TimelineRegistry <── Timelines::bootstrap()（登记全部预定义时间线与环境属性）
```

## 上下游外部依赖关系

**依赖（上游）**：
- `world/attribute/` — 属性类型、属性修改器、插值函数、环境属性层接口

**被依赖（下游）**：
- `world/attribute/DimensionAttributes.cpp` — 按 `DimensionType::timelines()` 装配时间线层
- `world/attribute/EnvironmentAttributeSystem` — 时间线层作为 `TimeBased` 层参与求值
- `entity/ai/brain/Brain.hpp` — `setTimeline` 创建活动轨道采样器，`_updateActivity` 按 dayTime 采样当前活动
- `entity/entities/villager/VillagerEntity.cpp` — 按成年/幼年注册对应的活动属性
- `server/registry/RegistryBootstrap.cpp` — `Timelines::bootstrap()` 预热预定义时间线

## 生命周期

`Timelines::bootstrap()` 是幂等的装配入口（`std::call_once`）：登记全部预定义环境属性到
`EnvironmentAttributeRegistry`，再登记全部预定义时间线到 `TimelineRegistry`。凡按注册名解析维度
时间线的代码（如 `DimensionAttributes::createSystem`）都依赖它，应先调用一次。

`TimelineRegistry` 以 `std::deque` 持有从数据包加载的时间线，保证已发放的 `Timeline*` 指针稳定。

## 容易踩的坑

### 1. 周期回绕段

`bakeSegments` 在带周期时会额外生成两段：**回绕段**（末关键帧 → 首关键帧，跨越周期边界）与**延伸段**（末关键帧 → 下一周期的首关键帧）。因此查询早于首个关键帧的 dayTime（例如 0）得到的是末关键帧的值，而非首关键帧的值——这正是村民在 0 tick 处于 REST 的原因。

推论：周期内最后一个关键帧之后的时刻**不保持末值**，而是在延伸段内插值到下一个周期的首关键帧。例如关键帧 `0 → 1、1000 → 3`、周期 2000 时，1500 对应 2 而非 3。

### 2. 采样边界左闭右开

`getSegmentAt` 取满足 `tick < segment.toTicks` 的首个分段，故 `tick == fromTicks` 落在当前段返回 `fromValue`，而 `tick == toTicks` 落到下一段返回其 `fromValue`。对阶跃插值而言，效果即"抵达关键帧时切换值"。

### 3. 轨道以属性地址为键

`Timeline` 用 `std::any` 类型擦除存储异构轨道，以 `EnvironmentAttribute` 的地址作键。属性必须是静态对象（地址稳定），否则 `createTrackSampler` 断言失败。

### 4. 采样器缓存必须随系统失效

`AttributeTrackSampler` 以"tick 序号"判定缓存：同一序号内多次求值只采样一次。
`EnvironmentAttributeSystem::invalidateTickCache()` 递增系统序号后，会一并调用各时间线层的
`invalidateTickCache()` 清空采样缓存；否则层会继续命中旧缓存，按过期的 dayTime 返回值。

> 历史教训：曾因系统只递增序号、不清层缓存，导致"每 tick 失效一次"形同虚设（某个值被无限复用）。
> 这属于**静默错误**——接口齐全、无崩溃，只有跨 tick 的值变化才能暴露。

### 5. 值/参数异构轨道需要适配器

`createTrackSampler<Value, Argument>` 返回 `AttributeTrackSampler<Value, Value>`：关键帧值按属性的值表示
给出，再转换为修改器参数。当 `Value` 与 `Argument` 不同（如颜色灰度混合）时，修改器必须是
`TypedModifierAdapter`，采样器通过 `applyArgumentValue` 完成"参数 → 值"的转换。

### 6. 预定义时间线的列举顺序不可随意调整

`Timelines::bootstrap()` 的列举顺序与 `RegistryDataBuilder` 中 `minecraft:timeline` 注册表条目顺序一致（DAY, MOON, VILLAGER_SCHEDULE, EARLY_GAME），网络层标签索引据此计算。

### 7. 缓动的具名分派与精度

`EasingType` 以枚举标识具名缓动（`LINEAR` / `CONSTANT` 在 `apply` 中直接返回，无函数间接调用），
三次贝塞尔另以控制点字段承担。具名缓动函数内部大量使用 `double`（`std::pow`）与 x87 中间值，
返回值必须经 `static_cast<f32>(...)` 落到单精度：直接返回 double 时，调用方若为 `f32` 结果且未显式
转换，可能读到未舍入的扩展精度值（实测 `1500/2000` 得到 0.75 而非 0.5），整段曲线错位且难以定位。