# Attribute 环境属性系统

环境属性系统定义世界级的具名属性（活动、月相、天空颜色等），并提供按时间驱动这些属性所需的基础类型。

对应 MC 1.21.11 的 `net.minecraft.world.attribute` 包。层序与 vanilla 一致：
**维度常量层 → 群系位置层 → 时间线层 → 天气层（尚未接入）**。

## 目录结构

```
attribute/
├── AttributeModifier.hpp          # 修改器接口、操作标识、类型适配器与全部具名修改器
├── AttributeRange.hpp             # 属性值范围（校验 + 规范化）
├── AttributeType.hpp              # 属性类型（关键帧/部分tick 两个插值函数 + 可用的修改器库）
├── AttributeTypes.hpp/cpp         # 预定义属性类型（BOOLEAN / TRI_STATE / FLOAT / ANGLE_DEGREES /
│                                  #   RGB_COLOR / ARGB_COLOR / MOON_PHASE / ACTIVITY）
├── EnvironmentAttribute.hpp       # 环境属性定义与构建器
├── EnvironmentAttributes.hpp/cpp  # 预定义环境属性（visual/ audio/ gameplay/ 三个命名空间共 44 个）
├── EnvironmentAttributeMap.hpp/cpp      # 属性表（维度/群系声明的"属性 → 修改器 + 参数"，含数据包 JSON 入口）
├── EnvironmentAttributeValueJson.hpp/cpp # 属性值类型的 JSON 表示（数据包 valueCodec 等价物）
├── EnvironmentAttributeRegistryDataPack.hpp # 数据包参数 → 属性表条目的类型化桥接
├── EnvironmentAttributeRegistry.hpp/cpp  # 属性注册表（名字 ↔ 属性、属性类型注册表）
├── EnvironmentAttributeLayer.hpp  # 属性层接口（Constant / TimeBased / Positional）
├── EnvironmentAttributeSystem.hpp/inl/cpp  # 环境属性系统与构建器（烘焙层序列 + per-tick 缓存）
└── DimensionAttributes.hpp/cpp    # 按维度装配：维度常量层 + 群系位置层 + 维度声明的时间线层
```

## 内部模块关系

```
EnvironmentAttributes ──构建──> EnvironmentAttribute ──持有──> AttributeType ──持有──> LerpFunction
                                     │                              ▲
                                     └──持有──> AttributeRange       │
                                                              AttributeTypes

DimensionType ──attributes()──> EnvironmentAttributeMap ──> EnvironmentAttributeSystemBuilder ──> EnvironmentAttributeSystem
             ──timelines()───> TimelineRegistry（时间线层）            ▲
                                                                       │
                              AttributeTrackSampler ──实现──> EnvironmentAttributeLayer<Value>::TimeBased
```

## 上下游外部依赖关系

**依赖（上游）**：
- `entity/ai/brain/schedule/Activity.hpp` — `AttributeTypes::ACTIVITY` 与村民活动属性以 `Activity` 作为值类型
- `world/timeline/` — 时间线给出各属性的关键帧轨道
- `world/level/MoonPhase.hpp` — `MOON_PHASE` 属性与月相时间线的值类型
- `world/dimension/DimensionType.hpp` — 维度声明常量属性与时间线名

**被依赖（下游）**：
- `world/timeline/` — `AttributeTrack` 持有 `AttributeModifier`；`AttributeTrackSampler` 实现 `EnvironmentAttributeLayer::TimeBased`
- `entity/ai/brain/Brain.hpp` — 按活动环境属性创建轨道采样器
- `server/world/ServerWorld.cpp` — `environmentAttributes()` 持有维度级系统实例，每 tick 末尾失效缓存
- `block/blocks/pale_garden/EyeblossomEnvironment.hpp` — 按 `EYEBLOSSOM_OPEN` 查询眼眸花开合

## 系统语义

- **常量层折叠**：构建时若属性以常量层开头，其值随求值函数固化，无需每次求值重新计算。
- **per-tick 缓存**：非位置属性的查询结果按 tick 序号缓存；`invalidateTickCache()` 递增序号并
  同时清空系统缓存与各层的自身缓存（时间线采样器的关键帧采样缓存）。世界每 tick 末尾调用一次。
- **位置查询**：`getValue(attr, pos)` 对不受位置影响的属性直接复用维度缓存；受位置影响的属性
  每次求值，并在非位置查询中跳过位置层。
- **数据包属性**：群系/维度 JSON 的 `attributes` 由 `EnvironmentAttributeMap::Builder::setFromJson`
  解析（键为属性注册名；值可直接给出参数，或 `{"modifier": ..., "argument": ...}`），
  群系属性表由 `Biome::attributes()` 持有，维度级系统以群系位置层按查询位置应用。

## 容易踩的坑

### 1. 值类型未必可默认构造

`EnvironmentAttribute::Builder` 要求默认值经构造函数传入（不提供无参构造），因为值类型可能没有默认构造函数——`Activity` 是值对象，只有 `explicit Activity(const std::string&)`，`Value{}` 无法编译。

### 2. 离散值必须用 ofNotInterpolated

`Activity` 这类离散值通过 `AttributeType::ofNotInterpolated()` 创建，其 `keyframeLerp` 为 `LerpFunction::ofStep(1.0)`：区间内保持前一关键帧的值，只有采样进度抵达下一关键帧时才切换。若误用 `ofInterpolated`，会在关键帧之间产生无意义的中间值。

### 3. 属性必须是静态实例

`Timeline` 以 `EnvironmentAttribute` 的**地址**为键存储轨道（类型擦除所需），系统也以地址索引属性，
因而属性必须是有稳定地址的静态对象，例如 `EnvironmentAttributes::VILLAGER_ACTIVITY()` 返回的函数内静态实例。
在测试里临时 `builder(...).build()` 出一份同名属性，会得到"另一个属性"（默认值）。

### 4. 系统失效缓存必须贯穿到层

`EnvironmentAttributeSystem::invalidateTickCache()` 不只是清系统缓存：它还调用各 `TimeBased` 层的
`invalidateTickCache()`。原因见 `../timeline/README.md` 第 4 条——否则时间线层按旧 dayTime 的采样值
会被无限复用。

### 5. 修改器单例的声明与定义

修改器以 `static const` 类内声明、命名空间外 `inline const` 定义（如 `FloatModifier::MULTIPLY`）。
类内直接写 `static constexpr FloatModifier MULTIPLY{...}` 会因类尚不完整而无法实例化。
通过 `nonOwningSharedPtr` 包装时，所有权语义为"不释放"，修改器必须是静态存储期对象。

### 6. 队列/列表的共享所有权

`addLayer` 会把层以 `shared_ptr<const Layer>` 持有；`AttributeTrackSampler` 以 `unique_ptr` 转移到
层接口后不再回取。系统拷贝是值语义（层共享、缓存独立），但层本身是有状态的
（采样器缓存），拷贝出的系统与原件共享同一层实例——若两者独立推进 tick，需要各自调用
`invalidateTickCache()`。

### 7. 值类型的 JSON 解析特化必须经函数指针取得

`valueJsonReader<Value>()` 的定义放在 .cpp 中的显式特化里，主模板只声明不定义。
调用方必须 `&valueJsonReader<Value>` 取函数指针再调用——直接写法 `valueJsonReader<Value>()`
会让编译器在调用点按主模板实例化，特化**静默失效**（表现为所有属性都"没有 JSON 表示"，
且不报任何错误）。新增值类型时务必在 .cpp 中补特化，否则链接失败。

### 8. 数据包修改器以属性默认值为基础

`EnvironmentAttributeMap` 的非覆写修改器（add/multiply/…）总是以**属性默认值**为被修改值，
而不是调用方传入的当前值——与 vanilla `EnvironmentAttributeSystem` 的常量层折叠一致。
新增数据包驱动的属性时不要假设它会叠加到上一层的结果上。
