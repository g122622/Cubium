# 方块注册属性构建与状态谓词

## 目录结构树

```text
properties/
├── BlockProperties.cpp  # 构造函数：从材质推导默认碰撞/固体/可燃性与默认声音类型
├── BlockProperties.hpp  # 注册期方块属性构建器（链式 setter，构造 Block 时逐字段摊平拷贝）
└── StatePredicate.hpp   # 可由多个方块共享的状态判定函数对象（函数指针 + 诊断名）
```

## 内部模块关系

`BlockProperties` 是**注册期**的值对象：`registry/` 下的注册代码构造它并逐项设置，`Block(BlockProperties)`
构造函数把每个字段**按值摊平**到 `Block` 的 `m_*` 成员（`Material*`、`BlockSoundType*`、`StatePredicate*`
指针一并拷贝），构造完成后 `BlockProperties` 对象即被丢弃，运行期不再存在，也不与 Block 保持任何引用关系。

`StatePredicate` 被 `suffocating()` / `viewBlocking()` 以裸指针纳入配置，实例必须具有静态存储期，
集中定义在 `registry/vanilla/BlockStatePredicates.hpp`，由 `Block::isSuffocatingAt` / `isViewBlockingAt` 求值。

与 `../capability/` 的分工：本目录描述方块"是什么"（静态参数，注册期一次性消费），capability 描述方块
"能做什么"（运行期 `dynamic_cast` 查询的可选行为接口）。两者无继承或包含关系，唯一交叉点是上述两个接收
`StatePredicate` 的 setter。

## 上下游外部依赖关系

上游：`registry/BlockRegistry.hpp` 与 `registry/vanilla/*Blocks` 负责构造并传入属性；`Block` 构造函数消费并摊平。
下游：`Block` / `BlockState` 只读自身成员，运行期不再引用本目录。本目录依赖 `base/material`、`base/sound`、
`base/types`（模型偏移、挖掘工具、音符盒乐器）与 `world/map/MaterialColor`。

## 容易踩的坑

- **注册后再改 BlockProperties 无效**：Block 构造时已按值摊平，二者没有持有关系。
- **谓词不能传临时对象**：`suffocating()` / `viewBlocking()` 保存裸指针，必须指向静态存储期对象。
- **窒息与视线默认共用同一判定**：只覆写 `suffocating()` 会连带改变视线判定，反之亦然；只改一个时必须两个都显式设置。
- **可替换性不从材质推导**：默认 false，必须显式 `replaceable()`（空气、水、草、雪层等少数方块）。
- **`noCollision()` 同时关闭遮挡**：只清碰撞会让光照把空气、花草当成遮挡体。
- **随机刻开关只是注册期默认值**：按状态动态决定随机刻（铜箱、花盆等）必须重写 `Block::ticksRandomly()` 虚函数。
- **`BlockProperties` 不是基类**：它是具体值类型，方块通过构造函数接收，不能从它派生。