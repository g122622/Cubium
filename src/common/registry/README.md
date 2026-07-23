# Registry 模块

统一注册表访问入口，为网络层 codec 提供按整数 ID / 资源位置查表的聚合门面。

## 目录结构

```
src/common/registry/
├── RegistryAccess.hpp   # 统一注册表访问门面（聚合引用 Item/Block/Entity/Biome 四大单例）
└── RegistryAccess.cpp   # instance() 单例实现
```

## 内部模块关系

`RegistryAccess` 是非拥有聚合体，运行时转发到各注册表的 `instance()` 单例：
- `ItemRegistry`（`common/item/core/`）→ `itemById` / `itemByKey` / `itemIdOf`
- `BlockRegistry`（`common/world/block/`）→ `blockById` / `blockByKey` / `blockStateById`
- `EntityRegistry`（`common/entity/core/`）→ `entityTypeById` / `entityTypeByKey` / `entityTypeIdOf`
- `BiomeRegistry`（`common/world/biome/`）→ `biomeById`

## 上下游外部依赖关系

- **依赖**：四大注册表单例、`ResourceLocation`、`Types`。
- **下游**：网络层 `RegistryByteBuf` 持有本对象，序列化物品/方块/实体类型时按 VarInt 整数 ID 查回类型指针（对应 MC Java 1.21.11 `RegistryFriendlyByteBuf` 持有 `RegistryAccess` 的角色）。

## 容易踩的坑

1. **不统一接口**：四大注册表历史接口不一（key 有 u32/u16/string/ResourceLocation 之别，返回有指针/引用之别），本类提供具名类型化方法直接转发，**不套泛型 `Registry<T>` 基类**（避免 adapter 兼容层）。
2. **实体类型整数 ID 由注册顺序决定**：`EntityRegistry::registerType` 按 `push_back` 顺序分配 0,1,2,...，顺序由 `VanillaEntities::doRegisterAll` 源码冻结。增删实体或调顺序会导致 ID 漂移，破坏网络兼容。
3. **BiomeRegistry 无 byKey**：生物群系只有整数 ID 查询（`BiomeId` 硬编码于 `BiomeIds.hpp`），无按资源位置查询方法。
