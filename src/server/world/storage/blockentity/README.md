# 方块实体存储

## 目录结构

```text
blockentity/
├── BlockEntityStorageManager.hpp/cpp # 按方块坐标保存、读取及删除方块实体 NBT
└── README.md                        # 模块边界与存储约束
```

## 内部模块关系

`BlockEntityStorageManager` 统一生成单个方块和区块前缀的键，调用 `RocksDBDatabase`
执行点读写、范围删除及有界范围读取；读取结果经 `BlockEntityDeserializer` 恢复对象。

## 上下游外部依赖关系

由 `SingleLevelStorageManager` 装配，区块加载和保存链路经存档门面访问。依赖方块实体
序列化、区块坐标工具和按维度区分的存储列族。

## 容易踩的坑

- 区块扫描通过 `readRange` 把结束键传到底层迭代器，避免扫描目标范围以外的已删除记录。
- 范围是半开区间，结束键不参与返回；边界 Slice 不得在迭代器之前销毁。
- 读取失败必须传播为错误，不能把迭代器错误当作空区块。
- 不同维度使用不同列族；单个方块实体的坐标键与所在区块前缀必须一致。
