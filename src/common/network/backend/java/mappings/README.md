# Java 注册表 ID 映射

## 目录结构

```text
mappings/
├── JavaItemIdMap.hpp/cpp         # 物品注册序映射
├── JavaBlockIdMap.hpp/cpp        # 方块注册序映射
├── JavaBlockStateIdMap.hpp/cpp   # 方块状态注册序映射
├── JavaAttributeIdMap.hpp/cpp    # 属性注册序映射
├── JavaEnchantmentIdMap.hpp/cpp  # 附魔注册序映射
├── JavaMobEffectIdMap.hpp/cpp    # 状态效果注册序映射
├── JavaPotionIdMap.hpp/cpp       # 药水注册序映射
└── JavaDamageTypeIds.hpp        # 同步伤害类型注册序与业务类型映射
```

## 内部模块关系

映射表在协议边界把内部类型转换为 Java 注册序；伤害类型的条目顺序同时供 Configuration 和 damage_event 使用。

## 上下游外部依赖关系

依赖公共类型、资源名及烘焙数据；codec、实体同步和配置注册表构造消费这些映射。

## 容易踩的坑

- 业务枚举值不等于 wire ID，禁止直接强制转换。
- damage_type 属于同步注册表，ID 必须匹配本连接收到的 RegistryData 条目顺序。
- mob_effect 属于客户端内置注册表，使用纯 VarInt 注册 ID，不额外加一。
- 可选实体 ID 的加一编码与注册表 ID 是不同规则，不能混用。
