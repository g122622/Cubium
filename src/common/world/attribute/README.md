# Attribute 环境属性系统

环境属性系统定义世界级的具名属性（活动、月相、天空颜色等），并提供按时间驱动这些属性所需的基础类型。

## 目录结构

```
attribute/
├── AttributeModifier.hpp         # 属性修改器接口与覆写修改器（OverrideModifier）
├── AttributeRange.hpp            # 属性值范围（校验 + 规范化）
├── AttributeType.hpp             # 属性类型（关键帧/状态变化/空间/部分tick 四个插值函数）
├── AttributeTypes.hpp/cpp        # 预定义属性类型（ACTIVITY）
├── EnvironmentAttribute.hpp      # 环境属性定义与构建器
├── EnvironmentAttributes.hpp/cpp # 预定义环境属性（VILLAGER_ACTIVITY、BABY_VILLAGER_ACTIVITY）
└── EnvironmentAttributeLayer.hpp # 属性层接口（TimeBased / Constant）
```

## 内部模块关系

```
EnvironmentAttributes ──构建──> EnvironmentAttribute ──持有──> AttributeType ──持有──> LerpFunction
                                     │                              ▲
                                     └──持有──> AttributeRange       │
                                                              AttributeTypes

AttributeModifier ──apply(subject, argument)──> 由 timeline 的 AttributeTrack 持有
EnvironmentAttributeLayer::TimeBased ──由 timeline 的 AttributeTrackSampler 实现
```

## 上下游外部依赖关系

**依赖（上游）**：
- `entity/ai/brain/schedule/Activity.hpp` — `AttributeTypes::ACTIVITY` 与村民活动属性以 `Activity` 作为值类型

**被依赖（下游）**：
- `world/timeline/` — `AttributeTrack` 持有 `AttributeModifier`；`AttributeTrackSampler` 实现 `EnvironmentAttributeLayer::TimeBased`；`Timeline::createTrackSampler` 读取 `EnvironmentAttribute::type()`
- `entity/ai/brain/Brain.hpp` — 按活动环境属性创建轨道采样器

## 容易踩的坑

### 1. 值类型未必可默认构造

`EnvironmentAttribute::Builder` 要求默认值经构造函数传入（不提供无参构造），因为值类型可能没有默认构造函数——`Activity` 是值对象，只有 `explicit Activity(const std::string&)`，`Value{}` 无法编译。

### 2. 离散值必须用 ofNotInterpolated

`Activity` 这类离散值通过 `AttributeType::ofNotInterpolated()` 创建，其 `keyframeLerp` 为 `LerpFunction::ofStep(1.0)`：区间内保持前一关键帧的值，只有采样进度抵达下一关键帧时才切换。若误用 `ofInterpolated`，会在关键帧之间产生无意义的中间值。

### 3. 属性必须是静态实例

`Timeline` 以 `EnvironmentAttribute` 的**地址**为键存储轨道（类型擦除所需），因而属性必须是有稳定地址的静态对象，例如 `EnvironmentAttributes::VILLAGER_ACTIVITY()` 返回的函数内静态实例。
