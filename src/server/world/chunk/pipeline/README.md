# 区块生成阶段、依赖与推进约束

## 目录结构树

```text
pipeline/
├── ChunkDependencies.cpp  # 模块接口与实现
├── ChunkDependencies.hpp  # 区块生成阶段的依赖关系
├── ChunkPyramid.cpp  # 构建累积依赖
├── ChunkPyramid.hpp  # 区块生成金字塔
├── ChunkStatus.cpp  # 模块接口与实现
├── ChunkStatus.hpp  # 模块接口与实现
└── ChunkStep.hpp  # 区块生成步骤
```

## 内部模块关系

ChunkStatus 描述生成阶段；ChunkDependencies 按半径描述邻居要求；ChunkStep 汇总读写约束；ChunkPyramid 组合阶段依赖。这里不执行生成算法或调度线程。

## 上下游外部依赖关系

依赖 common/core 的值类型和容器；ChunkTaskScheduler、ChunkProgressionTask、单区块生命周期、生成器与区块发送消费本目录。

## 容易踩的坑

- 阶段模型和邻居依赖属于服务端，common 不得反向引用本目录。
- 生成步骤的直接依赖、累积依赖和可写半径含义不同。
