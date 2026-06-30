# 实体类型标签模块

实体类型标签（EntityTypeTag）用于将实体类型分组以便功能判断。

## 架构

```
entity/tag/
├── EntityTypeTag.hpp       # 实体类型标签类
├── EntityTypeTag.cpp       # 实体类型标签实现
├── EntityTypeTags.hpp      # 内置标签集合
├── EntityTypeTags.cpp      # 标签注册和初始化
├── EntityTypeTagLoader.hpp # 数据包 JSON 加载器
├── EntityTypeTagLoader.cpp # 加载器实现
└── README.md               # 本文档
```

## 设计模式

EntityTypeTag 采用与 BlockTag/FluidTag 一致的设计模式：

- 使用 `std::unordered_set<ResourceLocation>` 存储实体类型资源位置（如 `minecraft:arrow`）
- `EntityTypeTags` 静态注册表管理所有内置标签
- `EntityTypeTagLoader` 从数据包 JSON 文件加载标签内容

## 数据包格式

数据包路径：`data/<namespace>/tags/entity_type/<path>.json`

```json
{
  "replace": false,
  "values": [
    "minecraft:arrow",
    "#minecraft:arrows",
    { "id": "minecraft:optional_entity", "required": false }
  ]
}
```

## 使用示例

```cpp
// 检查实体类型是否在标签中
if (entityType.isIn(EntityTypeTags::IMPACT_PROJECTILES())) {
    // 该实体类型属于冲击投射物
}

// 检查实体是否在标签中（通过 getTypeId()）
if (EntityTypeTags::IMPACT_PROJECTILES().contains(entity.getTypeId())) {
    // 该实体属于冲击投射物
}
```

## 已注册标签

- `IMPACT_PROJECTILES` — 冲击投射物（箭矢、三叉戟、火球等，可破坏陶罐等方块）
- `ARROWS` — 箭矢（普通箭矢、光灵箭）
- `ARTHROPOD` — 节肢动物
- `UNDEAD` — 亡灵
- `SKELETONS` — 骷髅类
- `ZOMBIES` — 僵尸类
- `RAIDERS` — 袭击者
- 以及数据包中定义的其他标签
