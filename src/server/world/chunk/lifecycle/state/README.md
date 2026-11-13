# 当前生命周期实例、区块发布与状态不变量

## 目录结构树

```text
state/
├── ChunkHolder.cpp  # 单区块阶段、来源、请求与邻居保活状态
├── ChunkHolder.hpp  # 单区块阶段、来源、请求与邻居保活状态
├── ChunkRegistry.cpp  # 统一拥有当前 holder 和共享 FULL 数据索引
└── ChunkRegistry.hpp  # 统一拥有当前 holder 和共享 FULL 数据索引
```

## 内部模块关系

ChunkRegistry 持有实例表与 FULL 索引；ChunkHolder 原子管理阶段、来源、请求及活跃任务。

## 上下游外部依赖关系

由生命周期控制器、来源加载、生成调度、卸载和诊断消费；依赖阶段与公共区块数据。

## 容易踩的坑

- 来源状态与 primer 必须原子安装；FULL 索引共享同一 ChunkData。旧实例的发布和退场不能影响同坐标新实例。shared_ptr 仅保活，不能替代数据同步。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
