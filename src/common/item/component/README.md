# 物品数据组件

## 目录结构

```text
component/
├── DataComponentType.hpp/cpp         # 组件资源名与注册 ID
├── DataComponentMap.hpp              # 异构载荷与补丁
├── DataComponentPatchWire.hpp/cpp    # 网络补丁及精确谓词
├── DataComponentPatchNbt.hpp/cpp     # 存档补丁
├── DataComponentPayloadCodec.hpp/cpp # 存档载荷转换
└── AttackRange.hpp/cpp               # 攻击范围值类型
```

## 内部模块关系

类型表驱动载荷转换；补丁区分新增和移除。网络编码与存档编码共用载荷定义，分别执行字段编码。

## 上下游外部依赖关系

ItemStack 导出、应用补丁，IR 物品桥接和存档调用编码器。本目录依赖文本、NBT、附魔与效果值类型，以及 Java 注册序映射。

## 容易踩的坑

- 组件 ID 必须匹配协议表 SlotComponentType。damage_type 占编号 8，后续组件不可少算这一项。
- 网络补丁先写两个计数，再写新增项和移除项；空补丁仍占两个零字节。
- 组件没有统一长度前缀，未知载荷必须报错，不能跳过后继续解析。
- 商人 ItemCost 的精确谓词只有一个计数，与 ItemStack 补丁不同。
