# 实体组件序列化

## 目录结构树

```text
components/
├── ComponentSerializerRegistry.hpp/cpp # 组件保存和加载函数的统一注册入口
├── EntityComponentSerialization.hpp/cpp # 基础位置、运动、标记与环境状态
├── LivingEntityComponentSerialization.hpp/cpp # 生命、伤害和装备状态
├── PlayerComponentSerialization.hpp/cpp # 玩家分数状态
├── ArmorStandComponentSerialization.hpp/cpp # 盔甲架外观和姿态
├── ProjectileComponentSerialization.hpp/cpp # 投射物归属和特有状态
├── MinecartComponentSerialization.hpp/cpp # 矿车特有状态
└── HorseComponentSerialization.hpp/cpp # 马匹驯服、跳跃与属性状态
```

## 内部模块关系

ComponentSerializerRegistry 注册保存与加载函数，按优先级加载，Entity 的 NBT 入口统一调用。addon 自行提供属性组件序列化函数并在统一入口注册。

## 上下游外部依赖关系

由实体保存和恢复链路调用，依赖公共实体、ECS 组件与 NBT 类型。需要网络副作用的字段通过实体 setter 写入。

## 容易踩的坑

- 加载函数会被统一调用，不能假定对应组件已经存在。
- 有跨组件依赖时明确加载优先级；避免同时从虚函数链重复保存相同字段。
- addon 属性快照恢复前，需要实体工厂安装该类型的声明属性定义。

- addon 活动组件先于普通组件恢复，确保健康组件读取存档生命值时已经使用组件组提供的最大值。
