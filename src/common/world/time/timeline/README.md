# 按时间采样属性和活动轨道

## 目录结构树

```text
timeline/
├── AttributeTrack.hpp  # 属性轨道
├── AttributeTrackSampler.hpp  # 属性轨道采样器
├── EasingType.hpp  # 缓动类型
├── Keyframe.hpp  # 关键帧
├── KeyframeTrack.hpp  # 关键帧轨道
├── KeyframeTrackSampler.hpp  # 关键帧轨道采样器
├── Timeline.hpp  # 时间线
├── TimelineRegistry.cpp  # 模块接口与实现
├── TimelineRegistry.hpp  # 时间线注册表
├── Timelines.cpp  # 模块接口与实现
└── Timelines.hpp  # 预定义时间线
```

## 内部模块关系

base 定义月相和默认周期规则，timeline 把累计时间采样成环境属性与活动值。月相规则本身不依赖属性求值器。

## 上下游外部依赖关系

环境属性、维度时间线、光照与难度计算消费共享规则；运行时时钟推进由 server/world/time 负责。

## 容易踩的坑

- dayTime 保留累积时间；仅日内计算归一化，月相按完整天数回绕。
- 月相枚举和默认亮度只维护一份；实际环境值由属性系统和时间线求值。
- 时间线关键帧、周期回绕和每 tick 缓存失效必须保持一致。
